#include "SpdLogSetup.h"

#include <Poco/Util/ServerApplication.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Task.h>
#include <iostream>
#include <csignal>
#include <atomic>

#include "AgentObserver.h"
#include "cpversion.h"

using Poco::Util::ServerApplication;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;

class SoundLinuxDaemon final : public ServerApplication
{
protected:
    static std::function<void()> s_deactivateCallback;
    static void signalHandler(int signum) {
        if (s_deactivateCallback) {
            s_deactivateCallback();
        }
    }

    static void SetupSignalHandlers(
        SoundDeviceCollectionInterface& collection,
        std::function<void()> deactivateCallback
    )
    {
        s_deactivateCallback = deactivateCallback;
        std::signal(SIGTERM, signalHandler);
        std::signal(SIGINT, signalHandler);
    }

    void initialize(Application& self) override
    {
        loadConfiguration();
        ServerApplication::initialize(self);
    }

    void defineOptions(OptionSet& options) override
    {
        ServerApplication::defineOptions(options);
        
        options.addOption(
            Poco::Util::Option("url", "u", "Base Server URL, e.g. http://localhost:5027")
            .required(false)
            .repeatable(false)
            .argument("<url>", true)
            .callback(Poco::Util::OptionCallback<SoundLinuxDaemon>(this, &SoundLinuxDaemon::HandleUrl)));

        options.addOption(
            Option("help", "h", "Help information")
                .required(false)
                .repeatable(false)
                .callback(Poco::Util::OptionCallback<SoundLinuxDaemon>(this, &SoundLinuxDaemon::handleHelp)));

        options.addOption(
            Option("version", "v", "Version information")
            .required(false)
            .repeatable(false)
            .callback(Poco::Util::OptionCallback<SoundLinuxDaemon>(this, &SoundLinuxDaemon::handleVersion)));

    }

    void HandleUrl(const std::string& name, const std::string& value)
    {
        std::cout << "Got Server URL " << value << "\n";
    }

    void handleHelp(const std::string& name, const std::string& value)
    {
        HelpFormatter helpFormatter(options());
        helpFormatter.setCommand(commandName());
        helpFormatter.setHeader("Options:");
        helpFormatter.setUsage("[options]");
        helpFormatter.setFooter("\n");
        helpFormatter.format(std::cout);
        stopOptionsProcessing();
        _helpRequested = true;    }

    void handleVersion(const std::string& name, const std::string& value)
    {
        std::cout << "Version " << VERSION << "\n";
        stopOptionsProcessing();
        _helpRequested = true;
    }

    int main(const std::vector<std::string>& args) override
    {
        if (_helpRequested)
            return Application::EXIT_OK;
        try
        {
            SpdLogSetup("SoundLinuxDaemon.log");
            spdlog::info("Sound Linux Daemon {} started", VERSION); 

            const auto deviceCollectionSmartPtr = SoundAgent::CreateDeviceCollection();
            if (deviceCollectionSmartPtr == nullptr)
            {
                throw std::runtime_error("Failed to create device collection");
            }
            auto& collection = *deviceCollectionSmartPtr;
            
            AgentObserver subscriber(collection);
            collection.Subscribe(subscriber);

            std::atomic<SoundDeviceCollectionInterface*> collectionPtrAtomic(&collection);
            SetupSignalHandlers(collection,
                [&collectionPtrAtomic]()
                {
                    spdlog::info("Termination signal received.\n");
                    collectionPtrAtomic.load()->DeactivateAndStopLoop();
                }
            );

            collection.ActivateAndStartLoop(); // waits here for deactivation

            collection.Unsubscribe(subscriber);
            spdlog::info("Main loop exited. Shutting down...");
        }
        catch (const std::exception & e)
        {
            spdlog::error("...Version {}, ended with an exception: {}", VERSION, e.what());
            return Application::EXIT_SOFTWARE;
        }
        spdlog::info("...Version {}, ended successfully...", VERSION);
        return Application::EXIT_OK;
    }
    
private:
    bool _helpRequested = false;
};

std::function<void()> SoundLinuxDaemon::s_deactivateCallback{nullptr};


POCO_SERVER_MAIN(SoundLinuxDaemon)
