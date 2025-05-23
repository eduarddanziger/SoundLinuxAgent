#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "ClassDefHelper.h"

inline void SpdLogSetup(const std::string& logFileName = "log.log", 
                        size_t maxFileSize = 10240, 
                        size_t maxFiles = 5, 
                        spdlog::level::level_enum logLevel = spdlog::level::info)
{
    static bool initialized = false;
    if (initialized)
    {
        return;
    }
    const auto logPath = std::filesystem::path(std::getenv("HOME")) / "logs";  // NOLINT(concurrency-mt-unsafe)
    // Create logs directory if it doesn't exist
    std::filesystem::create_directories(logPath);

    // Set up spdlog to log to both console and rotating file
    const auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    const auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logPath / logFileName, maxFileSize, maxFiles);
    std::vector<spdlog::sink_ptr> sinks{ fileSink, consoleSink };
    const auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());

    spdlog::set_default_logger(logger);
    spdlog::set_level(logLevel); // Set global log level
    spdlog::flush_on(spdlog::level::info);
    spdlog::set_pattern("%^%Y-%m-%d %H:%M:%S.%f [%l] %v%$");

    spdlog::info("Logging initialized. Log file: {}", (logPath / logFileName).string());
	initialized = true;
}

namespace ed::model
{
    class Logger final  // NOLINT(clang-diagnostic-padded)
    {
    public:
        DISALLOW_COPY_MOVE(Logger);
    private:
        Logger();
    public:
        ~Logger() = default;

        static Logger& Inst();
        std::shared_ptr<spdlog::logger> L();
    };
}

inline ed::model::Logger::Logger()
{
    SpdLogSetup();
}

inline ed::model::Logger& ed::model::Logger::Inst()
{
    static Logger logger;
    return logger;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
inline std::shared_ptr<spdlog::logger> ed::model::Logger::L()
{
	return spdlog::default_logger();
}

#define SPD_L ed::model::Logger::Inst().L()



