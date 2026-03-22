#include "ApiClient/common/SpdLogger.h"

#include <Poco/Util/ServerApplication.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Task.h>
#include <Poco/UnicodeConverter.h>
#include <Poco/String.h>

#include <iostream>
#include <csignal>
#include <atomic>

#include "cpversion.h"
#include "ServiceObserver.h"

#if SOUNDLINUXAGENT_HAS_RMQCPP
#include "ApiClient/RabbitMqHttpRequestDispatcher.h"
#endif


using Poco::Util::ServerApplication;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;

class LinuxSoundScanner final : public ServerApplication
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

        if (transportMethod_.empty())
        {   // If no transport method is provided via command line, read it from the configuration
            spdlog::info("Transport method not provided via command line. Reading from configuration...");
            transportMethod_ = ReadOptionalSimpleConfigProperty(API_TRANSPORT_METHOD_CONFIGURATED_PROPERTY_KEY, API_TRANSPORT_METHOD_VALUE00_NONE);
        }

        if (Poco::icompare(transportMethod_, API_TRANSPORT_METHOD_VALUE00_NONE) != 0
            && Poco::icompare(transportMethod_, API_TRANSPORT_METHOD_VALUE02_RABBITMQ) != 0
        )
        {
            spdlog::info(R"(Invalid transport method "{}". Using default: "{}".)", transportMethod_, API_TRANSPORT_METHOD_VALUE00_NONE);
            transportMethod_ = API_TRANSPORT_METHOD_VALUE00_NONE;
        }
        else
        {
            spdlog::info(R"(Transport method value "{}" validated.)", transportMethod_);
        }
    }

    static void SetUpLog()
    {
        constexpr auto appName = "LinuxSoundScanner";
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
            Poco::Util::Option("transport", "", "Transport method: None or RabbitMQ")
            .required(false)
            .repeatable(false)
            .argument("<transport>", true)
            .callback(Poco::Util::OptionCallback<LinuxSoundScanner>(this, &LinuxSoundScanner::HandleTransport)));

        options.addOption(
            Option("help", "h", "Help information")
                .required(false)
                .repeatable(false)
                .callback(Poco::Util::OptionCallback<LinuxSoundScanner>(this, &LinuxSoundScanner::HandleHelp)));

        options.addOption(
            Option("version", "v", "Version information")
            .required(false)
            .repeatable(false)
            .callback(Poco::Util::OptionCallback<LinuxSoundScanner>(this, &LinuxSoundScanner::handleVersion)));

    }

    void HandleTransport(const std::string& name, const std::string& value)
    {
        std::cout << fmt::format(R"(Got Transport Method "{}"
)", value);
        transportMethod_ = value;
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
        onlyConsoleOutputRequested_ = true;
    }

    void handleVersion(const std::string& , const std::string&)
    {
        std::cout << "Version " << VERSION << "\n";
        stopOptionsProcessing();
        onlyConsoleOutputRequested_ = true;
    }

    int main(const std::vector<std::string>&) override
    {
        if (onlyConsoleOutputRequested_)
            return Application::EXIT_OK;

        try
        {
            spdlog::info("Linux Sound Scanner {} started", VERSION); 

            const auto deviceCollectionSmartPtr = SoundAgent::CreateDeviceCollection();
            if (deviceCollectionSmartPtr == nullptr)
            {
                throw std::runtime_error("Failed to create device collection");
            }
            auto& collection = *deviceCollectionSmartPtr;

            std::unique_ptr<HttpRequestDispatcherInterface> requestDispatcherSmartPtr;

            if (Poco::icompare(transportMethod_, API_TRANSPORT_METHOD_VALUE00_NONE) == 0)
            {
                class EmptyDispatcher : public HttpRequestDispatcherInterface
                {
                public:
                    void EnqueueRequest(bool, const std::chrono::system_clock::time_point&,
                                        const std::string&, const std::string&,
                                        const std::unordered_map<std::string, std::string>&, const std::string&
                    ) override
                    {
                        spdlog::info("Enqueueing ignored, because the transport method is \"{}\"",
                                     API_TRANSPORT_METHOD_VALUE00_NONE);
                    }
                };
                requestDispatcherSmartPtr.reset(new EmptyDispatcher());
            }
            else if (Poco::icompare(transportMethod_, API_TRANSPORT_METHOD_VALUE02_RABBITMQ) == 0)
            {
#if SOUNDLINUXAGENT_HAS_RMQCPP
                requestDispatcherSmartPtr.reset(new RabbitMqHttpRequestDispatcher());
#else
                throw std::runtime_error(
                    "RabbitMQ transport selected, but this build was compiled without rmqcpp support. "
                    "Reconfigure with -DSOUNDLINUXAGENT_ENABLE_RMQCPP=ON so the vcpkg manifest installs rmqcpp."
                );
#endif
            }
            
//            AgentObserver subscriber(collection);
            ServiceObserver subscriber(collection, *requestDispatcherSmartPtr);

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

    [[nodiscard]] std::string ReadOptionalSimpleConfigProperty(const std::string& propertyName,
                                                               const std::string& defaultValue = "") const
    {
        if (!config().hasProperty(propertyName))
        {
            spdlog::info(R"(Property "{}" not found in configuration. Using default value: "{}".)", propertyName, defaultValue);
            return defaultValue;
        }

        return ReadStringConfigProperty(propertyName);
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
        return returnValue;
    }

private:
    bool onlyConsoleOutputRequested_ = false;
    
    std::string transportMethod_;

    std::string apiBaseUrl_;
    std::string universalToken_;
    std::string codespaceName_;

    static constexpr auto API_TRANSPORT_METHOD_CONFIGURATED_PROPERTY_KEY = "custom.transportMethod";
    static constexpr auto API_TRANSPORT_METHOD_VALUE00_NONE = "None";
    static constexpr auto API_TRANSPORT_METHOD_VALUE02_RABBITMQ = "RabbitMQ";
    
};

std::function<void()> LinuxSoundScanner::deactivateCallback_{nullptr};


POCO_SERVER_MAIN(LinuxSoundScanner)
