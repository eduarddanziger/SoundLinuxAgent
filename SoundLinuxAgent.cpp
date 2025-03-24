#include <iostream>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <filesystem>
#include <glib.h>
#include <thread>
#include <atomic>

#include "SpdLogSetup.h" // Include the spdlog setup header
#include "cpversion.h" // generated version header

#include "AudioDeviceCollection.h"

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

// Function to watch for key input in a separate thread
void keyInputThread(GMainLoop* loop, std::atomic<bool>& running) {
    std::cout << "Press 'q' and Enter to quit\n";
    while (running) {
        char input;
        std::cin >> input;
        if (input == 'q' || input == 'Q') {
            std::cout << "Quitting...\n";
            g_main_loop_quit(loop);
            break;
        }
    }
}

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
        const auto subscriber = std::make_shared<ConsoleSubscriber>();
        
        collection.Subscribe(subscriber);
        collection.StartMonitoring();

        // Create a GLib main loop
        GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

        // Set up a flag for the input thread
        std::atomic<bool> running{true};

        // Start the key input thread
        std::thread inputThread(keyInputThread, loop, std::ref(running));

        // Run the main loop
        g_main_loop_run(loop);

        // Clean up
        running = false;
        if (inputThread.joinable()) {
            inputThread.join();
        }
        g_main_loop_unref(loop);

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
