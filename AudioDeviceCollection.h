#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

#include "AudioDevice.h"

#include <pulse/glib-mainloop.h>


enum class DeviceEventType { Added, Removed, VolumeChanged };

struct DeviceEvent {
    AudioDevice device;
    DeviceEventType type;
    pa_cvolume newVolume;
};

class IDeviceSubscriber {
public:
    virtual ~IDeviceSubscriber() = default;
    virtual void onDeviceEvent(const DeviceEvent& event) = 0;
};

class AudioDeviceCollection {
public:
    AudioDeviceCollection();
    ~AudioDeviceCollection();

    void subscribe(std::shared_ptr<IDeviceSubscriber> subscriber);
    void unsubscribe(std::shared_ptr<IDeviceSubscriber> subscriber);
    
    void updateDeviceList();
    void startMonitoring();
    void stopMonitoring();

private:
    pa_glib_mainloop* mainloop;
    pa_context* context;
    std::unordered_map<uint32_t, AudioDevice> devices;
    std::vector<std::weak_ptr<IDeviceSubscriber>> subscribers;
    
    void notifySubscribers(const DeviceEvent& event);
    static void contextStateCallback(pa_context* c, void* userdata);
    static void subscribeCallback(pa_context* c, pa_subscription_event_type_t t, uint32_t idx, void* userdata);
    static void serverInfoCallback(pa_context* c, const pa_server_info* i, void* userdata);
    static void sinkInfoCallback(pa_context* c, const pa_sink_info* i, int eol, void* userdata);
    static void sourceInfoCallback(pa_context* c, const pa_source_info* i, int eol, void* userdata);
};