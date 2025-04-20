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
#include "magic_enum/magic_enum_iostream.hpp"


using namespace std::literals;

class ConsoleSubscriber final : public SoundDeviceObserverInterface
{
    public:
        void OnCollectionChanged(SoundDeviceEventType event, const std::string& devicePnpId) override
        {
            using magic_enum::iostream_operators::operator<<; // out-of-the-box stream operators for enums

            std::cout << '\n' << "Event caught: " << event << "."
                << " Device PnP id: " << devicePnpId << '\n';
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

		ConsoleSubscriber subscriber;

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
