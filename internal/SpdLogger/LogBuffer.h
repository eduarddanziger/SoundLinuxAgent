#pragma once

#include <vector>
#include <mutex>
#include <sstream>

#include <spdlog/sinks/sink.h>

namespace ed::model
{
    class LogBuffer final : public spdlog::sinks::sink
    {
    public:
        LogBuffer() = default;
        std::vector<std::string> GetAndClearNextQueueChunk();

        void log(const spdlog::details::log_msg& msg) override;

        void flush() override
        {
        }

        void set_pattern([[maybe_unused]] const std::string& pattern) override
        {
        }

        void set_formatter([[maybe_unused]] std::unique_ptr<spdlog::formatter> sink_formatter) override
        {
        }

    protected:
        void Put(const std::string& val);
    private:
        std::vector<std::string> m_queue;
        mutable std::recursive_mutex m_queueGuard;
    };
}


inline std::vector<std::string> ed::model::LogBuffer::GetAndClearNextQueueChunk()
{
    std::vector<std::string> result;

    std::lock_guard lock(m_queueGuard);

    result.swap(m_queue);
    return result;
}

inline void ed::model::LogBuffer::log(const spdlog::details::log_msg& msg)
{
    const std::string text(msg.payload.begin(), msg.payload.end());
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line))
    {
        Put(line);
    }
}

inline void ed::model::LogBuffer::Put(const std::string& val)
{
    std::lock_guard<std::recursive_mutex> lock(m_queueGuard);
    m_queue.push_back(val);
}

