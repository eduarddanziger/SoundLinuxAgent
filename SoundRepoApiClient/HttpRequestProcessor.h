#pragma once

#include <cpprest/http_client.h>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <unordered_map>

#include "public/ClassDefHelper.h"

class HttpRequestProcessor {

public:
    struct RequestItem {
        bool PostOrPut;
		std::chrono::system_clock::time_point Time;
        std::string UrlSuffix;
        std::string Payload;
        std::unordered_map<std::string, std::string> Header;
        std::string Hint; // For logging/tracking
    };

    HttpRequestProcessor(std::string apiBaseUrl,
                         std::string universalToken,
                         std::string codeSpaceName);

    DISALLOW_COPY_MOVE(HttpRequestProcessor);

    ~HttpRequestProcessor();

    void EnqueueRequest(
        bool postOrPut,
        const std::chrono::system_clock::time_point & time,
        const std::string & urlSuffix, const std::string & payload,
        const std::unordered_map<std::string, std::string> & header,
        const std::string & hint
    );

private:
    void ProcessingWorker();
    static bool SendRequest(const RequestItem & requestItem, const std::string & urlBase);
    [[nodiscard]] RequestItem CreateAwakingRequest() const;

private:
    std::string apiBaseUrlNoTrailingSlash_;
    std::string universalToken_;
    std::string codeSpaceName_;

    std::deque<RequestItem> requestQueue_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread workerThread_;
    std::atomic<bool> running_;
    uint64_t retryAwakingCount_ = 0;
    static constexpr uint64_t MAX_AWAKING_RETRIES = 15;
    static constexpr uint64_t MAX_IGNORING_RETRIES = MAX_AWAKING_RETRIES * 3;
};
