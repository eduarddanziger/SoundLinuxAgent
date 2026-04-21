#include "../SoundLibRuntimeSettings.h"

#include <atomic>

namespace
{
    std::atomic<bool> pulseAudioReconnectionEnabled{false};
    std::atomic<uint32_t> pulseAudioInitialReconnectDelayMs{1000};
}

void SoundLibRuntimeSettings::SetPulseAudioReconnectionEnabled(const bool value)
{
    pulseAudioReconnectionEnabled.store(value);
}

bool SoundLibRuntimeSettings::GetPulseAudioReconnectionEnabled()
{
    return pulseAudioReconnectionEnabled.load();
}

void SoundLibRuntimeSettings::SetPulseAudioInitialReconnectDelayMs(const uint32_t value)
{
    pulseAudioInitialReconnectDelayMs.store(value);
}

uint32_t SoundLibRuntimeSettings::GetPulseAudioInitialReconnectDelayMs()
{
    return pulseAudioInitialReconnectDelayMs.load();
}
