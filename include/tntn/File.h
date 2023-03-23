#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#ifdef _MSC_VER 
#define ftello ftell
#endif

namespace tntn {

class FileLike
{
  private:
    //disallow copy and assign
    FileLike(const FileLike& other) = delete;
    FileLike& operator=(const FileLike& other) = delete;

  public:
    FileLike() = default;
    virtual ~FileLike() = default;

    typedef uint64_t position_type;

    /**
     get an optional describing string
     for a real File it should be a filename
     for other implementations the meaning is implementation specific
     */
    virtual std::string name() const = 0;

    /**
     check whether the object can be used (read from, written to)
     implementations might set the internal is_good flag to false on
     failing write and/or read operations.
     */
    virtual bool is_good() = 0;

    /**
     get the number of bytes currently in this object
     keep in mind that this value is allways an unsigned 64bit type while
     on 32bit platforms size_t is only 32bit.
     */
    virtual position_type size() = 0;

    /**
     read data
     @param from_offset offset against the beginning of the file-like, 0 is the beginning.
     @return the actual number of bytes read, returns less then size_of_buffer on eof or read error
     */
    virtual size_t read(position_type from_offset,
                        unsigned char* buffer,
                        size_t size_of_buffer) = 0;

    /**
     write data

     @param to_offset offset against the beginning of the file, can be > size in which case the internal object will be expanded
     @return success/failure
     */
    virtual bool write(position_type to_offset, const unsigned char* data, size_t data_size) = 0;

    virtual void flush() = 0;

    //convenience wrappers:

    size_t read(position_type from_offset, char* buffer, size_t size_of_buffer)
    {
        return read(from_offset, reinterpret_cast<unsigned char*>(buffer), size_of_buffer);
    }
    void read(position_type from_offset, std::vector<char>& buffer, size_t max_bytes)
    {
        buffer.resize(max_bytes);
        size_t bytes_read = read(from_offset, buffer.data(), max_bytes);
        buffer.resize(bytes_read);
    }
    void read(position_type from_offset, std::vector<unsigned char>& buffer, size_t max_bytes)
    {
        buffer.resize(max_bytes);
        size_t bytes_read = read(from_offset, buffer.data(), max_bytes);
        buffer.resize(bytes_read);
    }
    void read(position_type from_offset, std::string& buffer, size_t max_bytes)
    {
        buffer.resize(max_bytes);
        if(max_bytes > 0)
        {
            size_t bytes_read = read(from_offset, &buffer[0], max_bytes);
            buffer.resize(bytes_read);
        }
    }

    bool write(position_type to_offset, const char* data, size_t data_size)
    {
        return write(to_offset, reinterpret_cast<const unsigned char*>(data), data_size);
    }
    bool write(position_type to_offset, const std::vector<char>& data)
    {
        return write(to_offset, data.data(), data.size());
    }
    bool write(position_type to_offset, const std::vector<unsigned char>& data)
    {
        return write(to_offset, data.data(), data.size());
    }
    bool write(position_type to_offset, const std::string& data)
    {
        return write(to_offset, data.data(), data.size());
    }
};

class File : public FileLike
{
  public:
    enum OpenMode
    {
        OM_NONE,
        OM_R, //read
        OM_RW, //read-write
        OM_RWC, //read-write-create (not overwriting aka exclusive create)
        OM_RWCF, //read-write, force-create (overwrite when existing)
    };
    bool open(const char* filename, OpenMode open_mode);
    bool open(const std::string& filename, OpenMode open_mode);

    bool close();

    File();
    ~File();

    std::string name() const override;

    bool is_good() override;
    position_type size() override;

    size_t read(position_type from_offset, unsigned char* buffer, size_t size_of_buffer) override;
    using FileLike::read; //import convenience overloads

    bool write(position_type to_offset, const unsigned char* data, size_t data_size) override;
    using FileLike::write; //import convenience overloads

    void flush() override;

  private:
    class FileImpl;
    std::unique_ptr<FileImpl> m_impl;
};

class MemoryFile : public FileLike
{
  public:
    std::string name() const override { return std::string(); }

    bool is_good() override;
    position_type size() override;

    size_t read(position_type from_offset, unsigned char* buffer, size_t size_of_buffer) override;
    using FileLike::read; //import convenience overloads

    bool write(position_type to_offset, const unsigned char* data, size_t data_size) override;
    using FileLike::write; //import convenience overloads

    void flush() override {}

  private:
    std::vector<unsigned char> m_data;
    bool m_is_good = true;
};

FileLike::position_type getline(FileLike::position_type from_offset,
                                FileLike& f,
                                std::string& str);
FileLike::position_type getline(FileLike::position_type from_offset,
                                const std::shared_ptr<FileLike>& f,
                                std::string& str);

} //namespace tntn
