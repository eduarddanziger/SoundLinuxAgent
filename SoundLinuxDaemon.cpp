#include "ApiClient/common/SpdLogger.h"

#include <Poco/Util/ServerApplication.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Task.h>
#include <iostream>
#include <csignal>
#include <atomic>

#include "cpversion.h"
#include "ServiceObserver.h"

#include "ApiClient/SodiumCrypt.h"

using Poco::Util::ServerApplication;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;

class SoundLinuxDaemon final : public ServerApplication
{
protected:
    static std::function<void()> deactivateCallback_;
    static void SignalHandler(int) {
        if (deactivateCallback_) {
            deactivateCallback_();
        }
    }

    static void SetupSignalHandlers(
        const std::function<void()> & deactivateCallback
    )
    {
        deactivateCallback_ = deactivateCallback;
        std::signal(SIGTERM, SignalHandler);
        std::signal(SIGINT, SignalHandler);
    }

    void initialize(Application& self) override
    {
        loadConfiguration();
        ServerApplication::initialize(self);

        SetUpLog();

        if (apiBaseUrl_.empty())
        {
            apiBaseUrl_ = ReadStringConfigProperty(API_BASE_URL_PROPERTY_KEY);
        }
        apiBaseUrl_ += "/api/AudioDevices";

        universalToken_ = ReadStringConfigProperty(UNIVERSAL_TOKEN_PROPERTY_KEY);
        codespaceName_ = ReadStringConfigProperty(CODESPACE_NAME_PROPERTY_KEY);
    }

    static void SetUpLog()
    {
        constexpr auto appName = "SoundLinuxDaemon";
        ed::model::Logger::Inst().ConfigureAppNameAndVersion(appName, VERSION).SetOutputToConsole(true);
        try
        {
            if (std::filesystem::path logFile;
                ed::utility::AppPath::GetAndValidateLogFilePathName(
                    logFile, appName)
            )
            {
                ed::model::Logger::Inst().SetPathName(logFile);
            }
            else
            {
                spdlog::warn("Log file can not be written.");
            }
        }
        catch (const std::exception& ex)
        {
            spdlog::warn("Logging set-up partially done; Log file can not be used: {}.", ex.what());
        }
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
                .callback(Poco::Util::OptionCallback<SoundLinuxDaemon>(this, &SoundLinuxDaemon::HandleHelp)));

        options.addOption(
            Option("version", "v", "Version information")
            .required(false)
            .repeatable(false)
            .callback(Poco::Util::OptionCallback<SoundLinuxDaemon>(this, &SoundLinuxDaemon::handleVersion)));

    }

    void HandleUrl(const std::string&, const std::string& value)
    {
        std::cout << "Got Server URL " << value << "\n";
        apiBaseUrl_ = value;
    }

    void HandleHelp(const std::string&, const std::string&)
    {
        HelpFormatter helpFormatter(options());
        helpFormatter.setCommand(commandName());
        helpFormatter.setHeader("Options:");
        helpFormatter.setUsage("[options]");
        helpFormatter.setFooter("\n");
        helpFormatter.format(std::cout);
        stopOptionsProcessing();
        helpRequested_ = true;    }

    void handleVersion(const std::string& , const std::string&)
    {
        std::cout << "Version " << VERSION << "\n";
        stopOptionsProcessing();
        helpRequested_ = true;
    }

    int main(const std::vector<std::string>&) override
    {
        if (helpRequested_)
            return Application::EXIT_OK;
        try
        {
            spdlog::info("Sound Linux Daemon {} started", VERSION); 

            const auto deviceCollectionSmartPtr = SoundAgent::CreateDeviceCollection();
            if (deviceCollectionSmartPtr == nullptr)
            {
                throw std::runtime_error("Failed to create device collection");
            }
            auto& collection = *deviceCollectionSmartPtr;
            
//            AgentObserver subscriber(collection);
            ServiceObserver subscriber(collection, apiBaseUrl_, universalToken_, codespaceName_);

            collection.Subscribe(subscriber);

            std::atomic<SoundDeviceCollectionInterface*> collectionPtrAtomic(&collection);
            SetupSignalHandlers(
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

    [[nodiscard]] std::string ReadStringConfigProperty(const std::string& propertyName) const
    {
        if (!config().hasProperty(propertyName))
        {
            const auto msg = std::string("FATAL: No \"") + propertyName + "\" property configured.";
            spdlog::error(msg);
            throw std::runtime_error(msg);
        }

        auto returnValue = config().getString(propertyName);
        try
        {
            returnValue = SodiumDecrypt(returnValue, "32-characters-long-secure-key-12");
        }
        catch (const std::exception& ex)  // NOLINT(bugprone-empty-catch)
        {
            spdlog::info("Decryption doesn't work: {}.", ex.what());
        }
        catch (...)
        {
            spdlog::error("Unknown error. Propagating...");
            throw;
        }

        return returnValue;
    }

private:
    bool helpRequested_ = false;

    std::string apiBaseUrl_;
    std::string universalToken_;
    std::string codespaceName_;

    static constexpr auto API_BASE_URL_PROPERTY_KEY = "custom.apiBaseUrl";
    static constexpr auto UNIVERSAL_TOKEN_PROPERTY_KEY = "custom.universalToken";
    static constexpr auto CODESPACE_NAME_PROPERTY_KEY = "custom.codespaceName";
};

std::function<void()> SoundLinuxDaemon::deactivateCallback_{nullptr};


POCO_SERVER_MAIN(SoundLinuxDaemon)
