#include "SpdLogger.h"
#include "cpversion.h"

#include "KeyInputThread.h"

#include <memory>
#include <thread>
#include <iostream>

#include "public/SoundAgentInterface.h"
#include "magic_enum/magic_enum_iostream.hpp"


class AgentObserver final : public SoundDeviceObserverInterface
{
public:
    explicit AgentObserver(SoundDeviceCollectionInterface& collection)
        :collection_(collection)
    {
    }

    DISALLOW_COPY_MOVE(AgentObserver);
    ~AgentObserver() override = default;

    void OnCollectionChanged(SoundDeviceEventType event, const std::string& devicePnpId) override;

    static void PrintDeviceInfo(const SoundDeviceInterface* device);

    void PrintCollection() const;
private:
    SoundDeviceCollectionInterface& collection_;
};


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
        SpdLogSetup("SoundLinuxCli.log");
        
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
        
        // Print a last device collection state
        spdlog::info("Print collection final state...");
        subscriber.PrintCollection();

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

inline void AgentObserver::OnCollectionChanged(SoundDeviceEventType event, const std::string& devicePnpId)
{
    spdlog::info("Event \"{}\" caught, device PnP ID: {}.", magic_enum::enum_name(event), devicePnpId);
    if (event != SoundDeviceEventType::Detached)
    {
        const auto device = collection_.CreateItem(devicePnpId);
        if (device)
        {
            PrintDeviceInfo(device.get());
        }
        else
        {
            spdlog::warn("Failed to create device for PnP ID: {}", devicePnpId);
        }
    }
}

inline void AgentObserver::PrintDeviceInfo(const SoundDeviceInterface * device)
{
    using magic_enum::iostream_operators::operator<<;

    const auto idString = device->GetPnpId();
    std::ostringstream os;
    os << "Device " << idString << ": \"" << device->GetName()
        << "\", " << device->GetFlow() << ", Volume " << device->GetCurrentRenderVolume()
        << " / " << device->GetCurrentCaptureVolume();
    spdlog::info(os.str());
}

inline void AgentObserver::PrintCollection() const
{
    for (size_t i = 0; i < collection_.GetSize(); ++i)
    {
        const std::unique_ptr<SoundDeviceInterface> deviceSmartPtr(collection_.CreateItem(i));
        PrintDeviceInfo(deviceSmartPtr.get());
    }
    spdlog::info("...Collection print finished.", collection_.GetSize());
}

