﻿// ReSharper disable CppClangTidyModernizeUseNodiscard
#pragma once

#include <memory>

#include "ClassDefHelper.h"

class SoundDeviceCollectionInterface;
class SoundDeviceInterface;
class SoundDeviceObserverInterface;

enum class SoundDeviceEventType : uint8_t {
    None = 0,
    Discovered,
    Detached,
    VolumeChanged
};

enum class SoundDeviceFlowType : uint8_t {
    None = 0,
    Render,
    Capture,
    RenderAndCapture
};

class SoundAgent final {
public:
    static std::unique_ptr<SoundDeviceCollectionInterface> CreateDeviceCollection(
        const std::string & nameFilter, bool bothHeadsetAndMicro = false);

    DISALLOW_COPY_MOVE(SoundAgent);
    SoundAgent() = delete;
    ~SoundAgent() = delete;
};

class SoundDeviceCollectionInterface {
public:
    virtual size_t GetSize() const = 0;
    virtual std::unique_ptr<SoundDeviceInterface> CreateItem(size_t deviceNumber) const = 0;

    virtual void Subscribe(SoundDeviceObserverInterface & observer) = 0;
    virtual void Unsubscribe(SoundDeviceObserverInterface & observer) = 0;

    virtual void ResetContent() = 0;

    AS_INTERFACE(SoundDeviceCollectionInterface);
    DISALLOW_COPY_MOVE(SoundDeviceCollectionInterface);
};

class SoundDeviceObserverInterface {
public:
    virtual void OnCollectionChanged(SoundDeviceEventType event, const std::string & devicePnpId) = 0;
    virtual void OnTrace(const std::string & line) = 0;
    virtual void OnTraceDebug(const std::string & line) = 0;

    AS_INTERFACE(SoundDeviceObserverInterface);
    DISALLOW_COPY_MOVE(SoundDeviceObserverInterface);
};


class SoundDeviceInterface {
public:
    virtual std::string GetName() const = 0;
    virtual std::string GetPnpId() const = 0;
    virtual SoundDeviceFlowType GetFlow() const = 0;
    virtual uint16_t GetCurrentRenderVolume() const = 0;
    virtual uint16_t GetCurrentCaptureVolume() const = 0;

    AS_INTERFACE(SoundDeviceInterface);
    DISALLOW_COPY_MOVE(SoundDeviceInterface);
};
