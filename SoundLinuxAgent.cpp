#include <iostream>
#include <memory>
#include <filesystem>
#include <glib.h>
#include <thread>
#include <pulse/pulseaudio.h>


#include "SpdLogSetup.h" // Include the spdlog setup header
#include "cpversion.h" // generated version header
#include "PulseDeviceCollection.h"
#include "KeyInputThread.h" // Add this include for the keyInputThread function

class ConsoleSubscriber final : public IDeviceSubscriber {
    public:
        void OnDeviceEvent(const DeviceEvent& event) override {
            // ReSharper disable once CppUseAuto
            const char* eventTypeAsString = "";
            switch(event.type) {
                case DeviceEventType::Added: eventTypeAsString = "Added"; break;
                case DeviceEventType::Removed: eventTypeAsString = "Removed"; break;
                case DeviceEventType::VolumeChanged: eventTypeAsString = "VolumeChanged"; break;
            }
            const char* deviceTypeAsString = "";
            switch(event.device.type) {
                case DeviceType::Capture: deviceTypeAsString = "Capture"; break;
                case DeviceType::Render: deviceTypeAsString = "Render"; break;
            }
            std::cout << "Got Event (" << eventTypeAsString << "): " << "\nid: " <<
                event.device.pnpId << "\ntype: " << deviceTypeAsString <<
                "\nname: " << event.device.name << "\nqvolume: " << event.device.volume << ".\n";
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

        collection.Activate();
        auto* loop = collection.GetMainLoop();

        // Start the key input thread
        std::thread inputThread([&]() {
            keyInputThread(
                [&collection, loop](char input) {  // Quit callback (executes when 'q' is pressed)
                    if (input == 'q' || input == 'Q') {
                        std::cout << "Quitting...\n";
                        collection.Deactivate();
                        g_main_loop_quit(loop);
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

        // Run the main loop
        g_main_loop_run(loop);

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
