#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <glib.h>
#include <set>

#include "PulseDevice.h"
#include "../../public/SoundAgentInterface.h"
#include <pulse/glib-mainloop.h>
#include <pulse/pulseaudio.h>


class PulseDeviceCollection final  : public SoundDeviceCollectionInterface
{
public:
    PulseDeviceCollection();
    ~PulseDeviceCollection() override;

    DISALLOW_COPY_MOVE(PulseDeviceCollection);

    void ActivateAndStartLoop() override;
    void DeactivateAndStopLoop() override;

    [[nodiscard]] size_t GetSize() const override;
    [[nodiscard]] std::unique_ptr<SoundDeviceInterface> CreateItem(size_t deviceNumber) const override;
    [[nodiscard]] std::unique_ptr<SoundDeviceInterface> CreateItem(const std::string& devicePnpId) const override;

    void Subscribe(SoundDeviceObserverInterface& observer) override;
    void Unsubscribe(SoundDeviceObserverInterface& observer) override;

private:
    void RequestInitialInfo();
    void StartMonitoring();
    void StopMonitoring();

    void AddOrUpdateAndNotify(SoundDeviceEventType event, const std::string& pnpId, const std::string& name, uint32_t volume, SoundDeviceFlowType type);
    void CheckIfVolumeChangedAndNotify(const std::string& pnpId, uint16_t volume, SoundDeviceFlowType type);

    void NotifyObservers(SoundDeviceEventType action, const std::string& devicePNpId) const;

    static void ContextStateCallback(pa_context* c, void* userdata);
    static void SubscribeCallback(pa_context* c, pa_subscription_event_type_t t, uint32_t idx, void* userdata);
    static void ServerInfoCallback(pa_context* c, const pa_server_info* i, void* userdata);


    template<typename INFO_T_>
    static void InfoCallback(pa_context* context, const INFO_T_* info, int eol, void* userdata,
        SoundDeviceEventType event);

    template<typename INFO_T_>
    static void ChangedInfoCallback(pa_context* context, const INFO_T_* info, int eol, void* userdata);
    

    template<typename INFO_T_>
    void DeliverDeviceAndState(SoundDeviceEventType event, const INFO_T_& info);

    template<typename INFO_T_>
    void DeliverChangedState(const INFO_T_& info);
    
    template<typename INFO_T_>
    static std::pair<uint16_t, std::string> ExtractVolumeAndPnpId(const INFO_T_& info);


    // Wrapper functions to maintain the original callback signatures
    static void InitialInfoSinkCallback(pa_context* context, const pa_sink_info* sinkInfo, int eol, void* userdata)
    {
        InfoCallback(context, sinkInfo, eol, userdata, SoundDeviceEventType::Confirmed);
    }

    static void NewInfoSinkCallback(pa_context* context, const pa_sink_info* sinkInfo, int eol, void* userdata)
    {
        InfoCallback(context, sinkInfo, eol, userdata, SoundDeviceEventType::Discovered);
    }

    static void ChangedInfoSinkCallback(pa_context* context, const pa_sink_info* sinkInfo, int eol, void* userdata)
    {
        ChangedInfoCallback(context, sinkInfo, eol, userdata);
    }

    static void InitialInfoSourceCallback(pa_context* context, const pa_source_info* sourceInfo, int eol, void* userdata)
    {
        InfoCallback(context, sourceInfo, eol, userdata, SoundDeviceEventType::Confirmed);
    }

    static void NewInfoSourceCallback(pa_context* context, const pa_source_info* sourceInfo, int eol, void* userdata)
    {
        InfoCallback(context, sourceInfo, eol, userdata, SoundDeviceEventType::Discovered);
    }

    static void ChangedInfoSourceCallback(pa_context* context, const pa_source_info* sourceInfo, int eol, void* userdata)
    {
        ChangedInfoCallback(context, sourceInfo, eol, userdata);
    }

    [[nodiscard]] PulseDevice MergeDeviceWithExistingOneBasedOnPnpIdAndFlow(const PulseDevice& device) const;

private:
    pa_glib_mainloop* mainLoop_;
    pa_context* context_;
    GMainLoop* gMainLoop_;
    std::unordered_map<std::string, PulseDevice> pnpToDeviceMap_;
    std::set<SoundDeviceObserverInterface*> observers_;
};