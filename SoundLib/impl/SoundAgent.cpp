#include "../../public/SoundAgentInterface.h"

#include "PulseDeviceCollection.h"


std::unique_ptr<SoundDeviceCollectionInterface> SoundAgent::CreateDeviceCollection()
{
    return std::make_unique<PulseDeviceCollection>();
}
