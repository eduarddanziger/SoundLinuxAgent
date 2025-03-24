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
    pa_context_subscribe(context_
    , static_cast<pa_subscription_mask_t>(
        PA_SUBSCRIPTION_MASK_SINK | 
        PA_SUBSCRIPTION_MASK_SOURCE | 
        PA_SUBSCRIPTION_MASK_SERVER |
        PA_SUBSCRIPTION_MASK_SINK_INPUT |    // Add this for volume changes
        PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT   // Add this for source volume changes
    )
    , nullptr, nullptr);
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

void AudioDeviceCollection::AddOrUpdateAndNotify(const std::string& id, const std::string& name, uint32_t volume, DeviceType type, uint32_t index)
{
    /*
    std::cout << "----- Info ------" << std::endl;
    std::cout << "Id (orig .name): " << id << std::endl;
    std::cout << "Name (orig .description): " << name << std::endl;
    std::cout << "Volume: " << volume << std::endl;
    std::cout << "Index: " << index << std::endl;
    */

    // Add or update the sink in the device collection
    AudioDevice device(id, name, type, index, volume);

    devices_[index] = device;

    // Notify subscribers about the new/updated device
    const DeviceEvent event{ device, DeviceEventType::Added} ;
    NotifySubscribers(event);
}

void AudioDeviceCollection::SinkInfoCallback(pa_context* context, const pa_sink_info* info, int eol, void* userdata) {
    LOG_SCOPE();
    auto* self = static_cast<AudioDeviceCollection*>(userdata);

    if (eol) {
        // End of list, no more sinks to process
        return;
    }

    if (!info) {
        std::cerr << "Failed to get sink info." << std::endl;
        return;
    }

    const auto [deviceId, deviceName] = std::make_pair(std::string("SINC:") + info->name, std::string(info->description));
    const uint32_t volume = pa_cvolume_avg(&info->volume);
    constexpr auto type = DeviceType::Render;  // Sinks are output devices
    const uint32_t index = info->index;  // Sinks are output devices

    self->AddOrUpdateAndNotify(deviceId, deviceName, volume, type, index);
}

void AudioDeviceCollection::SourceInfoCallback(pa_context* context, const pa_source_info* info, int eol, void* userdata) {
    LOG_SCOPE();
    auto* self = static_cast<AudioDeviceCollection*>(userdata);

    if (eol) {
        // End of list, no more sources to process
        return;
    }

    if (!info) {
        std::cerr << "Failed to get source info." << std::endl;
        return;
    }

    const auto [deviceId, deviceName] = std::make_pair(std::string("SOURCE:") + info->name, std::string(info->description));
    const uint32_t volume = pa_cvolume_avg(&info->volume);
    constexpr auto type = DeviceType::Capture;  // Sinks are output devices
    const uint32_t index = info->index;  // Sinks are output devices

    self->AddOrUpdateAndNotify(deviceId, deviceName, volume, type, index);
}

void AudioDeviceCollection::NotifySubscribers(const DeviceEvent& event) {
    LOG_SCOPE();
    // Iterate through the list of subscribers
    for (auto it = subscribers_.begin(); it != subscribers_.end();) {
        // Lock the weak_ptr to get a shared_ptr
        if (auto subscriber = it->lock()) {
            // Notify the subscriber about the event
            subscriber->OnDeviceEvent(event);
            ++it;  // Move to the next subscriber
        } else {
            // If the weak_ptr is expired (subscriber no longer exists), remove it from the list
            it = subscribers_.erase(it);
        }
    }
}