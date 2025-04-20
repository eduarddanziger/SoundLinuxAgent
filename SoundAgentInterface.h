#pragma once

#include <memory>

#include "ClassDefHelper.h"

class SoundDeviceCollectionInterface;
class DeviceCollectionObserver;
class SoundDeviceInterface;
class SoundDeviceObserverInterface;

enum class SoundDeviceEventType : uint8_t {
    Confirmed = 0,
    Discovered,
    Detached,
    VolumeRenderChanged,
    VolumeCaptureChanged
};

enum class SoundDeviceFlowType : uint8_t {
    None = 0,
    Render,
    Capture,
    RenderAndCapture
};

class SoundAgent final {
public:
    static std::unique_ptr<SoundDeviceCollectionInterface> CreateDeviceCollection();

    DISALLOW_COPY_MOVE(SoundAgent);
    SoundAgent() = delete;
    ~SoundAgent() = delete;
};

class SoundDeviceCollectionInterface {
public:
    [[nodiscard]] virtual std::unique_ptr<SoundDeviceInterface> CreateItem(const std::string& devicePnpId) const = 0;

	virtual void ActivateAndStartLoop() = 0;
	virtual void DeactivateAndStopLoop() = 0;

    virtual void Subscribe(SoundDeviceObserverInterface& observer) = 0;
    virtual void Unsubscribe(SoundDeviceObserverInterface& observer) = 0;

    AS_INTERFACE(SoundDeviceCollectionInterface);
    DISALLOW_COPY_MOVE(SoundDeviceCollectionInterface);
};

class SoundDeviceObserverInterface {
public:
    virtual void OnCollectionChanged(SoundDeviceEventType event, const std::string& devicePnpId) = 0;

    AS_INTERFACE(SoundDeviceObserverInterface);
    DISALLOW_COPY_MOVE(SoundDeviceObserverInterface);
};


class SoundDeviceInterface {
public:
    virtual std::string GetName() const = 0;
    virtual std::string GetPnpId() const = 0;
    virtual SoundDeviceFlowType GetFlow() const = 0;
    virtual uint16_t GetCurrentRenderVolume() const = 0; // 0 to 1000
    virtual uint16_t GetCurrentCaptureVolume() const = 0;

    AS_INTERFACE(SoundDeviceInterface);
    DISALLOW_COPY_MOVE(SoundDeviceInterface);
};
