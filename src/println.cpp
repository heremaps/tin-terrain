#include "tntn/println.h"

#include <cstdio>

namespace tntn {
namespace detail {

void print_impl(const std::string& msg, bool newline)
{
    if(newline && !msg.empty())
    {
        std::fwrite(msg.c_str(), msg.size(), 1, stdout);
        std::puts("");
    }
    else if(newline && msg.empty())
    {
        std::puts("");
    }
    else if(!newline && !msg.empty())
    {
        std::fwrite(msg.c_str(), msg.size(), 1, stdout);
    } //else if(!newline && msg.empty()) {
    // do nothing
    //}
}

void print_impl(const char* msg, bool newline)
{
    if(newline && msg)
    {
        std::printf("%s\n", msg);
    }
    else if(newline && !msg)
    {
        std::puts("");
    }
    else if(!newline && msg)
    {
        auto len = std::strlen(msg);
        std::fwrite(msg, len, 1, stdout);
    } //else if(!newline && !msg) {
    // do nothing
    //}
}

void print_impl(const std::stringstream& msg, bool newline)
{
    print_impl(msg.str(), newline);
}

void print_ptr_impl(const void* p, bool newline)
{
    if(newline)
    {
        std::printf("%p\n", p);
    }
    else
    {
        std::printf("%p", p);
    }
}

void print_raw(const void* src, size_t src_size)
{
    if(src && src_size)
    {
        std::fwrite(src, src_size, 1, stdout);
    }
}

} //namespace detail
} //namespace tntn
