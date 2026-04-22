#pragma once

#include "ClassDefHelper.h"

#include <cstdint>

class SoundLibRuntimeSettings final
{
public:
    static void SetPulseAudioReconnectionEnabled(bool value);
    [[nodiscard]] static bool GetPulseAudioReconnectionEnabled();

    static void SetPulseAudioInitialReconnectDelayMs(uint32_t value);
    [[nodiscard]] static uint32_t GetPulseAudioInitialReconnectDelayMs();

    DISALLOW_IMPLICIT_CONSTRUCTORS(SoundLibRuntimeSettings);
};
