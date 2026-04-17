#include "ServiceObserver.h"

#include <iostream>

#include "AudioDeviceApiClient.h"
#include "internal/StringUtils.h"

#include <spdlog/spdlog.h>
#include "magic_enum/magic_enum.hpp"

#include <fstream>
#include <string>
#include <algorithm>

#include <sys/utsname.h>

ServiceObserver::ServiceObserver(SoundDeviceCollectionInterface& collection,
                                 HttpRequestDispatcherInterface& requestProcessor
                                 )
    : collection_(collection)
    , requestProcessorInterface_(requestProcessor)
{
}

void ServiceObserver::PostDeviceToApi(const SoundDeviceEventType messageType, const SoundDeviceInterface* devicePtr, const std::string & hintPrefix) const
{
    const AudioDeviceApiClient apiClient(requestProcessorInterface_, GetHostName, GetOperationSystemName);
    apiClient.PostDeviceToApi(messageType, devicePtr, hintPrefix);
}

void ServiceObserver::PutVolumeChangeToApi(const std::string & pnpId, bool renderOrCapture, uint16_t volume, const std::string & hintPrefix) const
{
	const AudioDeviceApiClient apiClient(requestProcessorInterface_, GetHostName, GetOperationSystemName);
	apiClient.PutVolumeChangeToApi(pnpId, renderOrCapture, volume, hintPrefix);
}

void ServiceObserver::OnCollectionChanged(SoundDeviceEventType event, const std::string & devicePnpId)
{
    spdlog::info("Event caught: {}, device PnP id: {}.", magic_enum::enum_name(event), devicePnpId);

    const auto soundDeviceInterface = collection_.CreateItem(devicePnpId);
    if (!soundDeviceInterface)
    {
        spdlog::warn("Sound device with PnP id cannot be initialized.", devicePnpId);
        return;
    }

	//There is no SoundDeviceEventType::Confirmed processing. "Confirmed" is sent by collection initialization only
    if (event == SoundDeviceEventType::Discovered || event == SoundDeviceEventType::Confirmed)
    {
		const bool discoveredOrConfirmed = event == SoundDeviceEventType::Discovered;
        PostDeviceToApi(event, soundDeviceInterface.get(), discoveredOrConfirmed ? "(by device discovery) " : "(by device inventory) ");
    }
    else if (event == SoundDeviceEventType::VolumeRenderChanged || event == SoundDeviceEventType::VolumeCaptureChanged)
    {
		const bool renderOrCapture = event == SoundDeviceEventType::VolumeRenderChanged;
        PutVolumeChangeToApi(devicePnpId, renderOrCapture, renderOrCapture ? soundDeviceInterface->GetCurrentRenderVolume() : soundDeviceInterface->GetCurrentCaptureVolume());
    }
    else if (event == SoundDeviceEventType::Detached)
    {
        // not yet implemented RemoveToApi(devicePnpId);
    }
    else
	{
        spdlog::warn("Unexpected event type: {}", static_cast<int>(event));
	}

}

std::string ServiceObserver::GetHostName()
{
    // ReSharper disable once CppInconsistentNaming
    constexpr size_t MAX_COMPUTER_NAME_LENGTH = 256;
    static const std::string HOST_NAME = []() -> std::string
        {
            char hostNameBuffer[MAX_COMPUTER_NAME_LENGTH];
            if (gethostname(hostNameBuffer, MAX_COMPUTER_NAME_LENGTH) != 0) {
                spdlog::warn("Failed to get host name by gethostname.");
                strcpy(hostNameBuffer, "UNKNOWN_HOST");
            }
            else
            {
                spdlog::info("Host name obtained from gethostname: {}.", hostNameBuffer);
            }
            std::string hostName(hostNameBuffer);
            std::ranges::transform(hostName, hostName.begin(),
                [](char c) { return std::toupper(c); });
            spdlog::info("Rememberred host name: {}.", hostName);
            return hostName;
        }();
    return HOST_NAME;
}

std::string ServiceObserver::GetOperationSystemName()
{
    static const std::string OS_NAME = []() -> std::string
        {
            std::ifstream osReleaseFile("/etc/os-release");
            std::string osName, distroName, distroVersion;
            if (osReleaseFile.is_open())
            {
                std::string line;
                while (std::getline(osReleaseFile, line))
                {
                    if (constexpr auto namePrefix = "NAME="; line.find(namePrefix) == 0)
                    {
                        distroName = line.substr(std::strlen(namePrefix));
                        std::erase(distroName, '"'); // Remove quotes
                    }
                    else if (constexpr auto versionPrefix = "VERSION_ID="; line.find(versionPrefix) == 0)
                    {
                        distroVersion = line.substr(std::strlen(versionPrefix));
                        std::erase(distroVersion, '"'); // Remove quotes
                    }
                }
            }
            else
            {
                spdlog::warn("Failed to open /etc/os-release to get OS distribution info.");
            }
            if(!distroName.empty() && !distroVersion.empty())
            {
                spdlog::info("OS distribution obtained from /etc/os-release: {}, version {}.", distroName, distroVersion);
            }

            utsname uts{};
            if (uname(&uts) != 0)
            {
                spdlog::warn("Failed to get OS version by uname.");
                if (!distroName.empty() && !distroVersion.empty())
                {
                    osName = std::format("{} {}", distroName, distroVersion);
                }
                else
                {
                    osName = "Linux, no version info";
                }
            }
            else
            {
                spdlog::info("OS version obtained from uname: {}.", uts.release);
                if (!distroName.empty() && !distroVersion.empty())
                {
                    osName = std::format("{} {} {}", distroName, distroVersion, uts.release);
                }
                else
                {
                    osName = std::format("Linux, kernel {}", uts.release);
                }
            }
            spdlog::info("Rememberred OS name: {}.", osName);
            return osName;
        }();
    return OS_NAME;
}
