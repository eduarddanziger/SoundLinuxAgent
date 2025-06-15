#pragma once

#include "public/SoundAgentInterface.h"

class HttpRequestProcessor;

class ServiceObserver final : public SoundDeviceObserverInterface {
public:
    ServiceObserver(SoundDeviceCollectionInterface& collection,
        std::string apiBaseUrl,
        std::string universalToken,
        std::string codespaceName); // Added codespaceName parameter

    void PostDeviceToApi(SoundDeviceEventType messageType, const SoundDeviceInterface* devicePtr, const std::string & hintPrefix= "") const;
    void PutVolumeChangeToApi(const std::string & pnpId, bool renderOrCapture, uint16_t volume, const std::string & hintPrefix= "") const;

    DISALLOW_COPY_MOVE(ServiceObserver);
    ~ServiceObserver() override = default;

public:
    void OnCollectionChanged(SoundDeviceEventType event, const std::string & devicePnpId) override;

private:
    static std::string GetHostName();
    static std::string GetOperationSystemName();
private:
    SoundDeviceCollectionInterface& collection_;
    std::string apiBaseUrl_;
    std::string universalToken_;
    std::string codespaceName_; // Newly added member for codespaceName
    std::shared_ptr<HttpRequestProcessor> requestProcessorSmartPtr_;
};
