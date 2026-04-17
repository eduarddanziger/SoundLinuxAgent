#pragma once

#include <string_view>

namespace contracts::message_fields
{
    inline constexpr std::string_view HTTP_REQUEST = "httpRequest";
    inline constexpr std::string_view URL_SUFFIX = "urlSuffix";

    inline constexpr std::string_view DEVICE_MESSAGE_TYPE = "deviceMessageType";
    inline constexpr std::string_view VOLUME = "volume";
    inline constexpr std::string_view UPDATE_DATE = "updateDate";
}
