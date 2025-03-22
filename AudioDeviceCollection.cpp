#include "AudioDeviceCollection.h"
#include <iostream>
#include <pulse/subscribe.h>
#include <pulse/glib-mainloop.h>

AudioDeviceCollection::AudioDeviceCollection() : context(nullptr), mainloop(nullptr) {
    mainloop = pa_glib_mainloop_new(nullptr);
    context = pa_context_new(pa_glib_mainloop_get_api(mainloop), "DeviceMonitor");
    
    pa_context_set_state_callback(context, contextStateCallback, this);
    pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
}

AudioDeviceCollection::~AudioDeviceCollection() {
    if(context) {
        pa_context_disconnect(context);
        pa_context_unref(context);
    }
    if(mainloop) pa_glib_mainloop_free(mainloop);
}

void AudioDeviceCollection::subscribe(std::shared_ptr<IDeviceSubscriber> subscriber) {
    subscribers.emplace_back(subscriber);
}

void AudioDeviceCollection::unsubscribe(std::shared_ptr<IDeviceSubscriber> subscriber) {
    subscribers.erase(
        std::remove_if(subscribers.begin(), subscribers.end(),
            [&](const auto& wp) { return wp.lock() == subscriber; }),
        subscribers.end()
    );
}

void AudioDeviceCollection::startMonitoring() {
    pa_context_subscribe(context, static_cast<pa_subscription_mask_t>(PA_SUBSCRIPTION_MASK_SINK | 
                                PA_SUBSCRIPTION_MASK_SOURCE |
                                PA_SUBSCRIPTION_MASK_SERVER),
                        nullptr, nullptr);
    pa_context_set_subscribe_callback(context, subscribeCallback, this);
}

void AudioDeviceCollection::updateDeviceList() {
    pa_operation* op = pa_context_get_server_info(context, serverInfoCallback, this);
    pa_operation_unref(op);
}

void AudioDeviceCollection::contextStateCallback(pa_context* c, void* userdata) {
    auto* self = static_cast<AudioDeviceCollection*>(userdata);
    if(pa_context_get_state(c) == PA_CONTEXT_READY) {
        self->updateDeviceList();
    }
}

void AudioDeviceCollection::subscribeCallback(pa_context* c, pa_subscription_event_type_t t, 
                                            uint32_t idx, void* userdata) {
    auto* self = static_cast<AudioDeviceCollection*>(userdata);
    const auto facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
    const auto type = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;

    if(facility == PA_SUBSCRIPTION_EVENT_SINK) {
        if(type == PA_SUBSCRIPTION_EVENT_REMOVE) {
            // Handle device removal
        } else {
            pa_operation* op = pa_context_get_sink_info_by_index(c, idx, 
                [](pa_context*, const pa_sink_info* i, int eol, void* ud) {
                    if(!eol && i) {
                        auto* self = static_cast<AudioDeviceCollection*>(ud);
                        // Update device collection and notify
                    }
                }, self);
            pa_operation_unref(op);
        }
    }
    // Similar handling for sources
}

void AudioDeviceCollection::serverInfoCallback(pa_context* c, const pa_server_info* info, void* userdata)
{
    auto* self = static_cast<AudioDeviceCollection*>(userdata);
    if (!info) {
        std::cerr << "Failed to get server info." << std::endl;
        return;
    }

    std::cout << "Server Info:" << std::endl;
    std::cout << "  Default Sink: " << info->default_sink_name << std::endl;
    std::cout << "  Default Source: " << info->default_source_name << std::endl;

    // Update the default sink and source in the device collection
    self->updateDeviceList();
/*    
    // Process server info
    // For example, you might want to get the default sink and source names
    std::string defaultSink = info->default_sink_name ? info->default_sink_name : "";
    std::string defaultSource = info->default_source_name ? info->default_source_name : "";
    
    // Then request info for all sinks and sources
    pa_operation* op = pa_context_get_sink_info_list(c, 
        [](pa_context*, const pa_sink_info* i, int eol, void* ud) {
            if (!eol && i) {
                auto* self = static_cast<AudioDeviceCollection*>(ud);
                // Process sink info
            }
        }, self);
    pa_operation_unref(op);
    
    op = pa_context_get_source_info_list(c,
        [](pa_context*, const pa_source_info* i, int eol, void* ud) {
            if (!eol && i) {
                auto* self = static_cast<AudioDeviceCollection*>(ud);
                // Process source info
            }
        }, self);
    pa_operation_unref(op);
    */
}

void AudioDeviceCollection::sinkInfoCallback(pa_context* c, const pa_sink_info* i, int eol, void* userdata) {
    auto* self = static_cast<AudioDeviceCollection*>(userdata);

    if (eol) {
        // End of list, no more sinks to process
        return;
    }

    if (!i) {
        std::cerr << "Failed to get sink info." << std::endl;
        return;
    }

    std::cout << "Sink Info:" << std::endl;
    std::cout << "  Name: " << i->name << std::endl;
    std::cout << "  Description: " << i->description << std::endl;
    std::cout << "  Volume: " << pa_cvolume_avg(&i->volume) << std::endl;

    // Add or update the sink in the device collection
    DeviceType type = DeviceType::Render;  // Sinks are output devices
    AudioDevice device(i->name, i->description, type, i->index);
    device.volume = i->volume;

    self->devices[i->index] = device;

    // Notify subscribers about the new/updated device
    DeviceEvent event{device, DeviceEventType::Added, i->volume};
    self->notifySubscribers(event);
}

void AudioDeviceCollection::sourceInfoCallback(pa_context* c, const pa_source_info* i, int eol, void* userdata) {
    auto* self = static_cast<AudioDeviceCollection*>(userdata);

    if (eol) {
        // End of list, no more sources to process
        return;
    }

    if (!i) {
        std::cerr << "Failed to get source info." << std::endl;
        return;
    }

    std::cout << "Source Info:" << std::endl;
    std::cout << "  Name: " << i->name << std::endl;
    std::cout << "  Description: " << i->description << std::endl;
    std::cout << "  Volume: " << pa_cvolume_avg(&i->volume) << std::endl;

    // Add or update the source in the device collection
    DeviceType type = DeviceType::Capture;  // Sources are input devices
    AudioDevice device(i->name, i->description, type, i->index);
    device.volume = i->volume;

    self->devices[i->index] = device;

    // Notify subscribers about the new/updated device
    DeviceEvent event{device, DeviceEventType::Added, i->volume};
    self->notifySubscribers(event);
}

void AudioDeviceCollection::notifySubscribers(const DeviceEvent& event) {
    // Iterate through the list of subscribers
    for (auto it = subscribers.begin(); it != subscribers.end();) {
        // Lock the weak_ptr to get a shared_ptr
        if (auto subscriber = it->lock()) {
            // Notify the subscriber about the event
            subscriber->onDeviceEvent(event);
            ++it;  // Move to the next subscriber
        } else {
            // If the weak_ptr is expired (subscriber no longer exists), remove it from the list
            it = subscribers.erase(it);
        }
    }
}