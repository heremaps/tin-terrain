#pragma once

#include "fmt/core.h"
#include <string>
#include <string.h>

namespace tntn {
enum class LogLevel
{
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5,
};

enum class LogStream
{
    NONE,
    STDOUT,
    STDERR,
};

void log_set_global_logstream(LogStream ls);
void log_set_global_level(LogLevel lvl);
LogLevel log_get_global_level();
LogLevel log_decrease_global_level();
void log_message(LogLevel lvl, const char* filename, const int line, const std::string& message);

} //namespace tntn

//trace log messages are fully disabled in non-debug builds to not interfere with performance sensitive code
#ifdef TNTN_DEBUG
#    define TNTN_LOG_TRACE(fmtstr, ...) \
        ::tntn::log_message( \
            ::tntn::LogLevel::TRACE, __FILE__, __LINE__, ::fmt::format(fmtstr, ##__VA_ARGS__))
#else
#    define TNTN_LOG_TRACE(fmtstr, ...)
#endif

#define TNTN_LOG_DEBUG(fmtstr, ...) \
    ::tntn::log_message( \
        ::tntn::LogLevel::DEBUG, __FILE__, __LINE__, ::fmt::format(fmtstr, ##__VA_ARGS__))
#define TNTN_LOG_INFO(fmtstr, ...) \
    ::tntn::log_message( \
        ::tntn::LogLevel::INFO, __FILE__, __LINE__, ::fmt::format(fmtstr, ##__VA_ARGS__))
#define TNTN_LOG_WARN(fmtstr, ...) \
    ::tntn::log_message( \
        ::tntn::LogLevel::WARN, __FILE__, __LINE__, ::fmt::format(fmtstr, ##__VA_ARGS__))
#define TNTN_LOG_ERROR(fmtstr, ...) \
    ::tntn::log_message( \
        ::tntn::LogLevel::ERROR, __FILE__, __LINE__, ::fmt::format(fmtstr, ##__VA_ARGS__))
#define TNTN_LOG_FATAL(fmtstr, ...) \
    ::tntn::log_message( \
        ::tntn::LogLevel::FATAL, __FILE__, __LINE__, ::fmt::format(fmtstr, ##__VA_ARGS__))
