#include <Poco/Util/ServerApplication.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Task.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include "AgentObserver.h"
#include "cpversion.h"

using Poco::Util::ServerApplication;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;

class SoundLinuxDaemon : public ServerApplication
{
protected:
    void initialize(Application& self)
    {
        loadConfiguration();
        ServerApplication::initialize(self);
        spdlog::info("SoundLinuxDaemon initializing");
    }

    void uninitialize()
    {
        spdlog::info("SoundLinuxDaemon shutting down");
        ServerApplication::uninitialize();
    }

    void defineOptions(OptionSet& options)
    {
        ServerApplication::defineOptions(options);
        
        options.addOption(
            Option("help", "h", "Display help information")
                .required(false)
                .repeatable(false)
                .callback(Poco::Util::OptionCallback<SoundLinuxDaemon>(this, &SoundLinuxDaemon::handleHelp)));

        options.addOption(
            Option("version", "v", "Display version information")
            .required(false)
            .repeatable(false)
            .callback(Poco::Util::OptionCallback<SoundLinuxDaemon>(this, &SoundLinuxDaemon::handleVersion)));

    }

    void handleHelp(const std::string& name, const std::string& value)
    {
        HelpFormatter helpFormatter(options());
        helpFormatter.setCommand(commandName());
        helpFormatter.setUsage("OPTIONS");
        helpFormatter.setHeader("Sound Linux Agent Daemon.");
        helpFormatter.format(std::cout);
        stopOptionsProcessing();
        _helpRequested = true;
    }

    void handleVersion(const std::string& name, const std::string& value)
    {
        std::cout << "AudioTest version " << VERSION << "\n";
        stopOptionsProcessing();
        _helpRequested = true;
    }

    int main(const std::vector<std::string>& args)
    {
        if (_helpRequested)
            return Application::EXIT_OK;
            
		spdlog::info("SoundLinuxDaemon {} started", VERSION); 
            
        // Initialize your sound library functionality here
        // SoundLib soundLib;
        // soundLib.initialize();
        
        // The server application will now wait for termination
        waitForTerminationRequest();
        
        return Application::EXIT_OK;
    }
    
private:
    bool _helpRequested = false;
};

POCO_SERVER_MAIN(SoundLinuxDaemon)
