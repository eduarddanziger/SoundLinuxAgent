#pragma once

#include <cpprest/http_client.h>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>

#include <ClassDefHelper.h>

class HttpRequestProcessor {

public:
    struct RequestItem {
        web::http::http_request Request;
        std::string UrlSuffix;
        std::string Payload;
        std::string Hint; // For logging/tracking
    };

    // Updated constructor: now takes both apiBaseUrl and codespaceName, along with universalToken.
    HttpRequestProcessor(std::string apiBaseUrl,
                         std::string universalToken,
                         std::string codespaceName);

    DISALLOW_COPY_MOVE(HttpRequestProcessor);

    ~HttpRequestProcessor();

    void EnqueueRequest(
        const web::http::http_request & request,
        const std::string & urlSuffix, const std::string & payload, const std::string & hint);

private:
    void ProcessingWorker();
    static bool SendRequest(const RequestItem & item, const std::string & urlBase);
    RequestItem CreateAwakingRequest() const;
    RequestItem CloneRequestItem(const RequestItem& original) const;

private:
    std::string apiBaseUrlNoTrailingSlash_;
    std::string universalToken_;
    std::string codespaceName_; // Newly added member for codespace name

    std::deque<RequestItem> requestQueue_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread workerThread_;
    std::atomic<bool> running_;
    uint64_t retryAwakingCount_ = 0;
    static constexpr uint64_t MAX_AWAKING_RETRIES = 15;
    static constexpr uint64_t MAX_IGNORING_RETRIES = MAX_AWAKING_RETRIES * 3;
};
