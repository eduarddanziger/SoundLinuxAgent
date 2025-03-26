// ReSharper disable CppClangTidyModernizeUseNodiscard
#pragma once

#include <memory>

#include "ClassDefHelper.h"

class AudioDeviceCollectionInterface;
class DeviceCollectionObserver;
class SoundDeviceInterface;
class AudioDeviceCollectionObserverInterface;

enum class AudioDeviceCollectionEvent : uint8_t {
    None = 0,
    Discovered,
    Detached,
    VolumeChanged
};

enum class DeviceFlowEnum : uint8_t {
    None = 0,
    Render,
    Capture,
    RenderAndCapture
};

class SoundAgent {
public:
    static std::unique_ptr<AudioDeviceCollectionInterface> CreateDeviceCollection(
        const std::wstring & nameFilter, bool bothHeadsetAndMicro = false);

    DISALLOW_COPY_MOVE(SoundAgent);
    SoundAgent() = delete;
    ~SoundAgent() = delete;
};

class AudioDeviceCollectionInterface {
public:
    virtual size_t GetSize() const = 0;
    virtual std::unique_ptr<SoundDeviceInterface> CreateItem(size_t deviceNumber) const = 0;

    virtual void Subscribe(AudioDeviceCollectionObserverInterface & observer) = 0;
    virtual void Unsubscribe(AudioDeviceCollectionObserverInterface & observer) = 0;

    virtual void ResetContent() = 0;

    AS_INTERFACE(AudioDeviceCollectionInterface);
    DISALLOW_COPY_MOVE(AudioDeviceCollectionInterface);
};

class AudioDeviceCollectionObserverInterface {
public:
    virtual void OnCollectionChanged(AudioDeviceCollectionEvent event, const std::wstring & devicePnpId) = 0;
    virtual void OnTrace(const std::wstring & line) = 0;
    virtual void OnTraceDebug(const std::wstring & line) = 0;

    AS_INTERFACE(AudioDeviceCollectionObserverInterface);
    DISALLOW_COPY_MOVE(AudioDeviceCollectionObserverInterface);
};


class SoundDeviceInterface {
public:
    virtual std::wstring GetName() const = 0;
    virtual std::wstring GetPnpId() const = 0;
    virtual DeviceFlowEnum GetFlow() const = 0;
    virtual uint16_t GetCurrentRenderVolume() const = 0;
    virtual uint16_t GetCurrentCaptureVolume() const = 0;

    AS_INTERFACE(SoundDeviceInterface);
    DISALLOW_COPY_MOVE(SoundDeviceInterface);
};
