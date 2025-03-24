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

    void Subscribe(std::shared_ptr<IDeviceSubscriber> subscriber);
    void Unsubscribe(std::shared_ptr<IDeviceSubscriber> subscriber);
    
    void GetServerInfo();
    void StartMonitoring();
    void stopMonitoring();

private:
    std::pair<std::string, std::string> GetBetterDeviceNames(const pa_proplist* proplist,
        const std::string& defaultId,
        const std::string& defaultName);
    void NotifySubscribers(const DeviceEvent& event);
    static void ContextStateCallback(pa_context* c, void* userdata);
    static void SubscribeCallback(pa_context* c, pa_subscription_event_type_t t, uint32_t idx, void* userdata);
    static void ServerInfoCallback(pa_context* c, const pa_server_info* i, void* userdata);
    static void SinkInfoCallback(pa_context* c, const pa_sink_info* i, int eol, void* userdata);
    static void SourceInfoCallback(pa_context* c, const pa_source_info* i, int eol, void* userdata);

private:
    pa_glib_mainloop* mainloop_;
    pa_context* context_;
    std::unordered_map<uint32_t, AudioDevice> devices_;
    std::vector<std::weak_ptr<IDeviceSubscriber>> subscribers_;

};