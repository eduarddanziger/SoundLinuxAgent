#pragma once
#include <string>
#include <pulse/pulseaudio.h>

enum class DeviceType { Render, Capture, RenderAndCapture };

struct AudioDevice {
    std::string pnpId;
    std::string name;
    DeviceType type;
    uint32_t index;
    uint32_t volume;
    
    AudioDevice(std::string id, std::string n, DeviceType t, uint32_t idx, uint32_t vol)
        : pnpId(std::move(id)), name(std::move(n)), type(t), index(idx), volume(vol) {}
    AudioDevice() : pnpId(""), name(""), type(DeviceType::Render), index(0), volume(0) {}
};