#include "os-dependencies.h"

#include "StringUtils.h"

#include <boost/nowide/convert.hpp>


std::string ed::Utf16ToUtf8(const std::wstring& str)
{
    return boost::nowide::narrow(str);
}
