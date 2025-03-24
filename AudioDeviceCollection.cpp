#include "AudioDeviceCollection.h"
#include <iostream>
#include <pulse/subscribe.h>
#include <pulse/glib-mainloop.h>
#include <pulse/proplist.h>

#include "ScopeLogger.h"


AudioDeviceCollection::AudioDeviceCollection()
    : mainloop_(nullptr)
    , context_(nullptr)
{
    LOG_SCOPE();
    mainloop_ = pa_glib_mainloop_new(nullptr);
    context_ = pa_context_new(pa_glib_mainloop_get_api(mainloop_), "DeviceMonitor");

    pa_context_set_state_callback(context_, ContextStateCallback, this);
    pa_context_connect(context_, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
}

AudioDeviceCollection::~AudioDeviceCollection() {
    LOG_SCOPE();
    if(context_) {
        pa_context_disconnect(context_);
        pa_context_unref(context_);
    }
    if(mainloop_) pa_glib_mainloop_free(mainloop_);
}

void AudioDeviceCollection::Subscribe(std::shared_ptr<IDeviceSubscriber> subscriber) {
    subscribers_.emplace_back(subscriber);
}

void AudioDeviceCollection::Unsubscribe(std::shared_ptr<IDeviceSubscriber> subscriber)
{
    std::erase_if(subscribers_,
                  [&](const auto & wp) { return wp.lock() == subscriber; });
}

void AudioDeviceCollection::StartMonitoring() {
    LOG_SCOPE();
    pa_context_subscribe(context_, static_cast<pa_subscription_mask_t>(PA_SUBSCRIPTION_MASK_SINK | 
                                PA_SUBSCRIPTION_MASK_SOURCE |
                                PA_SUBSCRIPTION_MASK_SERVER),
                        nullptr, nullptr);
    pa_context_set_subscribe_callback(context_, SubscribeCallback, this);
}

void AudioDeviceCollection::GetServerInfo() {
    spdlog::info("SERVER: Requesting info...");
    pa_operation* op = pa_context_get_server_info(context_, ServerInfoCallback, this);
    pa_operation_unref(op);
}

void AudioDeviceCollection::ContextStateCallback(pa_context* c, void* userdata)
{
    auto* self = static_cast<AudioDeviceCollection*>(userdata);
    if(pa_context_get_state(c) == PA_CONTEXT_READY) {
        self->GetServerInfo();
    }
}

void AudioDeviceCollection::SubscribeCallback(pa_context* c, pa_subscription_event_type_t t,
    uint32_t idx, void* userdata)
{
    LOG_SCOPE();
    auto* self = static_cast<AudioDeviceCollection*>(userdata);
    const auto facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
    const auto type = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;

    if (facility == PA_SUBSCRIPTION_EVENT_SINK) {
        if (type == PA_SUBSCRIPTION_EVENT_REMOVE) {
            spdlog::info("SINK index {}: Removing...", idx);
            if (self->devices_.count(idx) > 0) {
                AudioDevice device = self->devices_[idx];
                self->devices_.erase(idx);

                // Notify about removal
                DeviceEvent event{ device, DeviceEventType::Removed };
                self->NotifySubscribers(event);
            }
        }
        else {
            spdlog::info("SINK index {}: Adding or updating...", idx);
            pa_operation* op = pa_context_get_sink_info_by_index(c, idx, SinkInfoCallback, self);
            pa_operation_unref(op);
        }
    }
    else if (facility == PA_SUBSCRIPTION_EVENT_SOURCE) {
        if (type == PA_SUBSCRIPTION_EVENT_REMOVE) {
            spdlog::info("SOURCE index {}: Removing...", idx);
            if (self->devices_.count(idx) > 0) {
                AudioDevice device = self->devices_[idx];
                self->devices_.erase(idx);

                // Notify about removal
                DeviceEvent event{ device, DeviceEventType::Removed };
                self->NotifySubscribers(event);
            }
        }
        else {
            spdlog::info("SOURCE index {}: Adding or updating...", idx);
            pa_operation* op = pa_context_get_source_info_by_index(c, idx, SourceInfoCallback, self);
            pa_operation_unref(op);
        }
    }
}
void AudioDeviceCollection::ServerInfoCallback(pa_context* c, const pa_server_info* info, void* userdata)
{
    LOG_SCOPE();

    auto* self = static_cast<AudioDeviceCollection*>(userdata);
    if (!info) {
        std::cerr << "Failed to get server info." << std::endl;
        return;
    }

    std::cout << "SERVER Info:" << std::endl;
    std::cout << "  Default Sink: " << info->default_sink_name << std::endl;
    std::cout << "  Default Source: " << info->default_source_name << std::endl;

    // Process server info
    // For example, you might want to get the default sink and source names
    std::string defaultSink = info->default_sink_name ? info->default_sink_name : "";
    std::string defaultSource = info->default_source_name ? info->default_source_name : "";
    
    // Then request info for all sinks and sources
    spdlog::info("SINK: Requesting info...");
    pa_operation* op = pa_context_get_sink_info_list(c, SinkInfoCallback, self);
    pa_operation_unref(op);

    spdlog::info("SOURCE: Requesting info...");
    op = pa_context_get_source_info_list(c, SourceInfoCallback, self);
    pa_operation_unref(op);
}

std::pair<std::string, std::string> AudioDeviceCollection::GetBetterDeviceNames(const pa_proplist* proplist, const std::string& defaultId, const std::string& defaultName) {
    std::string deviceId = defaultId;
    std::string deviceName = defaultName;

    if (proplist) {
/*
        // Print all properties for debugging
        std::cout << "  Properties:" << std::endl;

        // Instead of using pa_proplist_foreach, use iterative property access
        const char* key = nullptr;
        void* state = nullptr;

        while ((key = pa_proplist_iterate(proplist, &state))) {
            const char* value = pa_proplist_gets(proplist, key);
            if (value) {
                std::cout << "    " << key << ": " << value << std::endl;
            }
            else {
                std::cout << "    " << key << ": [binary data]" << std::endl;
            }
        }
*/

        // Look for the actual hardware device information
        const char* device_id = pa_proplist_gets(proplist, "device.id");
        const char* device_nick = pa_proplist_gets(proplist, "device.nick");
        const char* device_api = pa_proplist_gets(proplist, "device.api");

        // Use better identification if available
        if (device_id) {
            deviceId = device_id;
        }

        // Try to construct a more meaningful name
        if (device_nick) {
            deviceName = device_nick;
        }

        // Append API information if available
        if (device_api) {
            deviceName += " (" + std::string(device_api) + ")";
        }
    }

    return { deviceId, deviceName };
}


void AudioDeviceCollection::SinkInfoCallback(pa_context* c, const pa_sink_info* i, int eol, void* userdata) {
    LOG_SCOPE();
    auto* self = static_cast<AudioDeviceCollection*>(userdata);

    if (eol) {
        // End of list, no more sinks to process
        return;
    }

    if (!i) {
        std::cerr << "Failed to get sink info." << std::endl;
        return;
    }

    const auto [deviceId, deviceName] = std::make_pair(std::string("SINC:") + i->name, std::string(i->description));
    // auto [deviceId, deviceName] = self->GetBetterDeviceNames(i->proplist, i->name, i->description);

    std::cout << "----- Info ------" << std::endl;
    std::cout << "Id (orig .name): " << deviceId << std::endl;
    std::cout << "Name (orig .description): " << deviceName << std::endl;
    std::cout << "Volume: " << pa_cvolume_avg(&i->volume) << std::endl;

    // Add or update the sink in the device collection
    DeviceType type = DeviceType::Render;  // Sinks are output devices
    AudioDevice device(deviceId, deviceName, type, i->index);
    device.volume = i->volume;

    self->devices_[i->index] = device;

    // Notify subscribers about the new/updated device
    DeviceEvent event{ device, DeviceEventType::Added} ;
    self->NotifySubscribers(event);
}

void AudioDeviceCollection::SourceInfoCallback(pa_context* c, const pa_source_info* i, int eol, void* userdata) {
    LOG_SCOPE();
    auto* self = static_cast<AudioDeviceCollection*>(userdata);

    if (eol) {
        // End of list, no more sources to process
        return;
    }

    if (!i) {
        std::cerr << "Failed to get source info." << std::endl;
        return;
    }

    std::cout << "-- SOURCE Info --" << std::endl;
    std::cout << "Id (orig .name): " << i->name << std::endl;
    std::cout << "Name (orig .description): " << i->description << std::endl;
    std::cout << "Driver: " << (i->driver ? i->driver : "Unknown") << std::endl;
    std::cout << "Volume: " << pa_cvolume_avg(&i->volume) << std::endl;

    // Get better device names using our helper function
    auto [deviceId, deviceName] = self->GetBetterDeviceNames(i->proplist, i->name, i->description);

    // Add or update the source in the device collection
    DeviceType type = DeviceType::Capture;  // Sources are input devices
    AudioDevice device(deviceId, deviceName, type, i->index);
    device.volume = i->volume;

    self->devices_[i->index] = device;

    // Notify subscribers about the new/updated device
    DeviceEvent event{ device, DeviceEventType::Added };
    self->NotifySubscribers(event);
}

void AudioDeviceCollection::NotifySubscribers(const DeviceEvent& event) {
    LOG_SCOPE();
    // Iterate through the list of subscribers
    for (auto it = subscribers_.begin(); it != subscribers_.end();) {
        // Lock the weak_ptr to get a shared_ptr
        if (auto subscriber = it->lock()) {
            // Notify the subscriber about the event
            subscriber->onDeviceEvent(event);
            ++it;  // Move to the next subscriber
        } else {
            // If the weak_ptr is expired (subscriber no longer exists), remove it from the list
            it = subscribers_.erase(it);
        }
    }
}