#include "tntn/BinaryIO.h"
#include "tntn/endianness.h"
#include "tntn/tntn_assert.h"

#include <algorithm>
#include <cstring>

namespace tntn {

std::string BinaryIOError::to_string() const
{
    if(!is_error())
    {
        return std::string();
    }

    std::string out;
    out += what == BinaryIOError::READ ? "reading failed at " : "writing failed at ";
    out += std::to_string(where);
    out += " expected ";
    out += std::to_string(expected_bytes);
    out += " bytes, got ";
    out += std::to_string(actual_bytes);
    out += " bytes on type ";
    out += type;

    return out;
}

//treat nullptrs as empty strings
static bool cstrs_equal_nullptr_tolerant(const char* a, const char* b)
{
    if(a == b)
    {
        return true;
    }
    else if(a == nullptr || b == nullptr)
    {
        return false;
    }
    else
    {
        return std::strcmp(a, b) == 0;
    }
}

bool BinaryIOError::operator==(const BinaryIOError& other) const
{
    return cstrs_equal_nullptr_tolerant(type, other.type) && where == other.where &&
        what == other.what && expected_bytes == other.expected_bytes &&
        actual_bytes == other.actual_bytes;
}

std::string BinaryIOErrorTracker::to_string() const
{
    std::string out;
    if(first_error.is_error() && first_error == last_error)
    {
        out += "error: ";
        out += first_error.to_string();
    }
    else
    {
        if(first_error.is_error())
        {
            out += "first error: ";
            out += first_error.to_string();
        }
        if(last_error.is_error())
        {
            out += out.empty() ? "last error: " : ", last error: ";
            out += last_error.to_string();
        }
    }
    return out;
}

//returns successfully read element count
size_t BinaryIO::read_impl(void* const dest,
                           size_t dest_elem_size,
                           size_t dest_count,
                           const char* const elem_type,
                           BinaryIOErrorTracker& e)
{
    TNTN_ASSERT(dest_elem_size > 0);
    TNTN_ASSERT(dest_count > 0);

    unsigned char* dest_c = reinterpret_cast<unsigned char*>(dest);
    const size_t bytes_to_read = dest_elem_size * dest_count;
    const size_t bytes_read = m_f->read(m_read_pos, dest_c, bytes_to_read);

    if(bytes_read != bytes_to_read)
    {
        e.last_error.type = elem_type;
        e.last_error.where = m_read_pos;
        e.last_error.what = BinaryIOError::READ;
        e.last_error.actual_bytes = bytes_read;
        e.last_error.expected_bytes = bytes_to_read;
        if(!e.first_error.type)
        {
            e.first_error = e.last_error;
        }
    }

    if(PLATFORM_NATIVE_ENDIANNESS != m_target_endianness)
    {
        for(unsigned char* p = dest_c; p < dest_c + bytes_to_read; p += dest_elem_size)
        {
            std::reverse(p, p + dest_elem_size);
        }
    }

    m_read_pos += bytes_read;

    return bytes_read / dest_elem_size;
}

void BinaryIO::write_impl(const void* const src,
                          size_t src_elem_size,
                          size_t src_count,
                          const char* const elem_type,
                          BinaryIOErrorTracker& e)
{
    TNTN_ASSERT(src_elem_size > 0);
    TNTN_ASSERT(src_elem_size <= 32);
    if(src_elem_size > 32)
    {
        return;
    }

    const size_t bytes_to_write = src_elem_size * src_count;
    const unsigned char* src_c = reinterpret_cast<const unsigned char*>(src);
    if(PLATFORM_NATIVE_ENDIANNESS == m_target_endianness)
    {
        //simple write
        const bool write_ok = m_f->write(m_write_pos, src_c, bytes_to_write);
        if(!write_ok)
        {
            e.last_error.type = elem_type;
            e.last_error.where = m_write_pos;
            e.last_error.what = BinaryIOError::WRITE;
            e.last_error.actual_bytes = 0;
            e.last_error.expected_bytes = bytes_to_write;
            if(!e.first_error.type)
            {
                e.first_error = e.last_error;
            }
        }
        if(write_ok)
        {
            m_write_pos += bytes_to_write;
        }
    }
    else
    {
        //copy to temporary storage, reverse, write
        unsigned char tmp[34];
        for(const unsigned char* p = src_c; p < src_c + bytes_to_write; p += src_elem_size)
        {
            memcpy(tmp, p, src_elem_size);
            std::reverse(tmp, tmp + src_elem_size);
            const bool write_ok = m_f->write(m_write_pos, tmp, src_elem_size);
            if(!write_ok)
            {
                e.last_error.type = elem_type;
                e.last_error.where = m_write_pos;
                e.last_error.what = BinaryIOError::WRITE;
                e.last_error.actual_bytes = 0;
                e.last_error.expected_bytes = bytes_to_write;
                if(!e.first_error.type)
                {
                    e.first_error = e.last_error;
                }
                break;
            }
            if(write_ok)
            {
                m_write_pos += bytes_to_write;
            }
        }
    }
}

} // namespace tntn
