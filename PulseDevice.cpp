#include "PulseDevice.h"

#include <pulse/volume.h>


PulseDevice::~PulseDevice() = default;

PulseDevice::PulseDevice()
    : PulseDevice("", "", SoundDeviceFlowType::None, 0, 0)
{
}

// ReSharper disable once CppParameterMayBeConst
PulseDevice::PulseDevice(std::string pnpGuid, std::string name, SoundDeviceFlowType flow, uint16_t renderVolume,
                          uint16_t captureVolume)
    : pnpGuid_(std::move(pnpGuid))
      , name_(std::move(name))
      , flow_(flow)
      , renderVolume_(renderVolume)
      , captureVolume_(captureVolume)
{
}

PulseDevice::PulseDevice(const PulseDevice & toCopy)
    : pnpGuid_(toCopy.pnpGuid_)
      , name_(toCopy.name_)
      , flow_(toCopy.flow_)
      , renderVolume_(toCopy.renderVolume_)
      , captureVolume_(toCopy.captureVolume_)
{
}

PulseDevice::PulseDevice(PulseDevice && toMove) noexcept
    : pnpGuid_(std::move(toMove.pnpGuid_))
      , name_(std::move(toMove.name_))
      , flow_(toMove.flow_)
      , renderVolume_(toMove.renderVolume_)
      , captureVolume_(toMove.captureVolume_)
{
}

PulseDevice & PulseDevice::operator=(const PulseDevice & toCopy)
{
    if (this != &toCopy)
    {
        pnpGuid_ = toCopy.pnpGuid_;
        name_ = toCopy.name_;
        flow_ = toCopy.flow_;
        renderVolume_ = toCopy.renderVolume_;
        captureVolume_ = toCopy.captureVolume_;
    }
    return *this;
}

PulseDevice & PulseDevice::operator=(PulseDevice && toMove) noexcept
{
    if (this != &toMove)
    {
        pnpGuid_ = std::move(toMove.pnpGuid_);
        name_ = std::move(toMove.name_);
        flow_ = toMove.flow_;
        renderVolume_ = toMove.renderVolume_;
        captureVolume_ = toMove.captureVolume_;
    }
    return *this;
}

std::string PulseDevice::GetName() const
{
    return name_;
}

std::string PulseDevice::GetPnpId() const
{
    return pnpGuid_;
}

SoundDeviceFlowType PulseDevice::GetFlow() const
{
    return flow_;
}

uint16_t PulseDevice::GetCurrentRenderVolume() const
{
    return renderVolume_;
}

uint16_t PulseDevice::GetCurrentCaptureVolume() const
{
    return captureVolume_;
}

void PulseDevice::SetCurrentRenderVolume(uint16_t volume)
{
    renderVolume_ = volume;
}

void PulseDevice::SetCurrentCaptureVolume(uint16_t volume)
{
    captureVolume_ = volume;
}

uint16_t PulseDevice::NormalizeVolumeFromPulseAudioRangeToThousandBased(uint32_t pulseAudioVolume)
{
	uint16_t normalizedVolume = 0;
	if (pulseAudioVolume > 0)
	{
        normalizedVolume = static_cast<uint16_t>(
                (pulseAudioVolume - PA_VOLUME_MUTED) * 1000 / (PA_VOLUME_NORM - PA_VOLUME_MUTED)
            );
	}
	return normalizedVolume;
}

