#include "tntn/logging.h"

#include <cstdio>
#include <atomic>

namespace tntn {

#if(defined(DEBUG) || defined(TNTN_DEBUG)) && !defined(NDEBUG)
constexpr LogLevel GLOBAL_DEFAULT_LOG_LEVEL = LogLevel::DEBUG;
#else
constexpr LogLevel GLOBAL_DEFAULT_LOG_LEVEL = LogLevel::INFO;
#endif

static std::atomic<LogLevel> g_global_log_level = {GLOBAL_DEFAULT_LOG_LEVEL};
static std::atomic<LogStream> g_global_log_stream = {LogStream::STDERR};

void log_set_global_logstream(LogStream ls)
{
    g_global_log_stream = ls;
}

void log_set_global_level(LogLevel lvl)
{
    g_global_log_level = lvl;
}

LogLevel log_get_global_level()
{
    return g_global_log_level;
}

LogLevel log_decrease_global_level()
{
    LogLevel l = g_global_log_level;
    int li = static_cast<int>(l);

    if(li > 0)
    {
        li--;
        l = static_cast<LogLevel>(li);
        g_global_log_level = l;
    }
    return l;
}

static const char* loglevel_to_str(const LogLevel lvl)
{
    switch(lvl)
    {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "";
    }
}

void log_message(LogLevel lvl, const char* filename, const int line, const std::string& message)
{
    const LogLevel global_lvl = g_global_log_level;
    const LogStream global_ls = g_global_log_stream;

    std::FILE* const log_stream = global_ls == LogStream::STDOUT
        ? stdout
        : (global_ls == LogStream::STDERR ? stderr : nullptr);

    if(global_lvl <= lvl && !message.empty() && log_stream)
    {
        if(lvl == LogLevel::INFO)
        {
            std::fprintf(log_stream, "%s\n", message.c_str());
        }
        else
        {
            const char* filename_last_component = strrchr(filename, '/');
            if(filename_last_component)
            {
                filename = filename_last_component + 1;
            }
            std::fprintf(log_stream,
                         "%s %s:%d %s\n",
                         loglevel_to_str(lvl),
                         filename,
                         line,
                         message.c_str());
        }
        std::fflush(log_stream);
    }
}

} //namespace tntn
