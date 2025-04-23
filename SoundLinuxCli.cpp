#include "SpdLogSetup.h"
#include "cpversion.h"

#include "AgentObserver.h"

#include "KeyInputThread.h"

#include <memory>
#include <thread>
#include <iostream>


int main(int argc, char *argv[])
{
    // Check for version option
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--version" || std::string(argv[i]) == "-v")
        {
            std::cout << "AudioTest version " << VERSION << "\n";
            return 0;
        }
    }

    try
    {
        // Initialize logging
        SpdLogSetup();
        
        spdlog::info("Version {}, starting...", VERSION);

		const auto collSmartPtr = SoundAgent::CreateDeviceCollection();
        if (collSmartPtr == nullptr)
        {
			throw std::runtime_error("Failed to create device collection");
        }
		auto& collection = *collSmartPtr;

        AgentObserver subscriber(collection);

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
