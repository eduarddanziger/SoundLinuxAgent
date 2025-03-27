#include "PulseDevice.h"

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

