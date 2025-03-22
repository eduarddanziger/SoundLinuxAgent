#include <iostream>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <filesystem>

#include "SpdLogSetup.h" // Include the spdlog setup header
#include "cpversion.h" // generated version header


#include "AudioDeviceCollection.h"

class ConsoleSubscriber : public IDeviceSubscriber {
    public:
        void onDeviceEvent(const DeviceEvent& event) override {
            const char* typeStr = "";
            switch(event.type) {
                case DeviceEventType::Added: typeStr = "Added"; break;
                case DeviceEventType::Removed: typeStr = "Removed"; break;
                case DeviceEventType::VolumeChanged: typeStr = "VolumeChanged"; break;
            }
            std::cout << "Device event: " << typeStr << " - " << event.device.name << "\n";
        }
};


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

        AudioDeviceCollection collection;
        auto subscriber = std::make_shared<ConsoleSubscriber>();
        
        collection.subscribe(subscriber);
        collection.startMonitoring();

        // Run main loop
        while(true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    }
    catch (const std::exception & e)
    {
        spdlog::error("...Version {}, ended with an exception: {}", VERSION, e.what());
        return 1;
    }
    spdlog::info("...Version {}, ended successfully...", VERSION);

    return 0;
}
