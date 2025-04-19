#include "PulseDeviceCollection.h"

#include "ScopeLogger.h"


#include <pulse/subscribe.h>
#include <pulse/glib-mainloop.h>
#include <pulse/proplist.h>
#include <iostream>

PulseDeviceCollection::PulseDeviceCollection()
    : mainloop_(nullptr)
    , context_(nullptr)
{
    LOG_SCOPE();
    gMainLoop_ = g_main_loop_new(nullptr, FALSE);
    mainloop_ = pa_glib_mainloop_new(g_main_loop_get_context(gMainLoop_));
    context_ = pa_context_new(pa_glib_mainloop_get_api(mainloop_), "DeviceMonitor");
}

PulseDeviceCollection::~PulseDeviceCollection() {
    LOG_SCOPE();
    if(context_) {
        pa_context_unref(context_);
    }
    if(mainloop_) pa_glib_mainloop_free(mainloop_);
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
        spdlog::error("Failed to startt monitoring PulseAudio events");
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

void PulseDeviceCollection::GetServerInfo() {
    spdlog::info("SERVER: Requesting info...");
    pa_operation* op = pa_context_get_server_info(context_, ServerInfoCallback, this);
    pa_operation_unref(op);
}

void PulseDeviceCollection::ContextStateCallback(pa_context* c, void* userdata) {
    auto* self = static_cast<PulseDeviceCollection*>(userdata);
    pa_context_state_t state = pa_context_get_state(c);
    
    switch (state) {
        case PA_CONTEXT_READY:
            spdlog::info("PulseAudio context got READY status, state: {}", state);
            self->GetServerInfo();
            self->StartMonitoring();
            break;
            
        case PA_CONTEXT_FAILED:
            spdlog::error(
                "PulseAudio context got FAILED status (state {}): {}"
              , state, pa_strerror(pa_context_errno(c))
            );
            break;
            
        case PA_CONTEXT_TERMINATED:
            spdlog::info("PulseAudio context got TERMINATED status, state: {}", state);
            break;
            
        default:
            // Still connecting or other states
            spdlog::info("PulseAudio context's state: {}", state);
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
        // if (operation == PA_SUBSCRIPTION_EVENT_NEW) 
        if (operation == PA_SUBSCRIPTION_EVENT_REMOVE) {
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
        else if (operation == PA_SUBSCRIPTION_EVENT_CHANGE) {
            spdlog::info("SINC index {}: Volume changed...", idx);
            pa_operation* op = pa_context_get_sink_info_by_index(c, idx, SinkInfoCallback, self);
            pa_operation_unref(op);
        }
        else {
            spdlog::info("SINK index {}: Adding or updating...", idx);
            pa_operation* op = pa_context_get_sink_info_by_index(c, idx, SinkInfoCallback, self);
            pa_operation_unref(op);
        }
    }
    else if (facility == PA_SUBSCRIPTION_EVENT_SOURCE) {
        // if (operation == PA_SUBSCRIPTION_EVENT_NEW) 
        if (operation == PA_SUBSCRIPTION_EVENT_REMOVE) {
            spdlog::info("SOURCE index {}: Removing...", idx);
        }
        else if (operation == PA_SUBSCRIPTION_EVENT_CHANGE) {
            spdlog::info("SOURCE index {}: Volume changed...", idx);
            pa_operation* op = pa_context_get_source_info_by_index(c, idx, SourceInfoCallback, self);
            pa_operation_unref(op);
        }
        else {
            spdlog::info("SOURCE index {}: Adding or updating...", idx);
            pa_operation* op = pa_context_get_source_info_by_index(c, idx, SourceInfoCallback, self);
            pa_operation_unref(op);
        }
    }
}
void PulseDeviceCollection::ServerInfoCallback(pa_context* c, const pa_server_info* info, void* userdata)
{
    LOG_SCOPE();

    auto* self = static_cast<PulseDeviceCollection*>(userdata);
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

void PulseDeviceCollection::AddOrUpdateAndNotify(const std::string& pnpId, const std::string& name, uint32_t volume, SoundDeviceFlowType type, uint32_t index)
{
    // Add or update the sink in the device collection
    const PulseDevice device(pnpId, name, type, volume, 0);

    pnpToDeviceMap_[pnpId] = device;

    // Notify subscribers about the new/updated device
    NotifyObservers(SoundDeviceEventType::Confirmed, pnpId);
}

void PulseDeviceCollection::NotifyObservers(SoundDeviceEventType action, const std::string & devicePNpId) const
{
    for (auto* observer : observers_)
    {
        observer->OnCollectionChanged(action, devicePNpId);
    }
}

void PulseDeviceCollection::SinkInfoCallback(pa_context* context, const pa_sink_info* info, int eol, void* userdata) {
    LOG_SCOPE();
    auto* self = static_cast<PulseDeviceCollection*>(userdata);

    if (eol) {
        return;
    }

    if (!info) {
        std::cerr << "Failed to get sink info." << std::endl;
        return;
    }

    spdlog::info("Found sink '{}' with index {}.", info->name, info->index);

    const std::string deviceName = info->description;
    const uint32_t volumePulseAudio = pa_cvolume_avg(&info->volume);
	const uint16_t volume = PulseDevice::NormalizeVolumeFromPulseAudioRangeToThousandBased(volumePulseAudio);
    constexpr auto type = SoundDeviceFlowType::Render;
    const uint32_t index = info->index;

    std::string pnpId;
    const char* pnpIdPtr = pa_proplist_gets(info->proplist, "device.bus_path");
    if (pnpIdPtr != nullptr) {
        spdlog::info("device.bus_path property found, use it as a PnP ID");
        pnpId = pnpIdPtr;
    }
    else
    {
        spdlog::info("device.bus_path property not found, use the name as a PnP ID");
        pnpId = info->name;
    }

    self->AddOrUpdateAndNotify(pnpId, deviceName, volume, type, index);
}

void PulseDeviceCollection::SourceInfoCallback(pa_context* context, const pa_source_info* info, int eol, void* userdata) {
    LOG_SCOPE();
    auto* self = static_cast<PulseDeviceCollection*>(userdata);

    if (eol) {
        return;
    }

    if (!info) {
        std::cerr << "Failed to get source info." << std::endl;
        return;
    }

    spdlog::info("Found source '{}' with index {}.", info->name, info->index);

    const std::string deviceName = info->description;
    const uint32_t volume = pa_cvolume_avg(&info->volume);
    constexpr auto type = SoundDeviceFlowType::Capture;
    const uint32_t index = info->index;

    std::string pnpId;
    const char* pnpIdPtr = pa_proplist_gets(info->proplist, "device.bus_path");
    if (pnpIdPtr != nullptr) {
        spdlog::info("device.bus_path property found, use it as a PnP ID");
        pnpId = pnpIdPtr;
    }
    else
    {
        spdlog::info("device.bus_path property not found, use the name as a PnP ID");
        pnpId = info->name;
    }

    self->AddOrUpdateAndNotify(pnpId, deviceName, volume, type, index);
}

