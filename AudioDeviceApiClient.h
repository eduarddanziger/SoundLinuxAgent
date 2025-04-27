#pragma once

#include <cpprest/http_client.h>
#include <memory>


class HttpRequestProcessor;
class SoundDeviceInterface;


class AudioDeviceApiClient {
public:
    explicit AudioDeviceApiClient(std::shared_ptr<HttpRequestProcessor> processor);

    void PostDeviceToApi(SoundDeviceEventType eventType, const SoundDeviceInterface* device, const std::string & hintPrefix) const;
    void PutVolumeChangeToApi(const std::wstring& pnpId, bool renderOrCapture, uint16_t volume, const std::string& hintPrefix) const;

private:
    static std::string GetHostName();
    static std::wstring GetHostNameW();
private:
    std::shared_ptr<HttpRequestProcessor> requestProcessor_;
};
