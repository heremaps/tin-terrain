#pragma once

#include <algorithm>

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || defined(__BIG_ENDIAN__) || \
    defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_MIBSEB) || \
    defined(__MIBSEB) || defined(__MIBSEB__)
#    define TNTN_BIG_ENDIAN
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || defined(__LITTLE_ENDIAN__) || \
    defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) || \
    defined(__MIPSEL) || defined(__MIPSEL__) || defined(_WIN32)
#    define TNTN_LITTLE_ENDIAN
#else
#    error unknown architecture
#endif

#if !defined(TNTN_LITTLE_ENDIAN) && !defined(TNTN_BIG_ENDIAN)
#    error neither little, nor big endian, platform not supported
#endif

namespace tntn {

enum class Endianness
{
    BIG,
    LITTLE,
};

#ifdef TNTN_BIG_ENDIAN
constexpr Endianness PLATFORM_NATIVE_ENDIANNESS = Endianness::BIG;
#else
constexpr Endianness PLATFORM_NATIVE_ENDIANNESS = Endianness::LITTLE;
#endif

inline void flip_endianness_4byte(unsigned char* data)
{
    std::swap(data[0], data[3]);
    std::swap(data[1], data[2]);
}

} // namespace tntn
