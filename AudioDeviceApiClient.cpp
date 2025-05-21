#include "AudioDeviceApiClient.h"

#include <iostream>
#include <SoundAgentInterface.h>

#include <SpdLogger.h>

#include <string>
#include <sstream>
#include <nlohmann/json.hpp>

#include "TimeUtils.h"

#include "FormattedOutput.h"
#include "HttpRequestProcessor.h"


// ReSharper disable once CppPassValueParameterByConstReference
AudioDeviceApiClient::AudioDeviceApiClient(std::shared_ptr<HttpRequestProcessor> processor, std::function<std::string()> getHostNameCallback)
    : requestProcessor_(processor)  // NOLINT(performance-unnecessary-value-param)
	, getHostNameCallback_(std::move(getHostNameCallback))
{
}

void AudioDeviceApiClient::PostDeviceToApi(SoundDeviceEventType eventType, const SoundDeviceInterface* device, const std::string& hintPrefix) const
{
    if (!device)
    {
        const auto msg = "Cannot post device data: nullptr provided";
        std::cout << FormattedOutput::CurrentLocalTimeWithoutDate << msg << '\n';
        SPD_L->error(msg);
        return;
    }

    const std::string hostName = getHostNameCallback_();

    const auto nowTime = std::chrono::system_clock::now();
    const auto nowTimeAsSystemTimeString = ed::systemTimeAsStringWithSystemTime(nowTime, "T") + "Z";

    const nlohmann::json payload = {
        {"pnpId", device->GetPnpId()},
        {"name", device->GetName()},
        {"flowType", device->GetFlow()},
        {"renderVolume", device->GetCurrentRenderVolume()},
        {"captureVolume", device->GetCurrentCaptureVolume()},
        {"updateDate", nowTimeAsSystemTimeString},
        {"deviceMessageType", eventType},
        {"hostName", hostName}
    };

    // Convert nlohmann::json to string and to value
    const std::string payloadString = payload.dump();
    const auto hint = hintPrefix + "Post a device: " + device->GetPnpId();

    SPD_L->info("Enqueueing: {}...", hint);
    requestProcessor_->EnqueueRequest(true, nowTime, "", payloadString, {}, hint);
}

void AudioDeviceApiClient::PutVolumeChangeToApi(const std::string & pnpId, bool renderOrCapture, uint16_t volume, const std::string& hintPrefix) const
{
    const auto nowTime = std::chrono::system_clock::now();
    const auto nowTimeAsSystemTimeString = ed::systemTimeAsStringWithSystemTime(nowTime, "T") + "Z";

    const nlohmann::json payload = {
        {"deviceMessageType", renderOrCapture ? SoundDeviceEventType::VolumeRenderChanged : SoundDeviceEventType::VolumeCaptureChanged},
        {"volume", volume},
        {"updateDate", nowTimeAsSystemTimeString}
	};
    const std::string payloadString = payload.dump();

    const auto hint = hintPrefix + "Volume change (PUT) for a device: " + pnpId;
    SPD_L->info("Enqueueing: {}...", hint);
	// Instead of sending directly, enqueue the request in the processor

    const auto urlSuffix = std::format("/{}/{}", pnpId, getHostNameCallback_());
    requestProcessor_->EnqueueRequest(false, nowTime, urlSuffix, payloadString, {}, hint);
}

