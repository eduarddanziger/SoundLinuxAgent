#include "HttpRequestProcessor.h"

#include "FormattedOutput.h"

#include <nlohmann/json.hpp>
#include <format>

#include "SpdLogger.h"


HttpRequestProcessor::HttpRequestProcessor(std::string apiBaseUrl,
                                           std::string universalToken,
                                           std::string codespaceName)  // Added codespaceName parameter
    : apiBaseUrlNoTrailingSlash_(std::move(apiBaseUrl))
    , universalToken_(std::move(universalToken))
    , codespaceName_(std::move(codespaceName))  // Initialize new member
    , running_(true)
{
    workerThread_ = std::thread(&HttpRequestProcessor::ProcessingWorker, this);
}

HttpRequestProcessor::~HttpRequestProcessor()
{
    {
        std::unique_lock lock(mutex_);
        running_ = false;
        condition_.notify_all();
    }
    if (workerThread_.joinable())
    {
        workerThread_.join();
    }
}

void HttpRequestProcessor::EnqueueRequest(const web::http::http_request & request, const std::string & urlSuffix, const std::string & payload,
                                          const std::string & hint)
{
    std::unique_lock lock(mutex_);

    // Add to queue
    requestQueue_.push_back(RequestItem{.Request = request, .UrlSuffix = urlSuffix, .Payload = payload, .Hint = hint});

    // Notify worker thread
    condition_.notify_one();
}

bool HttpRequestProcessor::SendRequest(const RequestItem & item, const std::string & urlBase)
{
    const auto messageDeviceAppendix = item.Hint;

    try
    {
        SPD_L->info("Processing request: {}", messageDeviceAppendix);

        // Create HTTP client object
        web::http::client::http_client client(urlBase + item.UrlSuffix);

        // Synchronously send the request and get response
        const web::http::http_response response = client.request(item.Request).get();

        if (const auto statusCode = response.status_code();
            statusCode == web::http::status_codes::Created ||
            statusCode == web::http::status_codes::OK ||
            statusCode == web::http::status_codes::NoContent)
        {
            const auto msg = "Sent successfully: " + messageDeviceAppendix;
            FormattedOutput::LogAndPrint(msg);
        }
        else
        {
            const auto msg = "Failed to post data" + messageDeviceAppendix +
                " - Status code: " + std::to_string(statusCode);
            throw web::http::http_exception(msg);
        }
    }
    catch (const web::http::http_exception & ex)
    {
        const auto msg = "HTTP exception: " + messageDeviceAppendix + ": " + std::string(ex.what());
        FormattedOutput::LogAndPrint(msg);
        return false;
    }
    catch (const std::exception & ex)
    {
        const auto msg = "Common exception while sending HTTP request: " + messageDeviceAppendix + ": " +
            std::string(ex.what());
        FormattedOutput::LogAndPrint(msg);
        return false;
    }
    catch (...)
    {
        const auto msg = "Unspecified exception while sending HTTP request: " + messageDeviceAppendix;
        FormattedOutput::LogAndPrint(msg);
    }
    return true;
}

void HttpRequestProcessor::ProcessingWorker()
{
    while (true)
    {
        RequestItem item;
        {
            std::unique_lock lock(mutex_);

            condition_.wait(lock, [this]
            {
                return !running_ || !requestQueue_.empty();
            });

            // Check if we're shutting down
            if (!running_) // && requestQueue_.empty())
            {
                break;
            }

            if (requestQueue_.empty())
            {
                continue;
            }

            item = requestQueue_.front();
            requestQueue_.pop_front();
        }

        const auto itemCloned = CloneRequestItem(item);

		// If the sending was successful, set retries to 0 and remove the request from the queue
        if ((SendRequest(item, apiBaseUrlNoTrailingSlash_)))
		{   // Request was successful
            retryAwakingCount_ = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

		// Check if base url is on GitHub Codespace. If not , we don't need to wake up
        if (apiBaseUrlNoTrailingSlash_.find(".github.") == std::string::npos)
        {// NOT  a GitHub Codespace, no wake up
            const auto msg = std::string("Request sending to \"") + apiBaseUrlNoTrailingSlash_ + "\" unsuccessful. Wake up make no sense. Skipping request.";
			FormattedOutput::LogAndPrint(msg);
			continue;
        }

		if (++retryAwakingCount_ <= MAX_AWAKING_RETRIES)
		{   // Wake-retrials are yet to be exhausted

            const auto url = std::format("https://api.github.com/user/codespaces/{}/start", codespaceName_);
            SendRequest(
                CreateAwakingRequest()
                , url);
            std::unique_lock lock(mutex_);
            requestQueue_.push_front(itemCloned); // Requeue the request!
        }
		else
		{   
            if (retryAwakingCount_ > MAX_IGNORING_RETRIES)
            {
				retryAwakingCount_ = 0;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }
}

HttpRequestProcessor::RequestItem HttpRequestProcessor::CreateAwakingRequest() const
{
    const nlohmann::json payload = {
        // ReSharper disable once StringLiteralTypo
        {"codespace_name", codespaceName_}
    };
    // Convert nlohmann::json to string and to value
    const std::string payloadString = payload.dump();
    const web::json::value jsonPayload = web::json::value::parse(payloadString);

    const std::string authorizationValue = "Bearer " + universalToken_;

    web::http::http_request request(web::http::methods::POST);
    request.headers().add(U("Authorization"), authorizationValue);
    request.headers().add(U("Accept"), U("application/vnd.github.v3+json"));
    request.headers().set_content_type(U("application/json"));
    request.set_body(jsonPayload);

	std::ostringstream oss; oss << " awaking a backend " << retryAwakingCount_ << " / " << MAX_AWAKING_RETRIES;
    return RequestItem{ .Request = request, .UrlSuffix = "" , .Payload = payloadString, .Hint = oss.str() };
}

HttpRequestProcessor::RequestItem HttpRequestProcessor::CloneRequestItem(const RequestItem& original) const
{
    web::http::http_request cloned(original.Request.method());
    cloned.set_request_uri(original.Request.relative_uri());
    cloned.headers() = original.Request.headers();

    const web::json::value jsonPayload = web::json::value::parse(original.Payload);
    cloned.set_body(jsonPayload);
  

    return RequestItem{
        .Request = cloned,
        .UrlSuffix = original.UrlSuffix,
        .Payload = original.Payload,
        .Hint = original.Hint
    };
}
