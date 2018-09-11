#include "tntn/File.h"
#include "tntn/logging.h"

#include <cstdio>
#include <errno.h>
#include <limits>

namespace tntn {

static constexpr size_t max_read_write_chunk_size = std::numeric_limits<int>::max();

static const char* openmode_to_fopen_mode(File::OpenMode open_mode)
{
    switch(open_mode)
    {
        case File::OM_R: return "r";
        case File::OM_RW: return "r+";
        case File::OM_RWC: return "w+x";
        case File::OM_RWCF: return "w+";
        default: return "r";
    }
}

class File::FileImpl
{
  public:
    bool open(const char* filename, OpenMode open_mode)
    {
        close();
        const char* const omstr = openmode_to_fopen_mode(open_mode);
        FILE* fp = fopen(filename, omstr);
        const auto err = errno;
        TNTN_LOG_TRACE("fopen({}, {}) = {} / errno = {}", filename, omstr, fp != nullptr, err);
        m_fp = fp;
        m_is_good = fp != nullptr;
        m_size = get_size();
        m_filename = filename;
        return is_good();
    }

    bool close()
    {
        int rc = 0;
        if(m_fp != nullptr)
        {
            rc = fclose(m_fp);
            const auto err = errno;
            if(rc != 0)
            {
                TNTN_LOG_DEBUG("fclose on {} failed with errno {}", m_filename, err);
            }
            m_fp = nullptr;
            m_size = 0;
            m_is_good = false;
            m_filename.clear();
        }
        return rc == 0;
    }

    std::string name() const { return m_filename; }

    bool is_good() { return m_fp != nullptr && m_is_good; }

    position_type size() { return m_size; }

    size_t read(position_type from_offset, unsigned char* buffer, size_t size_of_buffer)
    {
        if(!is_good())
        {
            return 0;
        }
        if(!seek_to_position(from_offset))
        {
            m_is_good = false;
            return 0;
        }
        if(size_of_buffer > max_read_write_chunk_size)
        {
            TNTN_LOG_ERROR("huge read > INT_MAX bytes, won't do that");
            return 0;
        }

        const size_t rc = fread(buffer, 1, size_of_buffer, m_fp);
        return rc;
    }

    bool write(position_type to_offset, const unsigned char* data, size_t data_size)
    {
        if(!is_good())
        {
            return false;
        }
        if(data_size > max_read_write_chunk_size)
        {
            TNTN_LOG_ERROR("huge write > INT_MAX bytes, won't do that");
            m_is_good = false;
            return false;
        }
        if(!seek_to_position(to_offset))
        {
            m_is_good = false;
            return false;
        }

        const size_t rc = fwrite(data, 1, data_size, m_fp);
        const auto err = errno;
        if(rc != data_size)
        {
            TNTN_LOG_ERROR(
                "unable to write {} bytes into file {}, errno = {}", data_size, m_filename, err);
            m_is_good = false;
            m_size = get_size();
            return false;
        }
        else
        {
            //rc == data_size
            if(to_offset + static_cast<position_type>(data_size) > m_size)
            {
                m_size = to_offset + data_size;
            }
            return true;
        }
    }

    void flush()
    {
        if(m_fp)
        {
            int rc = fflush(m_fp);
            const auto err = errno;
            if(rc != 0)
            {
                TNTN_LOG_ERROR("fflush failed, errno = {}", m_filename, err);
            }
        }
    }

  private:
    bool seek_to_position(position_type position)
    {
        int rc = fseek(m_fp, 0, SEEK_SET);
        auto err = errno;
        while(rc == 0 && position > 0)
        {
            if(position > std::numeric_limits<long>::max())
            {
                rc = fseek(m_fp, std::numeric_limits<long>::max(), SEEK_CUR);
                err = errno;
                position -= std::numeric_limits<long>::max();
            }
            else
            {
                rc = fseek(m_fp, static_cast<long>(position), SEEK_CUR);
                err = errno;
                position = 0;
            }
        }
        if(rc == 0 && position == 0)
        {
            return true;
        }
        else
        {
            TNTN_LOG_ERROR("unable to fseek correctly in file {}, errno = {}", m_filename, err);
            return false;
        }
    }

    position_type get_size()
    {
        if(!m_fp)
        {
            return 0;
        }
        int rc = fseek(m_fp, 0, SEEK_END);
        auto err = errno;
        if(rc == 0)
        {
            const off_t end_pos = ftello(m_fp);
            if(end_pos >= 0)
            {
                return static_cast<position_type>(end_pos);
            }
        }
        else
        {
            TNTN_LOG_ERROR("unable to fseek correctly in file {}, errno = {}", m_filename, err);
            m_is_good = false;
        }
        return 0;
    }

    FILE* m_fp = nullptr;
    position_type m_size = 0;
    bool m_is_good = false;
    std::string m_filename;
};

File::File() : m_impl(std::make_unique<FileImpl>()) {}

File::~File() = default;

bool File::open(const char* filename, OpenMode open_mode)
{
    return m_impl->open(filename, open_mode);
}
bool File::open(const std::string& filename, OpenMode open_mode)
{
    return open(filename.c_str(), open_mode);
}

bool File::close()
{
    return m_impl->close();
}

std::string File::name() const
{
    return m_impl->name();
}

bool File::is_good()
{
    return m_impl->is_good();
}
FileLike::position_type File::size()
{
    return m_impl->size();
}
size_t File::read(position_type from_offset, unsigned char* buffer, size_t size_of_buffer)
{
    return m_impl->read(from_offset, buffer, size_of_buffer);
}
bool File::write(position_type to_offset, const unsigned char* data, size_t data_size)
{
    return m_impl->write(to_offset, data, data_size);
}
void File::flush()
{
    return m_impl->flush();
}

bool MemoryFile::is_good()
{
    return m_is_good;
}

FileLike::position_type MemoryFile::size()
{
    return m_data.size();
}

size_t MemoryFile::read(position_type from_offset_ui64,
                        unsigned char* buffer,
                        size_t size_of_buffer)
{
    if(!m_is_good)
    {
        return false;
    }
    if(!buffer)
    {
        TNTN_LOG_ERROR("buffer is NULL");
        return false;
    }
    if(size_of_buffer == 0)
    {
        return 0;
    }
    if(from_offset_ui64 > std::numeric_limits<size_t>::max())
    {
        TNTN_LOG_ERROR("unable to read more then SIZE_T_MAX bytes from MemoryFile");
        return 0;
    }
    if(size_of_buffer > max_read_write_chunk_size)
    {
        TNTN_LOG_ERROR("huge read > INT_MAX bytes, won't do that");
        return 0;
    }

    const size_t from_offset = static_cast<size_t>(from_offset_ui64);

    size_t bytes_to_read = 0;
    if(from_offset > m_data.size())
    {
        m_is_good = false;
        return 0;
    }
    else if(from_offset + size_of_buffer > m_data.size())
    {
        bytes_to_read = m_data.size() - from_offset;
    }
    else
    {
        bytes_to_read = size_of_buffer;
    }

    memcpy(buffer, m_data.data() + from_offset, bytes_to_read);
    return bytes_to_read;
}

bool MemoryFile::write(position_type to_offset_ui64, const unsigned char* data, size_t data_size)
{
    if(!m_is_good)
    {
        return false;
    }
    if(!data || data_size == 0)
    {
        return true;
    }
    if(to_offset_ui64 > std::numeric_limits<size_t>::max())
    {
        TNTN_LOG_ERROR("unable to write more then SIZE_T_MAX bytes into MemoryFile");
        return false;
    }
    if(data_size > max_read_write_chunk_size)
    {
        TNTN_LOG_ERROR("huge write > INT_MAX bytes, won't do that");
        m_is_good = false;
        return false;
    }

    const size_t to_offset = static_cast<size_t>(to_offset_ui64);

    if(to_offset + data_size > m_data.size())
    {
        try
        {
            m_data.resize(to_offset + data_size);
        }
        catch(const std::bad_alloc&)
        {
            TNTN_LOG_ERROR("unabled to resize MemoryFile to {}, std::bad_alloc",
                           to_offset + data_size);
            return false;
        }
    }
    memcpy(m_data.data() + to_offset, data, data_size);
    return data_size;
}

FileLike::position_type getline(FileLike::position_type from_offset,
                                FileLike& f,
                                std::string& str)
{
    str.clear();
    if(!f.is_good())
    {
        TNTN_LOG_ERROR("file-like object is not in a good state, cannot getline");
        return 0;
    }
    const char delim = '\n';
    //read in chunks of 4k
    constexpr size_t read_chunk_size = 4 * 1024;
    std::vector<char> tmp;
    tmp.reserve(read_chunk_size);

    bool delim_found = false;
    bool has_eof = false;
    auto read_position = from_offset;
    while(!delim_found && !has_eof)
    {
        f.read(read_position, tmp, read_chunk_size);
        read_position += tmp.size();
        if(tmp.size() != read_chunk_size)
        {
            //eof or read error
            has_eof = true;
        }
        size_t copy_count = 0;
        //scan for delimiter
        for(; copy_count < tmp.size() && !delim_found; copy_count++)
        {
            delim_found = tmp[copy_count] == delim;
        }
        copy_count -= delim_found ? 1 : 0; //ignore delimiter
        //copy over chunk
        if(copy_count > 0)
        {
            const size_t old_str_size = str.size();
            str.resize(old_str_size + copy_count);
            memcpy(&str[old_str_size], &tmp[0], copy_count);
        }
    }
    return from_offset + str.size() + (delim_found ? 1 : 0);
}

FileLike::position_type getline(FileLike::position_type from_offset,
                                const std::shared_ptr<FileLike>& f,
                                std::string& str)
{
    if(!f)
    {
        TNTN_LOG_ERROR("cannot getline from file-like object that is NULL");
        return 0;
    }
    return getline(from_offset, *f, str);
}

} // namespace tntn
