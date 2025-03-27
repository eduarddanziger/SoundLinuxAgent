#include "SoundAgentInterface.h"

#include <stdexcept>

//#include "SoundDeviceCollection.h"


std::unique_ptr<SoundDeviceCollectionInterface> SoundAgent::CreateDeviceCollection(const std::string& nameFilter, bool bothHeadsetAndMicro)
{
//    return std::make_unique<ed::audio::SoundDeviceCollection>(nameFilter, bothHeadsetAndMicro);
	return nullptr;
}
