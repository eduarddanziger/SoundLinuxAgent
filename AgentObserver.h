#pragma once

#include "SoundAgentInterface.h"

#include <iostream>

#include "magic_enum/magic_enum_iostream.hpp"


class AgentObserver final : public SoundDeviceObserverInterface  // NOLINT(clang-diagnostic-weak-vtables)
{
public:
    explicit AgentObserver(SoundDeviceCollectionInterface& collection)
        :collection_(collection)
    {
        
    }
    void OnCollectionChanged(SoundDeviceEventType event, const std::string& devicePnpId) override
    {
        spdlog::info("Event \"{}\" caught, device PnP ID: {}.\n", magic_enum::enum_name(event), devicePnpId);
		if (event != SoundDeviceEventType::Detached)
		{
			const auto device = collection_.CreateItem(devicePnpId);
			if (device)
			{
				PrintDeviceInfo(device.get());
			}
			else
			{
				spdlog::warn("Failed to create device for PnP ID: {}", devicePnpId);
			}
		}
    }

    static void PrintDeviceInfo(const SoundDeviceInterface* device)
    {
        using magic_enum::iostream_operators::operator<<;

        const auto idString = device->GetPnpId();
        std::ostringstream os; os << "Device "
            << idString
            << ": \"" << device->GetName()
            << "\", " << device->GetFlow()
            << ", Volume " << device->GetCurrentRenderVolume()
            << " / " << device->GetCurrentCaptureVolume();
		spdlog::info(os.str());
    }

private:
    SoundDeviceCollectionInterface& collection_;
};

