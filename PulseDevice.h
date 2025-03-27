#pragma once

#include <string>

#include "SoundAgentInterface.h"

class PulseDevice final : public SoundDeviceInterface  // NOLINT(clang-diagnostic-padded)
{
public:
    ~PulseDevice() override;

public:
    PulseDevice();
    PulseDevice(std::string pnpGuid, std::string name, SoundDeviceFlowType flow, uint16_t renderVolume, uint16_t captureVolume = 0);
    PulseDevice(const PulseDevice & toCopy);
    PulseDevice(PulseDevice && toMove) noexcept;
    PulseDevice & operator=(const PulseDevice & toCopy);
    PulseDevice & operator=(PulseDevice && toMove) noexcept;

public:
    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] std::string GetPnpId() const override;
    [[nodiscard]] SoundDeviceFlowType GetFlow() const override;
    [[nodiscard]] uint16_t GetCurrentRenderVolume() const override;
    [[nodiscard]] uint16_t GetCurrentCaptureVolume() const override;
    void SetCurrentRenderVolume(uint16_t volume);
    void SetCurrentCaptureVolume(uint16_t volume);

private:
    std::string pnpGuid_;
    std::string name_;
    SoundDeviceFlowType flow_;
    uint16_t renderVolume_;  // NOLINT(clang-diagnostic-padded)
    uint16_t captureVolume_;
};

