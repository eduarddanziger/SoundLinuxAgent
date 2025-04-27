#pragma once

#include <cpprest/http_client.h>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>

class HttpRequestProcessor {

public:
    struct RequestItem {
        web::http::http_request Request;
        std::wstring UrlSuffix;
        std::string Hint; // For logging/tracking
    };

    // Updated constructor: now takes both apiBaseUrl and codespaceName, along with universalToken.
    HttpRequestProcessor(std::wstring apiBaseUrl,
                         std::wstring universalToken,
                         std::wstring codespaceName);

    DISALLOW_COPY_MOVE(HttpRequestProcessor);

    ~HttpRequestProcessor();

    void EnqueueRequest(
        const web::http::http_request & request,
        const std::wstring & urlSuffix, const std::string & hint);

private:
    void ProcessingWorker();
    static bool SendRequest(const RequestItem & item, const std::wstring & urlBase);
    RequestItem CreateAwakingRequest() const;

private:
    std::wstring apiBaseUrlNoTrailingSlash_;
    std::wstring universalToken_;
    std::wstring codespaceName_; // Newly added member for codespace name

    std::queue<RequestItem> requestQueue_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread workerThread_;
    std::atomic<bool> running_;
    uint64_t retryAwakingCount_ = 0;
	bool successfullySent_ = false;
    static constexpr uint64_t MAX_AWAKING_RETRIES = 15;
    static constexpr uint64_t MAX_IGNORING_RETRIES = MAX_AWAKING_RETRIES * 3;
};
