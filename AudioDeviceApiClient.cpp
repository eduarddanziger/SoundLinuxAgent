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
AudioDeviceApiClient::AudioDeviceApiClient(std::shared_ptr<HttpRequestProcessor> processor)  // NOLINT(performance-unnecessary-value-param, modernize-pass-by-value)
    : requestProcessor_(processor)  // NOLINT(performance-unnecessary-value-param)
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

    const std::string hostName = GetHostName();

	auto systemTimeAsString = ed::getSystemTimeAsString("T");
	systemTimeAsString = systemTimeAsString.substr(0, systemTimeAsString.length() - 7);

    const nlohmann::json payload = {
        {"pnpId", device->GetPnpId()},
        {"name", device->GetName()},
        {"flowType", device->GetFlow()},
        {"renderVolume", device->GetCurrentRenderVolume()},
        {"captureVolume", device->GetCurrentCaptureVolume()},
        {"updateDate", systemTimeAsString},
        {"deviceMessageType", eventType},
        {"hostName", hostName}
    };

    // Convert nlohmann::json to string and to value
    const std::string payloadString = payload.dump();
    const web::json::value jsonPayload = web::json::value::parse(payloadString);

    web::http::http_request request(web::http::methods::POST);
    request.set_body(jsonPayload);
    request.headers().set_content_type(U("application/json"));

    const auto hint = hintPrefix + "Post a device: " + device->GetPnpId();
    SPD_L->info("Enqueueing: {}...", hint);
    requestProcessor_->EnqueueRequest(request, "", payloadString, hint);
    FormattedOutput::LogAndPrint("Enqueued: " + hint);
}

void AudioDeviceApiClient::PutVolumeChangeToApi(const std::string & pnpId, bool renderOrCapture, uint16_t volume, const std::string& hintPrefix) const
{
	auto systemTimeAsString = ed::getSystemTimeAsString("T");
	systemTimeAsString = systemTimeAsString.substr(0, systemTimeAsString.length() - 7);
	const nlohmann::json payload = {
        {"deviceMessageType", renderOrCapture ? SoundDeviceEventType::VolumeRenderChanged : SoundDeviceEventType::VolumeCaptureChanged},
        {"volume", volume},
        {"updateDate", systemTimeAsString}
	};
    // Convert nlohmann::json to string and to value
    const std::string payloadString = payload.dump();
    const web::json::value jsonPayload = web::json::value::parse(payloadString);

    web::http::http_request request(web::http::methods::PUT);
	request.set_body(jsonPayload);
	request.headers().set_content_type(U("application/json"));

    const auto hint = hintPrefix + "Volume change (PUT) for a device: " + pnpId;
    SPD_L->info("Enqueueing: {}...", hint);
	// Instead of sending directly, enqueue the request in the processor

    const auto urlSuffix = std::format("/{}/{}", pnpId, GetHostName());
    requestProcessor_->EnqueueRequest(request, urlSuffix, payloadString, hint);
    FormattedOutput::LogAndPrint("Enqueued: " + hint);
}

std::string AudioDeviceApiClient::GetHostName()
{
    // ReSharper disable once CppInconsistentNaming
    constexpr size_t MAX_COMPUTER_NAME_LENGTH = 256;
    static const std::string HOST_NAME = []() -> std::string
        {
            char hostNameBuffer[MAX_COMPUTER_NAME_LENGTH];
            if (gethostname(hostNameBuffer, MAX_COMPUTER_NAME_LENGTH) != 0) {
                return "UNKNOWN_HOST";
            }
            std::string hostName(hostNameBuffer);
            std::ranges::transform(hostName, hostName.begin(),
                [](char c) { return std::toupper(c); });
            return hostName;
        }();
    return HOST_NAME;
}

