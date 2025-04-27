#include "stdafx.h"

#include "HttpRequestProcessor.h"

#include "FormattedOutput.h"

#include <nlohmann/json.hpp>
#include <format>


HttpRequestProcessor::HttpRequestProcessor(std::wstring apiBaseUrl,
    std::wstring universalToken,
    std::wstring codespaceName)  // Added codespaceName parameter
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

void HttpRequestProcessor::EnqueueRequest(const web::http::http_request & request, const std::wstring & urlSuffix,
                                          const std::string & hint)
{
    std::unique_lock lock(mutex_);

    // Add to queue
    requestQueue_.push(RequestItem{.Request = request, .UrlSuffix = urlSuffix ,.Hint = hint});

    // Notify worker thread
    condition_.notify_one();
}

bool HttpRequestProcessor::SendRequest(const RequestItem & item, const std::wstring & urlBase)
{
    const auto messageDeviceAppendix = item.Hint;

    try
    {
        SPD_L->info("Processing request{}", messageDeviceAppendix);

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
        }

		// If the sending of the "genuine" request (before the actual copy) was successful, let's skip the copy and continue processing
		if (item.Hint.find("(copy)") != std::string::npos && successfullySent_)
		{
			std::unique_lock lock(mutex_);
			requestQueue_.pop();
			continue;
		}


		// If the sending was successful, set retries to 0 and remove the request from the queue
        if ((successfullySent_ = SendRequest(item, apiBaseUrlNoTrailingSlash_)))
		{   // Request was successful
            std::unique_lock lock(mutex_);
            retryAwakingCount_ = 0;
            requestQueue_.pop();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

		// Check if base url is on GitHub Codespace. If not , we don't need to wake up
        if (apiBaseUrlNoTrailingSlash_.find(L".github.") == std::wstring::npos)
        {// NOT  a GitHub Codespace, no wake up
            const auto msg = std::wstring(L"Request sending to \"") + apiBaseUrlNoTrailingSlash_ + L"\" unsuccessful. Wake up make no sense. Skipping request.";
			FormattedOutput::LogAndPrint(msg);

			std::unique_lock lock(mutex_);
			requestQueue_.pop();
			continue;
        }

		if (++retryAwakingCount_ <= MAX_AWAKING_RETRIES)
		{   // Wake-retrials are yet to be exhausted
            const auto url = std::format(L"https://api.github.com/user/codespaces/{}/start", codespaceName_);
            SendRequest(
                CreateAwakingRequest()
                , url);
        }
		else
		{   // we have tried enough. Abandon the request (pop)
            std::unique_lock lock(mutex_);
            requestQueue_.pop();
            if (retryAwakingCount_ > MAX_IGNORING_RETRIES)
            {
				retryAwakingCount_ = 0;
            }
                
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

HttpRequestProcessor::RequestItem HttpRequestProcessor::CreateAwakingRequest() const
{
    const std::string codespaceNameUtf8 = utility::conversions::to_utf8string(codespaceName_);

    const nlohmann::json payload = {
        // ReSharper disable once StringLiteralTypo
        {"codespace_name", codespaceNameUtf8}
    };
    // Convert nlohmann::json to cpprestsdk::json::value
    const web::json::value jsonPayload = web::json::value::parse(payload.dump());

    const std::wstring authorizationValue = L"Bearer " + universalToken_;

    web::http::http_request request(web::http::methods::POST);
    request.headers().add(U("Authorization"), authorizationValue);
    request.headers().add(U("Accept"), U("application/vnd.github.v3+json"));
    request.headers().set_content_type(U("application/json"));
    request.set_body(jsonPayload);

	std::ostringstream oss; oss << " awaking a backend " << retryAwakingCount_ << " / " << MAX_AWAKING_RETRIES;
    return RequestItem{ .Request = request, .UrlSuffix = L"" ,.Hint = oss.str() };
}
