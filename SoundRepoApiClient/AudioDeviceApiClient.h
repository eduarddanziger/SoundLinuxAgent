#pragma once

#include <functional>
#include <memory>

#include "public/SoundAgentInterface.h"


class HttpRequestProcessor;
class SoundDeviceInterface;


class AudioDeviceApiClient {
public:
	AudioDeviceApiClient(std::shared_ptr<HttpRequestProcessor> processor,
	                     std::function<std::string()> getHostNameCallback,
						 std::function<std::string()> getOperationSystemNameCallback
	);

	void PostDeviceToApi(SoundDeviceEventType eventType, const SoundDeviceInterface* device,
	                     const std::string& hintPrefix) const;
	void PutVolumeChangeToApi(const std::string& pnpId, bool renderOrCapture, uint16_t volume,
	                          const std::string& hintPrefix) const;

private:
    std::shared_ptr<HttpRequestProcessor> requestProcessor_;
	std::function<std::string()> getHostNameCallback_;
	std::function<std::string()> getOperationSystemNameCallback_;
};
