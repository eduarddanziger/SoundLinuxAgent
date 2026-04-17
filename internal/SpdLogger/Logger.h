#pragma once

#include <spdlog/spdlog.h>
#include <filesystem>

#include "../ClassDefHelper.h"
#include "../TimeUtil.h"


#include <string>

#include "LogBuffer.h"

#include <spdlog/sinks/dist_sink.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <utility>
#include <spdlog/pattern_formatter.h>


// ReSharper disable CppClangTidyClangDiagnosticPragmaMessages
#ifndef RESOURCE_FILENAME_ATTRIBUTE
#define RESOURCE_FILENAME_ATTRIBUTE "UnknownAppFilename"
#endif

#ifndef PRODUCT_VERSION_ATTRIBUTE
#define PRODUCT_VERSION_ATTRIBUTE "UnknownAppVersion"
#endif
// ReSharper restore CppClangTidyClangDiagnosticPragmaMessages

namespace ed::model
{
    using TMessageCallback = void(const std::string& timestamp, const std::string& level, const std::string& message);

    class CallbackSink;

    class Logger final
    {
    public:
        DISALLOW_COPY_MOVE(Logger);
    private:
        Logger();
    public:
        ~Logger() = default;

        static Logger& Inst();

        Logger& ConfigureAppNameAndVersion(const std::string& appName, const std::string& appVersion);

        Logger& SetPathName(const std::filesystem::path& fileName);
        [[nodiscard]] std::filesystem::path GetPathName() const;
        [[nodiscard]] std::wstring GetDir() const;

        Logger& SetOutputToConsole(bool isOutputToConsole);
        [[nodiscard]] bool IsOutputToConsole() const { return isOutputToConsole_; }

        Logger& SetDelimiterBetweenDateAndTime(const std::string& delimiterBetweenDateAndTime = " ");
        [[nodiscard]] std::string GetDelimiterBetweenDateAndTime() const;

        Logger& SetMessageCallback(TMessageCallback messageCallback);
        [[nodiscard]] bool IsMessageCallbackSet() const { return messageCallback_ != nullptr; }

        void SetLogBuffer(std::shared_ptr<LogBuffer> logBuffer);
        [[nodiscard]] bool IsLogBufferSet() const { return spLogBuffer_ != nullptr; }

        void Free();
    private:
        void Reinit();
    private:
        std::shared_ptr<LogBuffer> spLogBuffer_;
        std::filesystem::path pathName_;
        bool isOutputToConsole_ = false;
        std::string delimiterBetweenDateAndTime_ = " ";
        std::shared_ptr<spdlog::details::thread_pool> threadPoolSmartPtr_;
        std::string appName_;
        std::string appVersion_;
        TMessageCallback* messageCallback_;
    };

    class CallbackSink final : public spdlog::sinks::sink
    {
    public:
        explicit CallbackSink(TMessageCallback* callback);

        void log(const spdlog::details::log_msg& msg) override;

        void flush() override
        {
        }

        void set_pattern([[maybe_unused]] const std::string& pattern) override
        {
        }

        void set_formatter([[maybe_unused]] std::unique_ptr<spdlog::formatter> sinkFormatter) override
        {
        }

    private:
        TMessageCallback* callback_;
        std::unique_ptr<spdlog::formatter> formatter_;
    };


}


inline ed::model::Logger::Logger()
    : appName_(RESOURCE_FILENAME_ATTRIBUTE)
      , appVersion_(PRODUCT_VERSION_ATTRIBUTE)
      , messageCallback_(nullptr)
{
}

inline ed::model::Logger& ed::model::Logger::Inst()
{
    static Logger logger;
    return logger;
}

inline ed::model::Logger& ed::model::Logger::ConfigureAppNameAndVersion(const std::string& appName,
    const std::string& appVersion)
{
    appName_ = appName;
    appVersion_ = appVersion;
    Reinit();
    return *this;
}

inline ed::model::Logger& ed::model::Logger::SetPathName(const std::filesystem::path& fileName)
{
    pathName_ = fileName;
    Reinit();
    return *this;
}

inline std::filesystem::path ed::model::Logger::GetPathName() const
{
    return pathName_;
}

inline std::wstring ed::model::Logger::GetDir() const
{
    auto pathName = pathName_.wstring();
    const wchar_t* pathNamePtr = pathName.c_str();
    if
    (
        const wchar_t* found;
        (found = wcsrchr(pathNamePtr, L'\\')) != nullptr ||
        (found = wcsrchr(pathNamePtr, L'/')) != nullptr
    )
    {
        return {pathNamePtr, found};
    }

    return pathName;
}

inline ed::model::Logger& ed::model::Logger::SetDelimiterBetweenDateAndTime(
    const std::string& delimiterBetweenDateAndTime)
{
    if (delimiterBetweenDateAndTime_ != delimiterBetweenDateAndTime)
    {
        delimiterBetweenDateAndTime_ = delimiterBetweenDateAndTime;
        Reinit();
    }
    return *this;
}

inline std::string ed::model::Logger::GetDelimiterBetweenDateAndTime() const
{
    return delimiterBetweenDateAndTime_;
}

inline ed::model::Logger& ed::model::Logger::SetMessageCallback(TMessageCallback messageCallback)
{
    messageCallback_ = messageCallback;
    Reinit();
    return *this;
}

inline void ed::model::Logger::SetLogBuffer(std::shared_ptr<LogBuffer> logBuffer)
{
    spLogBuffer_ = std::move(logBuffer);
    Reinit();
}

inline ed::model::Logger& ed::model::Logger::SetOutputToConsole(bool isOutputToConsole)
{
    if (isOutputToConsole_ != isOutputToConsole)
    {
        isOutputToConsole_ = isOutputToConsole;
        Reinit();
    }
    return *this;
}


inline void ed::model::Logger::Reinit()
{
    auto finalMessage = std::string();
    spdlog::shutdown();

    auto distributedSink = std::make_shared<spdlog::sinks::dist_sink_st>();
    if (!pathName_.empty())
    {
        const auto rotatingFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            pathName_.string(), 1024 * 1024, 10);
        distributedSink->add_sink(rotatingFileSink);
        finalMessage += "Output to file ";
        finalMessage += pathName_.string();
    }
    else
    {
        finalMessage += "No output to file";
    }


    finalMessage += ", output to terminal (console)";
    if (isOutputToConsole_)
    {
        const auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        distributedSink->add_sink(consoleSink);
        finalMessage += " enabled";
    }
    else
    {
        finalMessage += " disabled";
    }

    finalMessage += ", output to log buffer";
    if (spLogBuffer_ != nullptr)
    {
        distributedSink->add_sink(spLogBuffer_);
        finalMessage += " enabled";
    }
    else
    {
        finalMessage += " disabled";
    }

    finalMessage += ", output to external callback";
    if (messageCallback_ != nullptr)
    {
        const auto callbackSink = std::make_shared<CallbackSink>(messageCallback_);
        distributedSink->add_sink(callbackSink);
        finalMessage += " enabled";
    }
    else
    {
        finalMessage += " disabled";
    }

    threadPoolSmartPtr_ = std::make_shared<
        spdlog::details::thread_pool>(65536, 2);

    // Create an async_logger using that custom thread pool
    const auto spdLogger = std::make_shared<spdlog::async_logger>(
        RESOURCE_FILENAME_ATTRIBUTE,
        distributedSink,
        threadPoolSmartPtr_,
        spdlog::async_overflow_policy::block
    );

    spdlog::register_logger(spdLogger);
    spdlog::set_default_logger(spdLogger);

    spdlog::set_pattern(std::string("%Y-%m-%d") + delimiterBetweenDateAndTime_ + "%H:%M:%S.%f%z %L [%t] %v");
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::debug);
    spdlog::info("Log for {} (version {}) was reinitiated: {}", appName_, appVersion_, finalMessage);
}

inline void ed::model::Logger::Free()
{
    if (threadPoolSmartPtr_ == nullptr)
    {
        return;
    }
    spdlog::info("Log for {} (version {}) is being freed.", appName_, appVersion_);
    spdlog::shutdown();
    threadPoolSmartPtr_.reset(); // Explicitly reset the thread pool to free resources
}

inline ed::model::CallbackSink::CallbackSink(TMessageCallback* callback)
    : callback_(callback)
      , formatter_(std::make_unique<spdlog::pattern_formatter>("%v"))
{
}

inline void ed::model::CallbackSink::log(const spdlog::details::log_msg& msg)
{
    if (callback_ != nullptr)
    {
        spdlog::memory_buf_t formatted;
        if (formatter_)
        {
            formatter_->format(msg, formatted);
        }

        const auto timestamp = TimePointToStringAsLocal(msg.time, false, false);
        const auto levelStringView = spdlog::level::to_string_view(msg.level);
        const std::string levelString(levelStringView.data(), levelStringView.size());
        auto messageString = fmt::to_string(formatted);

        // remove trailing new line characters
        if (!messageString.empty() && messageString.back() == '\n')
        {
            messageString.pop_back();
        }
        if (!messageString.empty() && messageString.back() == '\r')
        {
            messageString.pop_back();
        }

        callback_(timestamp, levelString, messageString);
    }
}
