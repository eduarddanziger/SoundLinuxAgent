#include "PulseDeviceCollection.h"

#include "../ScopeLogger.h"
#include "StringUtils.h"

#include <pulse/subscribe.h>
#include <pulse/glib-mainloop.h>
#include <pulse/proplist.h>

#include <ranges>
#include <iostream>
#include <spdlog/spdlog.h>

PulseDeviceCollection::PulseDeviceCollection()
    : mainLoop_(nullptr)
    , context_(nullptr)
{
    LOG_SCOPE();
    gMainLoop_ = g_main_loop_new(nullptr, FALSE);
    mainLoop_ = pa_glib_mainloop_new(g_main_loop_get_context(gMainLoop_));
    context_ = pa_context_new(pa_glib_mainloop_get_api(mainLoop_), "DeviceMonitor");
}

PulseDeviceCollection::~PulseDeviceCollection() {
    LOG_SCOPE();
    if(context_) {
        pa_context_unref(context_);
    }
    if(mainLoop_) pa_glib_mainloop_free(mainLoop_);
    if(gMainLoop_) g_main_loop_unref(gMainLoop_);
}

void PulseDeviceCollection::ActivateAndStartLoop() {
    LOG_SCOPE();
    pa_context_set_state_callback(context_, ContextStateCallback, this);
    pa_context_connect(context_, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
	g_main_loop_run(gMainLoop_);
}

void PulseDeviceCollection::DeactivateAndStopLoop() {
    LOG_SCOPE();
    StopMonitoring();
    pa_context_disconnect(context_);
    g_main_loop_quit(gMainLoop_);
}

void PulseDeviceCollection::Subscribe(SoundDeviceObserverInterface & observer)
{
    observers_.insert(&observer);
}

void PulseDeviceCollection::Unsubscribe(SoundDeviceObserverInterface & observer)
{
    observers_.erase(&observer);
}

size_t PulseDeviceCollection::GetSize() const
{
    return pnpToDeviceMap_.size();
}

std::unique_ptr<SoundDeviceInterface> PulseDeviceCollection::CreateItem(size_t deviceNumber) const
{
    if (deviceNumber >= pnpToDeviceMap_.size())
    {
        throw std::runtime_error("Device number is too big");
    }
    size_t i = 0;
    for (const auto & recordVal : pnpToDeviceMap_ | std::views::values)
    {
        if (i++ == deviceNumber)
        {
            return std::make_unique<PulseDevice>(recordVal);
        }
    }
    throw std::runtime_error("Device number not found");
}

std::unique_ptr<SoundDeviceInterface> PulseDeviceCollection::CreateItem(const std::string & devicePnpId) const
{
	LOG_SCOPE();
    if (!pnpToDeviceMap_.contains(devicePnpId))
    {
        throw std::runtime_error("Device pnpId not found");
    }
    return std::make_unique<PulseDevice>(pnpToDeviceMap_.at(devicePnpId));
}

void PulseDeviceCollection::StartMonitoring() {
    LOG_SCOPE();

    pa_context_set_subscribe_callback(context_, SubscribeCallback, this);

    pa_operation* op = pa_context_subscribe(context_
    , static_cast<pa_subscription_mask_t>(
        PA_SUBSCRIPTION_MASK_SINK | 
        PA_SUBSCRIPTION_MASK_SOURCE | 
        PA_SUBSCRIPTION_MASK_SERVER |
        PA_SUBSCRIPTION_MASK_SINK_INPUT |    // including volume changes for sink
        PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT   // including volume changes for sink
    )
    , nullptr, nullptr);

    if (op) {
        pa_operation_unref(op);
        spdlog::info("Started monitoring PulseAudio events");
    }
    else {
        spdlog::error("Failed to start monitoring PulseAudio events");
    }
}

void PulseDeviceCollection::StopMonitoring() {
    LOG_SCOPE();
    
    // Disable subscription callback
    if (context_ && pa_context_get_state(context_) == PA_CONTEXT_READY) {
        pa_context_set_subscribe_callback(context_, nullptr, nullptr);
        pa_operation* op = pa_context_subscribe(context_, 
                                              PA_SUBSCRIPTION_MASK_NULL,
                                              nullptr, nullptr);
        if (op) {
            pa_operation_unref(op);
            spdlog::info("Stopped monitoring PulseAudio events");
        }
        else {
            spdlog::warn("Failed to stop monitoring from PulseAudio events");
        }
    } else {
        spdlog::warn("Context not ready, can't stop monitoring");
    }
}

void PulseDeviceCollection::RequestInitialInfo() {
    spdlog::info("SERVER: Requesting info...");
    pa_operation* op = pa_context_get_server_info(context_, ServerInfoCallback, this);
    pa_operation_unref(op);
}

template<typename INFO_T_>
void PulseDeviceCollection::InfoCallback(pa_context*, const INFO_T_* info, int eol, void* userdata,
    SoundDeviceEventType event) {
    auto* self = static_cast<PulseDeviceCollection*>(userdata);

    if (eol) {
        return;
    }

    if (!info) {
        std::cerr << "Failed to get pulse audio device info." << std::endl;
        return;
    }

    self->DeliverDeviceAndState(event, *info);
}

template<typename INFO_T_>
void PulseDeviceCollection::DeliverDeviceAndState(SoundDeviceEventType event, const INFO_T_& info) {
    constexpr auto deviceFlowType = std::is_same_v<INFO_T_, pa_sink_info> ? SoundDeviceFlowType::Render :
        (std::is_same_v<INFO_T_, pa_source_info> ? SoundDeviceFlowType::Capture : SoundDeviceFlowType::None);
    static_assert(deviceFlowType != SoundDeviceFlowType::None,
        "DeliverDeviceAndState can only be used with pa_sink_info or pa_source_info types");

    const auto [volume, pnpId] = ExtractVolumeAndPnpId(info);

    std::string deviceName = info.description;

    if (constexpr auto monitorPrefix = "Monitor of ";
        deviceName.starts_with(monitorPrefix))
    {
        deviceName = deviceName.substr(std::strlen(monitorPrefix));
    }

    if (event == SoundDeviceEventType::Confirmed || event == SoundDeviceEventType::Discovered) {
        AddOrUpdateAndNotify(event, pnpId, deviceName, volume, deviceFlowType);
    }
    else if (event != SoundDeviceEventType::Detached) {
        // NotifyObservers(event, pnpId);
    }
}

template<typename INFO_T_>
void PulseDeviceCollection::ChangedInfoCallback(pa_context*, const INFO_T_* info, int eol, void* userdata)
{
    auto* self = static_cast<PulseDeviceCollection*>(userdata);

    if (eol) {
        return;
    }

    if (!info) {
        std::cerr << "Failed to get pulse audio device info." << std::endl;
        return;
    }

    self->DeliverChangedState(*info);
}

template<typename INFO_T_>
void PulseDeviceCollection::DeliverChangedState(const INFO_T_& info) {
    constexpr auto deviceFlowType = std::is_same_v<INFO_T_, pa_sink_info> ? SoundDeviceFlowType::Render :
        (std::is_same_v<INFO_T_, pa_source_info> ? SoundDeviceFlowType::Capture : SoundDeviceFlowType::None);
    static_assert(deviceFlowType != SoundDeviceFlowType::None,
        "DeliverChangedState can only be used with pa_sink_info or pa_source_info types");

    const auto [volume, pnpId] = ExtractVolumeAndPnpId(info);

    CheckIfVolumeChangedAndNotify(pnpId, volume, deviceFlowType);
}




void PulseDeviceCollection::ContextStateCallback(pa_context* c, void* userdata) {
    auto* self = static_cast<PulseDeviceCollection*>(userdata);

    pa_context_state_t state = pa_context_get_state(c);
    const int stateAsInt = static_cast<int>(state);

    switch (state) {
        case PA_CONTEXT_READY:
            spdlog::info("PulseAudio context got READY status, state: {}", stateAsInt);
            self->RequestInitialInfo();
            self->StartMonitoring();
            break;
                
        case PA_CONTEXT_FAILED:
            spdlog::error(
                "PulseAudio context got FAILED status (state {}): {}", stateAsInt, pa_strerror(pa_context_errno(c))
            );
            break;
            
        case PA_CONTEXT_TERMINATED:
            spdlog::info("PulseAudio context got TERMINATED status, state: {}", stateAsInt);
            break;
            
        default:
            // Still connecting or other states
            spdlog::info("PulseAudio context's state: {}", stateAsInt);
            break;
    }
}

void PulseDeviceCollection::SubscribeCallback(pa_context* c, pa_subscription_event_type_t t,
    uint32_t idx, void* userdata)
{
    LOG_SCOPE();
    auto* self = static_cast<PulseDeviceCollection*>(userdata);
    const auto facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
    const auto operation = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;

    if (facility == PA_SUBSCRIPTION_EVENT_SINK) {
        if (operation == PA_SUBSCRIPTION_EVENT_NEW)
        {
            spdlog::info("SINK index {}: Discovered...", idx);
            pa_operation* op = pa_context_get_sink_info_by_index(c, idx, NewInfoSinkCallback, self);
            pa_operation_unref(op);
        }
        else if (operation == PA_SUBSCRIPTION_EVENT_REMOVE) {
            spdlog::info("SINK index {}: Removing...", idx);
/*  
            pa_operation* op = pa_context_get_sink_info_by_index(c, idx, SinkInfoCallback, self);
            pa_operation_unref(op);
           if (self->pnpToDeviceMap_.count(idx) > 0) {
                PulseDevice device = self->pnpToDeviceMap_[idx];
                self->pnpToDeviceMap_.erase(idx);

                // Notify about removal
                DeviceEvent event{ device, DeviceEventType::Removed };
                self->NotifySubscribers(event);
            }
 */        
        }
        else if (operation == PA_SUBSCRIPTION_EVENT_CHANGE)
        {
            spdlog::info("SINC index {}: Changed...", idx);
            pa_operation* op = pa_context_get_sink_info_by_index(c, idx, ChangedInfoSinkCallback, self);
            pa_operation_unref(op);
        }
    }
    else if (facility == PA_SUBSCRIPTION_EVENT_SOURCE) {
        if (operation == PA_SUBSCRIPTION_EVENT_NEW)
        {
            spdlog::info("SOURCE index {}:  Discovered...", idx);
            pa_operation* op = pa_context_get_source_info_by_index(c, idx, NewInfoSourceCallback, self);
            pa_operation_unref(op);
        }
        else if (operation == PA_SUBSCRIPTION_EVENT_REMOVE) {
            spdlog::info("SOURCE index {}: Removing...", idx);
        }
        else if (operation == PA_SUBSCRIPTION_EVENT_CHANGE) {
            spdlog::info("SOURCE index {}: Changed...", idx);
            pa_operation* op = pa_context_get_source_info_by_index(c, idx, ChangedInfoSourceCallback, self);
            pa_operation_unref(op);
        }
    }
}
void PulseDeviceCollection::ServerInfoCallback(pa_context* c, const pa_server_info* info, void* userdata)
{
    LOG_SCOPE();

    auto* self = static_cast<PulseDeviceCollection*>(userdata);
    if (!info) {
        spdlog::error("Failed to get server info.");
        return;
    }

    // Process server info
    // For example, you might want to get the default sink and source names
    std::string defaultSink = info->default_sink_name ? info->default_sink_name : "";
    std::string defaultSource = info->default_source_name ? info->default_source_name : "";
    
    // Request initial info for all sinks and sources
    spdlog::info("SINK: Requesting info...");
    pa_operation* op = pa_context_get_sink_info_list(c, InitialInfoSinkCallback, self);
    pa_operation_unref(op);

    spdlog::info("SOURCE: Requesting info...");
    op = pa_context_get_source_info_list(c, InitialInfoSourceCallback, self);
    pa_operation_unref(op);
}

PulseDevice PulseDeviceCollection::MergeDeviceWithExistingOneBasedOnPnpIdAndFlow(const PulseDevice & device) const
{
    if
        (
            const auto foundPair = pnpToDeviceMap_.find(device.GetPnpId())
            ; foundPair != pnpToDeviceMap_.end()
         )
    {
        auto flow = device.GetFlow();
        uint16_t renderVolume = device.GetCurrentRenderVolume();
        uint16_t captureVolume = device.GetCurrentCaptureVolume();

		auto deviceName = device.GetName();
        if (const auto& foundDev = foundPair->second;
            foundDev.GetFlow() != device.GetFlow())
        {

            switch (flow)
            {
            case SoundDeviceFlowType::Capture:
                renderVolume = foundDev.GetCurrentRenderVolume();
                break;
            case SoundDeviceFlowType::Render:
                captureVolume = foundDev.GetCurrentCaptureVolume();
            default:
                break;
            }

            flow = SoundDeviceFlowType::RenderAndCapture;

            auto foundDevNameAsSet = ed::Split(foundDev.GetName(), '|');
            foundDevNameAsSet.insert(device.GetName());
            deviceName = ed::Merge(foundDevNameAsSet, '|');
        }

        return {
            device.GetPnpId(), deviceName, flow, renderVolume, captureVolume
        };
    }
    return device;
}

void PulseDeviceCollection::AddOrUpdateAndNotify(SoundDeviceEventType event, const std::string& pnpId, const std::string& name, uint32_t volume, SoundDeviceFlowType type)
{
    // Add or update the sink in the device collection
    const PulseDevice device(pnpId, name, type
        ,type == SoundDeviceFlowType::Render ? volume : 0
        ,type == SoundDeviceFlowType::Capture ? volume : 0);
    
    pnpToDeviceMap_[pnpId] = MergeDeviceWithExistingOneBasedOnPnpIdAndFlow(device);

    NotifyObservers(event, pnpId);
}

void PulseDeviceCollection::CheckIfVolumeChangedAndNotify(const std::string& pnpId, uint16_t volume, SoundDeviceFlowType type)
{
    if (pnpToDeviceMap_.contains(pnpId))
    {
        auto existingDevice = pnpToDeviceMap_[pnpId];
        // ReSharper disable once CppTooWideScopeInitStatement
        const auto prevVolume = existingDevice.GetCurrentRenderVolume();
        if (type == SoundDeviceFlowType::Render && prevVolume != volume)
        {
            existingDevice.SetCurrentRenderVolume(volume);
            pnpToDeviceMap_[pnpId] = existingDevice;

            NotifyObservers(SoundDeviceEventType::VolumeRenderChanged, pnpId);
        }
        else if (type == SoundDeviceFlowType::Capture && prevVolume != volume)
        {
            existingDevice.SetCurrentCaptureVolume(volume);
            pnpToDeviceMap_[pnpId] = existingDevice;

            NotifyObservers(SoundDeviceEventType::VolumeCaptureChanged, pnpId);
        }
    }
}

void PulseDeviceCollection::NotifyObservers(SoundDeviceEventType action, const std::string & devicePNpId) const
{
    for (auto* observer : observers_)
    {
        observer->OnCollectionChanged(action, devicePNpId);
    }
}

template<typename INFO_T_>
std::pair<uint16_t, std::string> PulseDeviceCollection::ExtractVolumeAndPnpId(const INFO_T_& info)
{
    const uint16_t volume = 
        (info.mute == 0) ?
        PulseDevice::NormalizeVolumeFromPulseAudioRangeToThousandBased(pa_cvolume_avg(&info.volume)) :
        0;

    std::string pnpId;
    if (const char* pnpIdPtr = pa_proplist_gets(info.proplist, "node.name");
        pnpIdPtr != nullptr)
    {
        pnpId = pnpIdPtr;
    }
    else
    {
        pnpId = info.name;
        if (constexpr auto monitorSuffix = ".monitor";
            pnpId.ends_with(monitorSuffix))
        {
            pnpId = pnpId.substr(0, pnpId.size() - std::strlen(monitorSuffix));
        }
    }

    return {volume, pnpId};
}


