#include "../../public/SoundAgentInterface.h"

#include <stdexcept>

#include "PulseDeviceCollection.h"


std::unique_ptr<SoundDeviceCollectionInterface> SoundAgent::CreateDeviceCollection()
{
    return std::make_unique<PulseDeviceCollection>();
}
