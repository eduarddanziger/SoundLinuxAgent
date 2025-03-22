#pragma once
#include <string>
#include <pulse/pulseaudio.h>

enum class DeviceType { Render, Capture, RenderAndCapture };

struct AudioDevice {
    std::string pnpId;
    std::string name;
    DeviceType type;
    uint32_t index;
    pa_cvolume volume;
    
    AudioDevice(const std::string& id, const std::string& n, DeviceType t, uint32_t idx)
        : pnpId(id), name(n), type(t), index(idx) {}
    AudioDevice() : pnpId(""), name(""), type(DeviceType::Render), index(0) {}
};