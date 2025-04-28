#pragma once

#include <SoundAgentInterface.h>


class FormattedOutput final
{
public:
    ~FormattedOutput() = delete;
protected:
	DISALLOW_IMPLICIT_CONSTRUCTORS(FormattedOutput);
public:
    static void LogAndPrint(const std::string& mess);
    static void LogAsErrorPrintAndThrow(const std::string& mess);

    static void PrintEvent(SoundDeviceEventType event, const std::string & devicePnpId);

    static void PrintDeviceInfo(const SoundDeviceInterface* device);

    static std::ostream & CurrentLocalTimeWithoutDate(std::ostream & os);
};
