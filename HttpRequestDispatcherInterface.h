#pragma once

#include "internal/ClassDefHelper.h"

#include <string>

class HttpRequestDispatcherInterface
{
public:
    virtual void EnqueueRequest(
        bool postOrPut,
        const std::string& urlSuffix,
        const std::string& payload,
        const std::string& hint
    ) = 0;
    AS_INTERFACE(HttpRequestDispatcherInterface);
    DISALLOW_COPY_MOVE(HttpRequestDispatcherInterface);
};
