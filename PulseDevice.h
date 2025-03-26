#pragma once
#include <string>

enum class DeviceType { Render, Capture, RenderAndCapture };

struct PulseDevice {
    std::string pnpId;
    std::string name;
    DeviceType type;
    uint32_t index;
    uint32_t volume;
    
    PulseDevice(std::string id, std::string n, DeviceType t, uint32_t idx, uint32_t vol)
        : pnpId(std::move(id)), name(std::move(n)), type(t), index(idx), volume(vol) {}
    PulseDevice() : pnpId(""), name(""), type(DeviceType::Render), index(0), volume(0) {}
};