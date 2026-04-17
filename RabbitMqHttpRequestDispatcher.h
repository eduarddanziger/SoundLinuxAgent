#pragma once

#include "internal/ClassDefHelper.h"

#include "HttpRequestDispatcherInterface.h"

#include <memory>
#include <string>

class RequestPublisher;

class RabbitMqHttpRequestDispatcher final : public HttpRequestDispatcherInterface
{
public:
    RabbitMqHttpRequestDispatcher(
        const std::string& host,
        const std::string& user,
        const std::string& password
    );

    DISALLOW_COPY_MOVE(RabbitMqHttpRequestDispatcher);

    ~RabbitMqHttpRequestDispatcher() override;

    void EnqueueRequest(
        bool postOrPut,
        const std::string& urlSuffix,
        const std::string& payload,
        const std::string& hint
    ) override;

private:
    std::unique_ptr<RequestPublisher> requestPublisher_;
};
