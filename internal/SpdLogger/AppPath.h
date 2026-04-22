#pragma once

#include <filesystem>
#include <sstream>

namespace ed::utility
{
    class AppPath
    {
    public:
        static bool GetAndValidateLogFilePathName(std::filesystem::path& logFile, const std::string& appFileNameWoExt);
    protected:
        static void GetLogDir(std::filesystem::path& ownDataPath, const std::string& appFileNameWoExt);
    };
}

inline bool ed::utility::AppPath::GetAndValidateLogFilePathName(std::filesystem::path& logFile, const std::string& appFileNameWoExt)
{
    std::filesystem::path ownDataPath;
    GetLogDir(ownDataPath, appFileNameWoExt);

    auto logFileNameWoExt(appFileNameWoExt);
    while (logFileNameWoExt.find('.') != std::string::npos)
    { // replace . via _
        logFileNameWoExt.replace(logFileNameWoExt.find('.'), 1, "_");
    }
    if (exists(ownDataPath) || create_directories(ownDataPath))
    {
        std::filesystem::path logFilePathName;
        auto numberToIncrement = -1;
        do
        {
            if (++numberToIncrement > 99999)
            {
                return false;
            }
            std::ostringstream ossForFileName;
            ossForFileName << logFileNameWoExt << std::setfill('0') << std::setw(5) << numberToIncrement;
            logFilePathName = ownDataPath / ossForFileName.str();
            logFilePathName.replace_extension(".log");
        } while (std::filesystem::exists(logFilePathName));

        logFile.swap(logFilePathName);
        return true;
    }
    return false;
}

inline void ed::utility::AppPath::GetLogDir(std::filesystem::path& ownDataPath,
    [[maybe_unused]] const std::string& appFileNameWoExt)
{
    ownDataPath = std::filesystem::path(std::getenv("HOME")) / "logs";
}

