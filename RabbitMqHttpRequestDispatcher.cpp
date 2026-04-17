#include "os-dependencies.h"

#include "RabbitMqHttpRequestDispatcher.h"

#include "RequestPublisher.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>


using namespace BloombergLP;


RabbitMqHttpRequestDispatcher::RabbitMqHttpRequestDispatcher(
    const std::string& host,
    const std::string& user,
    const std::string& password
)
    : requestPublisher_(std::make_unique<RequestPublisher>(host, "/", user, password))
{
}

RabbitMqHttpRequestDispatcher::~RabbitMqHttpRequestDispatcher() = default;

void RabbitMqHttpRequestDispatcher::EnqueueRequest(bool postOrPut, const std::string& urlSuffix,
                                          const std::string& payload, const std::string& hint)
{
    spdlog::info("Publishing to the RabbitMQ queue: {}...", hint);
    const nlohmann::json jsonPayload = nlohmann::json::parse(payload);
    requestPublisher_->Publish(jsonPayload, postOrPut ? "POST" : "PUT", urlSuffix);
}
