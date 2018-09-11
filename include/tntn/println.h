#pragma once

#include <string>
#include <sstream>
#include <vector>
#include "fmt/core.h"

namespace tntn {
namespace detail {

void print_impl(const std::string& s, bool newline);
void print_impl(const char* msg, bool newline);
void print_impl(const std::stringstream& s, bool newline);
void print_ptr_impl(const void* p, bool newline);

} //namespace detail

/**
 use the println function to output meaningful content for the consumption in stdout/in scripts
 adds a newline at the end of the line

 this function wraps fmt::format - it supports the libfmt format string syntax
 */

template<typename Arg0T, typename... Args>
inline void println(const char* fmtstr, const Arg0T& arg0, const Args&... args)
{
    detail::print_impl(::fmt::format(fmtstr, arg0, args...), true);
}
template<typename Arg0T, typename... Args>
inline void println(const std::string& fmtstr, const Arg0T& arg0, const Args&... args)
{
    detail::print_impl(::fmt::format(fmtstr, arg0, args...), true);
}

template<typename T>
inline void println(const T& t)
{
    detail::print_impl(static_cast<const std::stringstream&>(std::stringstream() << t), true);
}

inline void println(const std::stringstream& s)
{
    detail::print_impl(s, true);
}

inline void println(const std::string& s)
{
    detail::print_impl(s, true);
}

inline void println(const char* msg)
{
    detail::print_impl(msg, true);
}

inline void println()
{
    detail::print_impl(nullptr, true);
}

//delete overloads on simple types so that these doesn't expand to the std::stringstream<< version, use println("{}", i) instead
void println(bool) = delete;
void println(char) = delete;
void println(short) = delete;
void println(int) = delete;
void println(long) = delete;
void println(long long) = delete;
void println(unsigned char) = delete;
void println(unsigned short) = delete;
void println(unsigned) = delete;
void println(unsigned long) = delete;
void println(unsigned long long) = delete;
void println(float) = delete;
void println(double) = delete;
void println(long double) = delete;
void println(const void*) = delete;

/**
 same as println, but without the trailing newline
 */
template<typename Arg0T, typename... Args>
inline void print(const char* fmtstr, const Arg0T& arg0, const Args&... args)
{
    detail::print_impl(::fmt::format(fmtstr, arg0, args...), false);
}
template<typename Arg0T, typename... Args>
inline void print(const std::string& fmtstr, const Arg0T& arg0, const Args&... args)
{
    detail::print_impl(::fmt::format(fmtstr, arg0, args...), false);
}

template<typename T>
inline void print(const T& t)
{
    std::stringstream s;
    s << t;
    detail::print_impl(s, false);
}

inline void print(const std::stringstream& s)
{
    detail::print_impl(s, false);
}

inline void print(const std::string& s)
{
    detail::print_impl(s, false);
}

inline void print(const char* msg)
{
    detail::print_impl(msg, false);
}

//delete overloads on simple types so that these doesn't expand to the std::stringstream<< version, use print("{}", i) instead
void print(bool) = delete;
void print(char) = delete;
void print(short) = delete;
void print(int) = delete;
void print(long) = delete;
void print(long long) = delete;
void print(unsigned char) = delete;
void print(unsigned short) = delete;
void print(unsigned) = delete;
void print(unsigned long) = delete;
void print(unsigned long long) = delete;
void print(float) = delete;
void print(double) = delete;
void print(long double) = delete;
void print(const void*) = delete;

/**
 print raw bytes to the output stream, same as fwrite(...) to stdout
 */

void print_raw(const void* src, size_t src_size);

inline void print_raw(const std::string& src)
{
    print_raw(src.c_str(), src.size());
}

inline void print_raw(const std::vector<char>& src)
{
    print_raw(src.data(), src.size());
}

inline void print_raw(const std::vector<unsigned char>& src)
{
    print_raw(src.data(), src.size());
}

} // namespace tntn
