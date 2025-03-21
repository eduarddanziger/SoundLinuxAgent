#pragma once

#include <alsa/asoundlib.h>
#include <memory>

// Custom deleter for snd_mixer_t
struct MixerHandleDeleter {
    void operator()(snd_mixer_t * handle) const
    {
        if (handle)
        {
            snd_mixer_close(handle);
        }
    }
};

using MixerHandleRaiiPtr = std::unique_ptr<snd_mixer_t, MixerHandleDeleter>;

struct SimpleMixerElementDeleter {
    void operator()(snd_mixer_selem_id_t * handle) const
    {
        if (handle)
        {
            snd_mixer_selem_id_free(handle);
        }
    }
};

using SimpleMixerElementIdRaiiPtr = std::unique_ptr<snd_mixer_selem_id_t, SimpleMixerElementDeleter>;