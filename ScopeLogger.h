#pragma once

#include <spdlog/spdlog.h>
#include <string_view>


// RAII-style class that logs entry and exit automatically
class ScopeLogger {
public:
    ScopeLogger(const std::string_view& function) : function_(function) {
        spdlog::debug("ENTER: {}", function_);
    }
    
    ~ScopeLogger() {
        spdlog::debug("EXIT: {}", function_);
    }
    
private:
    std::string_view function_;
};

// Auto-named scope logger using RAII
#define LOG_SCOPE() ScopeLogger scope_logger_##__LINE__(__PRETTY_FUNCTION__)