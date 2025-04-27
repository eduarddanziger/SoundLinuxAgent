
#include "FormattedOutput.h"

#include "TimeUtils.h"
#include "StringUtils.h"

#include "SpdLogger.h"

#include <ClassDefHelper.h>

#include "magic_enum/magic_enum_iostream.hpp"


void FormattedOutput::LogAndPrint(const std::string & mess)
{
    SPD_L->info(mess);
}

void FormattedOutput::LogAsErrorPrintAndThrow(const std::string & mess)
{
    SPD_L->error(mess);
	throw std::runtime_error(mess);
}

void FormattedOutput::PrintEvent(SoundDeviceEventType event, const std::string & devicePnpId)
{
    using magic_enum::iostream_operators::operator<<;

    std::ostringstream wos; wos << "Event caught: " << event << "."
        << " Device PnP id: " << devicePnpId << '\n';
    LogAndPrint(wos.str());
}

void FormattedOutput::PrintDeviceInfo(const SoundDeviceInterface * device)
{
    using magic_enum::iostream_operators::operator<<;

    const auto idString = device->GetPnpId();
    const std::wstring idAsWideString(idString.begin(), idString.end());
	std::ostringstream wos; wos << std::string(4, ' ')
        << idString
        << ", \"" << device->GetName()
        << "\", " << device->GetFlow()
        << ", Volume " << device->GetCurrentRenderVolume()
        << " / " << device->GetCurrentCaptureVolume();
    LogAndPrint(wos.str());
}

std::ostream & FormattedOutput::CurrentLocalTimeWithoutDate(std::ostream & os)
{
    const std::string currentTime = ed::getLocalTimeAsString();
    if
    (
        constexpr int beginOfTimeCountingFromTheEnd = 15;
        currentTime.size() >= beginOfTimeCountingFromTheEnd
    )
    {
        os << currentTime.substr(currentTime.size() - beginOfTimeCountingFromTheEnd, 12) << " ";
    }
    return os;
}
