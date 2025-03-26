#include "SoundAgentInterface.h"

#include <stdexcept>

//#include "SoundDeviceCollection.h"


std::unique_ptr<AudioDeviceCollectionInterface> SoundAgent::CreateDeviceCollection(const std::wstring& nameFilter, bool bothHeadsetAndMicro)
{
//    return std::make_unique<ed::audio::SoundDeviceCollection>(nameFilter, bothHeadsetAndMicro);
	return nullptr;
}
