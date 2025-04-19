#pragma once

#include <string>

#include "SoundAgentInterface.h"

class PulseDevice final : public SoundDeviceInterface  // NOLINT(clang-diagnostic-padded)
{
public:
    ~PulseDevice() override;

public:
    PulseDevice();
    PulseDevice(std::string pnpGuid, std::string name, SoundDeviceFlowType flow, uint16_t renderVolume, uint16_t captureVolume);
    PulseDevice(const PulseDevice & toCopy);
    PulseDevice(PulseDevice && toMove) noexcept;
    PulseDevice & operator=(const PulseDevice & toCopy);
    PulseDevice & operator=(PulseDevice && toMove) noexcept;

public:
    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] std::string GetPnpId() const override;
    [[nodiscard]] SoundDeviceFlowType GetFlow() const override;
    [[nodiscard]] uint16_t GetCurrentRenderVolume() const override; // 0 to 1000
    [[nodiscard]] uint16_t GetCurrentCaptureVolume() const override; // 0 to 1000
    void SetCurrentRenderVolume(uint16_t volume); // 0 to 1000
    void SetCurrentCaptureVolume(uint16_t volume); // 0 to 1000

    static uint16_t NormalizeVolumeFromPulseAudioRangeToThousandBased(uint32_t pulseAudioVolume);

private:
    std::string pnpGuid_;
    std::string name_;
    SoundDeviceFlowType flow_;
    uint16_t renderVolume_;  // NOLINT(clang-diagnostic-padded) // 0 to 1000
    uint16_t captureVolume_;                                    // 0 to 1000
};

