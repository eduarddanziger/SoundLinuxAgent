#include "SpdLogSetup.h"
#include "cpversion.h"
#include "PulseDeviceCollection.h"
#include "KeyInputThread.h"

#include <pulse/pulseaudio.h>

#include <iostream>
#include <memory>
#include <filesystem>
#include <glib.h>
#include <thread>

using namespace std::literals;

class ConsoleSubscriber final : public IDeviceSubscriber {
    public:
        void OnDeviceEvent(const DeviceEvent& event) override {
            // ReSharper disable once CppUseAuto
            auto eventTypeAsString = ""s;
            switch(event.type) {
                case DeviceEventType::Added: eventTypeAsString = "Added"; break;
                case DeviceEventType::Removed: eventTypeAsString = "Removed"; break;
                case DeviceEventType::VolumeChanged: eventTypeAsString = "VolumeChanged"; break;
            }
            auto deviceTypeAsString = ""s;
            switch(event.device.GetFlow()) {
                case SoundDeviceFlowType::Capture: deviceTypeAsString = "Capture"; break;
                case SoundDeviceFlowType::Render: deviceTypeAsString = "Render"; break;
            default: ;
                deviceTypeAsString = "Undefined";
            }
            std::cout << "Got Event (" << eventTypeAsString << "): " << "\nid: " <<
                event.device.GetPnpId() << "\n""type: " << deviceTypeAsString <<
                "\n""name: " << event.device.GetName() << "\n""render volume: " << event.device.GetCurrentRenderVolume() <<
                "\n""capture volume: " << event.device.GetCurrentCaptureVolume() << ".\n";
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

        PulseDeviceCollection collection;

        const auto subscriber = std::make_shared<ConsoleSubscriber>();
        collection.Subscribe(subscriber);

        // Start the key input thread
        std::thread inputThread([&]() {
            keyInputThread(
                [&collection](char input) {  // Quit callback (executes when 'q' is pressed)
                    if (input == 'q' || input == 'Q') {
                        std::cout << "Quitting...\n";
                        collection.DeactivateAndStopLoop();
                        return true;
                    }
                    return false;
                },
                []() {  // Iteration callback (here called every 25 seconds)
                    std::cout << "Press 'q' and Enter to quit\n";
                },
                25
            );
        });

        // Activate and run the main loop
        collection.ActivateAndStartLoop();

        if (inputThread.joinable()) {
            inputThread.join();
        }

        collection.Unsubscribe(subscriber);
        
        spdlog::info("Main loop exited. Shutting down...");
    }
    catch (const std::exception & e)
    {
        spdlog::error("...Version {}, ended with an exception: {}", VERSION, e.what());
        return 1;
    }
    spdlog::info("...Version {}, ended successfully...", VERSION);

    return 0;
}
