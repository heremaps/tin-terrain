#pragma once

#include "tntn/endianness.h"
#include "tntn/File.h"

#include <memory>
#include <cstdint>
#include <vector>
#include <string>

namespace tntn {

struct BinaryIOError
{
    enum InOrOut
    {
        NONE,
        READ,
        WRITE
    };

    const char* type = nullptr;
    File::position_type where = -1;
    InOrOut what = NONE;
    size_t expected_bytes = 0;
    size_t actual_bytes = 0;

    std::string to_string() const;
    bool is_error() const { return type != nullptr; }

    bool operator==(const BinaryIOError& other) const;
    bool operator!=(const BinaryIOError& other) const { return !(*this == other); }
};

struct BinaryIOErrorTracker
{
    BinaryIOError first_error;
    BinaryIOError last_error;
    bool has_error() const { return first_error.is_error(); }
    std::string to_string() const;
};

class BinaryIO
{
  public:
    BinaryIO(std::shared_ptr<FileLike> f, Endianness target_endianness) :
        m_f(f),
        m_target_endianness(target_endianness)
    {
    }

    File::position_type read_pos() const noexcept { return m_read_pos; }
    File::position_type write_pos() const noexcept { return m_write_pos; }

    void read_skip(const File::position_type bytes, BinaryIOErrorTracker& e)
    {
        m_read_pos += bytes;
    }
    void read_byte(uint8_t& b, BinaryIOErrorTracker& e) { read_impl(&b, 1, 1, "uint8_t", e); }
    void read_uint16(uint16_t& o, BinaryIOErrorTracker& e) { read_impl(&o, 2, 1, "uint16_t", e); }
    void read_int16(int16_t& o, BinaryIOErrorTracker& e) { read_impl(&o, 2, 1, "int16_t", e); }
    void read_uint32(uint32_t& o, BinaryIOErrorTracker& e) { read_impl(&o, 4, 1, "uint32_t", e); }
    void read_int32(int32_t& o, BinaryIOErrorTracker& e) { read_impl(&o, 4, 1, "int32_t", e); }
    void read_array_uint16(std::vector<uint16_t>& o, size_t count, BinaryIOErrorTracker& e)
    {
        o.resize(count);
        const auto new_count = read_impl(o.data(), 2, count, "uint16_t[]", e);
        o.resize(new_count);
    }
    void read_array_int16(std::vector<int16_t>& o, size_t count, BinaryIOErrorTracker& e)
    {
        o.resize(count);
        const auto new_count = read_impl(o.data(), 2, count, "int16_t[]", e);
        o.resize(new_count);
    }
    void read_array_uint32(std::vector<uint32_t>& o, size_t count, BinaryIOErrorTracker& e)
    {
        o.resize(count);
        const auto new_count = read_impl(o.data(), 4, count, "uint32_t[]", e);
        o.resize(new_count);
    }
    void read_array_int32(std::vector<int32_t>& o, size_t count, BinaryIOErrorTracker& e)
    {
        o.resize(count);
        const auto new_count = read_impl(o.data(), 4, count, "int32_t[]", e);
        o.resize(new_count);
    }
    void read_float(float& o, BinaryIOErrorTracker& e) { read_impl(&o, 4, 1, "float", e); }
    void read_double(double& o, BinaryIOErrorTracker& e) { read_impl(&o, 8, 1, "double", e); }

    template<typename T>
    void read_array(std::vector<T>& o, size_t count, BinaryIOErrorTracker& e);

    void write_byte(const uint8_t v, BinaryIOErrorTracker& e)
    {
        write_impl(&v, 1, 1, "uint8_t", e);
    }
    void write_uint16(const uint16_t v, BinaryIOErrorTracker& e)
    {
        write_impl(&v, 2, 1, "uint16_t", e);
    }
    void write_int16(const int16_t v, BinaryIOErrorTracker& e)
    {
        write_impl(&v, 2, 1, "int16_t", e);
    }
    void write_uint32(const uint32_t v, BinaryIOErrorTracker& e)
    {
        write_impl(&v, 4, 1, "uint32_t", e);
    }
    void write_int32(const int32_t v, BinaryIOErrorTracker& e)
    {
        write_impl(&v, 4, 1, "int32_t", e);
    }
    void write_array_uint16(const std::vector<uint16_t>& v, BinaryIOErrorTracker& e)
    {
        write_impl(v.data(), 2, v.size(), "uint16_t[]", e);
    }
    void write_array_int16(const std::vector<int16_t>& v, BinaryIOErrorTracker& e)
    {
        write_impl(v.data(), 2, v.size(), "int16_t[]", e);
    }
    void write_array_uint32(const std::vector<uint32_t>& v, BinaryIOErrorTracker& e)
    {
        write_impl(v.data(), 4, v.size(), "uint32_t[]", e);
    }
    void write_array_int32(const std::vector<int32_t>& v, BinaryIOErrorTracker& e)
    {
        write_impl(v.data(), 4, v.size(), "int32_t[]", e);
    }
    void write_float(const float v, BinaryIOErrorTracker& e) { write_impl(&v, 4, 1, "float", e); }
    void write_double(const double& v, BinaryIOErrorTracker& e)
    {
        write_impl(&v, 8, 1, "double", e);
    }

    template<typename T>
    void write(const T& v, BinaryIOErrorTracker& e);

    template<typename T>
    void write_array(const std::vector<T>& v, BinaryIOErrorTracker& e);

  private:
    //returns successfully read element count
    size_t read_impl(void* const dest,
                     size_t dest_elem_size,
                     size_t dest_count,
                     const char* const elem_type,
                     BinaryIOErrorTracker& e);
    void write_impl(const void* const src,
                    size_t src_elem_size,
                    size_t src_count,
                    const char* const elem_type,
                    BinaryIOErrorTracker& e);

    const std::shared_ptr<FileLike> m_f;
    const Endianness m_target_endianness;
    File::position_type m_read_pos = 0;
    File::position_type m_write_pos = 0;
};

template<>
inline void BinaryIO::read_array<int16_t>(std::vector<int16_t>& o,
                                          size_t count,
                                          BinaryIOErrorTracker& e)
{
    return read_array_int16(o, count, e);
}
template<>
inline void BinaryIO::read_array<uint16_t>(std::vector<uint16_t>& o,
                                           size_t count,
                                           BinaryIOErrorTracker& e)
{
    return read_array_uint16(o, count, e);
}
template<>
inline void BinaryIO::read_array<int32_t>(std::vector<int32_t>& o,
                                          size_t count,
                                          BinaryIOErrorTracker& e)
{
    return read_array_int32(o, count, e);
}
template<>
inline void BinaryIO::read_array<uint32_t>(std::vector<uint32_t>& o,
                                           size_t count,
                                           BinaryIOErrorTracker& e)
{
    return read_array_uint32(o, count, e);
}

template<>
inline void BinaryIO::write_array<int16_t>(const std::vector<int16_t>& v, BinaryIOErrorTracker& e)
{
    return write_array_int16(v, e);
}
template<>
inline void BinaryIO::write_array<uint16_t>(const std::vector<uint16_t>& v,
                                            BinaryIOErrorTracker& e)
{
    return write_array_uint16(v, e);
}

template<>
inline void BinaryIO::write_array<int32_t>(const std::vector<int32_t>& v, BinaryIOErrorTracker& e)
{
    return write_array_int32(v, e);
}
template<>
inline void BinaryIO::write_array<uint32_t>(const std::vector<uint32_t>& v,
                                            BinaryIOErrorTracker& e)
{
    return write_array_uint32(v, e);
}

template<>
inline void BinaryIO::write(const int16_t& v, BinaryIOErrorTracker& e)
{
    return write_int16(v, e);
}
template<>
inline void BinaryIO::write(const uint16_t& v, BinaryIOErrorTracker& e)
{
    return write_uint16(v, e);
}
template<>
inline void BinaryIO::write(const int32_t& v, BinaryIOErrorTracker& e)
{
    return write_int32(v, e);
}
template<>
inline void BinaryIO::write(const uint32_t& v, BinaryIOErrorTracker& e)
{
    return write_uint32(v, e);
}

} // namespace tntn
