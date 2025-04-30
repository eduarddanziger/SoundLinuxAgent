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

void HttpRequestProcessor::EnqueueRequest(bool postOrPut, const std::chrono::system_clock::time_point & time, const std::string & urlSuffix,
                                          const std::string & payload, const std::unordered_map<std::string, std::string> & header, const std::string & hint)
{
    std::unique_lock lock(mutex_);

    // Add to queue
    requestQueue_.push_back(RequestItem{
        .PostOrPut = postOrPut,
        .Time = time,
        .UrlSuffix = urlSuffix,
        .Payload = payload,
        .Header = header,
        .Hint = hint
    });

    // Notify worker thread
    condition_.notify_one();
}

bool HttpRequestProcessor::SendRequest(const RequestItem & requestItem, const std::string & urlBase)
{
    const auto messageDeviceAppendix = requestItem.Hint;

    try
    {
        SPD_L->info("Processing request: {}", messageDeviceAppendix);

        // Create HTTP client
        web::http::client::http_client client(urlBase + requestItem.UrlSuffix);

        // Create HTTP request
        web::http::http_request httpRequest(requestItem.PostOrPut ? web::http::methods::POST : web::http::methods::PUT);
        httpRequest.headers().set_content_type("application/json");
        httpRequest.set_body(web::json::value::parse(requestItem.Payload));
        for (const auto& [name, val] : requestItem.Header)
        {
            httpRequest.headers().add(name, val);
        }

        // Synchronously send the request and get response
        const web::http::http_response response = client.request(httpRequest).get();

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

        const auto itemCloned = item;

		// If the sending was successful, set retries to 0 and remove the request from the queue
        if (SendRequest(item, apiBaseUrlNoTrailingSlash_))
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

            // send awaking request
            const auto url = std::format("https://api.github.com/user/codespaces/{}/start", codespaceName_);
            SendRequest(
                CreateAwakingRequest()
                , url);

			// push the prepared cloned request back to the queue
		    std::unique_lock lock(mutex_);
            requestQueue_.push_front(itemCloned);
        }
		else
		{   // Retries exhausted
			const auto msg = std::string("Request sending to \"") + apiBaseUrlNoTrailingSlash_ + "\" unsuccessful. Retries exhausted. Skipping request's sending.";
			FormattedOutput::LogAndPrint(msg);
    		retryAwakingCount_ = 0;
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

    const std::string authorizationValue = "Bearer " + universalToken_;
	std::unordered_map<std::string, std::string> header{
		{"Authorization", authorizationValue},
		{"Accept", "application/vnd.github.v3+json"}
	};

	std::ostringstream oss; oss << " awaking a backend " << retryAwakingCount_ << " / " << MAX_AWAKING_RETRIES;
    return RequestItem{.PostOrPut = true, .Time = std::chrono::system_clock::now(), .UrlSuffix = "" , .Payload = payloadString, .Header = header, .Hint = oss.str() };
}

