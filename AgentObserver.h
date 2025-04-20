#pragma once

#include "SoundAgentInterface.h"

#include <iostream>

#include "magic_enum/magic_enum.hpp"

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
    }
private:
    SoundDeviceCollectionInterface& collection_;
};

