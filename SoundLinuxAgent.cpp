#include <iostream>
#include <cstdlib>
#include <alsa/asoundlib.h>
#include <memory>
#include <stdexcept>
#include <filesystem>
#include "cpversion.h" // Include the generated version header
#include "SpdLogSetup.h" // Include the spdlog setup header
#include "MixerRaiiPtrs.h" // Include the new header file

void CheckErrorAndThrowIfAny(int err, const std::string & message)
{
    if (err < 0)
    {
        throw std::runtime_error(message + ": " + snd_strerror(err));
    }
}

#define CHECK_F(func_name, func_args) \
    CheckErrorAndThrowIfAny(func_name func_args, "Error calling " #func_name)



int main(int argc, char *argv[])
{
    // Check for version option
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--version" || std::string(argv[i]) == "-v")
        {
            std::cout << "AudioTest version " << VERSION << std::endl;
            return 0;
        }
    }

    try
    {
        // Initialize logging
        SpdLogSetup();
        
        spdlog::info("Version {}, starting...", VERSION);

        {
            constexpr auto envName = "PULSE_SERVER";
            // Check and output PULSE_SERVER environment variable
            if (const char * pulseServerName = std::getenv(envName)) // NOLINT(concurrency-mt-unsafe)
            {
                spdlog::info("{} environment variable: {}", envName, pulseServerName);
            }
            else
            {
                spdlog::info("{} environment variable is not set.", envName);
            }
        }

        MixerHandleRaiiPtr handleSmartPtr;
        {
            snd_mixer_t * handleTmp;
            CHECK_F(
                snd_mixer_open, (&handleTmp, 0)
            );
            handleSmartPtr = MixerHandleRaiiPtr(handleTmp);
        }

        CHECK_F(
            snd_mixer_attach, (handleSmartPtr.get(), "default")
        );

        CHECK_F(
            snd_mixer_selem_register, (handleSmartPtr.get(), nullptr, nullptr)
        );

        CHECK_F(
            snd_mixer_load, (handleSmartPtr.get())
        );

        // Get card information
        snd_hctl_t* hctl;
        CHECK_F(
            snd_mixer_get_hctl, (handleSmartPtr.get(), "default", &hctl)
        );
        // ReSharper disable once CppTooWideScope
        snd_ctl_t * ctl = snd_hctl_ctl(hctl);
        if (ctl)
        {
            snd_ctl_card_info_t * info;
            snd_ctl_card_info_alloca(&info); // don't need to be freed (allocated on the stack)
            if (snd_ctl_card_info(ctl, info) >= 0)
            {
                snd_ctl_card_info_get_card(info);
                spdlog::info("Card name: {}", snd_ctl_card_info_get_name(info));
                spdlog::info("Card ID: {}", snd_ctl_card_info_get_id(info));
            }
        }

        // manipulate volumes
        long minVolume, maxVolume, currentVolume;
        SimpleMixerElementIdRaiiPtr elementIdSmartPtr;
        {
            snd_mixer_selem_id_t * simpleMixerElementIdTmp;
            CHECK_F(
                snd_mixer_selem_id_malloc, (&simpleMixerElementIdTmp)
            );
            elementIdSmartPtr = SimpleMixerElementIdRaiiPtr(simpleMixerElementIdTmp);
        }

        snd_mixer_selem_id_set_index(elementIdSmartPtr.get(), 0);
        snd_mixer_selem_id_set_name(elementIdSmartPtr.get(), "Master");

        snd_mixer_elem_t * mixerElement = snd_mixer_find_selem(handleSmartPtr.get(), elementIdSmartPtr.get());
        if (!mixerElement)
        {
            throw std::runtime_error("Element not found!");
        }

        CHECK_F(
            snd_mixer_selem_get_playback_volume_range, (mixerElement, &minVolume, &maxVolume)
        );
        CHECK_F(
            snd_mixer_selem_get_playback_volume, (mixerElement, SND_MIXER_SCHN_FRONT_LEFT, &currentVolume)
        );

        spdlog::info("Current volume is: {}, between min {} and max {}.", currentVolume, minVolume, maxVolume);
        spdlog::info(
            "Setting volume to half a current volume {}. If it gets less or equal to the maximum divided by 10, namely {}, set it to half a maximum {}.",
            currentVolume / 2, maxVolume / 10, maxVolume / 2);

        long newVolume = currentVolume / 2;
        if (newVolume <= maxVolume / 10)
        {
            newVolume = maxVolume / 2;
        }

        CHECK_F(
            snd_mixer_selem_set_playback_volume_all, (mixerElement, newVolume)
        );

        spdlog::info("Volume set to {}", newVolume);
    }
    catch (const std::exception & e)
    {
        spdlog::error("...Version {}, ended with an exception: {}", VERSION, e.what());
        return 1;
    }
    spdlog::info("...Version {}, ended successfully...", VERSION);

    return 0;
}
