#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <glib.h>
#include <set>

#include "PulseDevice.h"
#include "SoundAgentInterface.h"


#include <pulse/glib-mainloop.h>
#include <pulse/pulseaudio.h>


class PulseDeviceCollection final  : public SoundDeviceCollectionInterface
{
public:
    PulseDeviceCollection();
    ~PulseDeviceCollection() override;

    DISALLOW_COPY_MOVE(PulseDeviceCollection);

    void ActivateAndStartLoop();
    void DeactivateAndStopLoop();

    void Subscribe(SoundDeviceObserverInterface& observer) override;
    void Unsubscribe(SoundDeviceObserverInterface& observer) override;

    [[nodiscard]] std::unique_ptr<SoundDeviceInterface> CreateItem(const std::string& devicePnpId) const override;

private:
    void GetServerInfo();
    void StartMonitoring();
    void StopMonitoring();

    void AddOrUpdateAndNotify(const std::string& pnpId, const std::string& name, uint32_t volume, SoundDeviceFlowType type, uint32_t index);
    void NotifyObservers(SoundDeviceEventType action, const std::string& devicePNpId) const;

    static void ContextStateCallback(pa_context* c, void* userdata);
    static void SubscribeCallback(pa_context* c, pa_subscription_event_type_t t, uint32_t idx, void* userdata);
    static void ServerInfoCallback(pa_context* c, const pa_server_info* i, void* userdata);
    static void SinkInfoCallback(pa_context* context, const pa_sink_info* info, int eol, void* userdata);
    static void SourceInfoCallback(pa_context* context, const pa_source_info* info, int eol, void* userdata);

public:

private:
    pa_glib_mainloop* mainloop_;
    pa_context* context_;
    GMainLoop* gMainLoop_;
    std::unordered_map<std::string, PulseDevice> pnpToDeviceMap_;
    std::set<SoundDeviceObserverInterface*> observers_;
};