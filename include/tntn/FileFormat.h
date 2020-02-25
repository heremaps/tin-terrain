#pragma once

#include <string>
#include <string.h>
#include "tntn/MeshMode.h"

#ifdef _MSC_VER 
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

namespace tntn {

class FileFormat
{
  public:
    enum Value
    {
        NONE = 0,
        OFF = 1,
        OBJ = 2,
        ASC = 3,
        XYZ = 4,
        TERRAIN = 5,
        JSON = 6,
        GEOJSON = 7,
        TIFF = 8,
        TIF = 9,
    };

    FileFormat() = default;
    FileFormat(const FileFormat& f) = default;
    FileFormat& operator=(const FileFormat& f) = default;

    FileFormat(const Value v) : m_value(v) {}
    FileFormat& operator=(const Value v) noexcept
    {
        m_value = v;
        return *this;
    }

    bool operator==(const Value v) const noexcept { return m_value == v; }
    bool operator!=(const Value v) const noexcept { return m_value != v; }
    bool operator==(const FileFormat& f) const noexcept { return m_value == f.m_value; }
    bool operator!=(const FileFormat& f) const noexcept { return m_value != f.m_value; }

    //for use in std::map etc.
    bool operator<(const FileFormat& f) const noexcept
    {
        return static_cast<int>(m_value) < static_cast<int>(f.m_value);
    }

    std::string to_string() const { return std::string(to_cstring()); }

    const char* to_cstring() const noexcept
    {
        switch(m_value)
        {
            case NONE: return "";
            case OFF: return "off";
            case OBJ: return "obj";
            case ASC: return "asc";
            case XYZ: return "xyz";
            case TERRAIN: return "terrain";
            case JSON: return "json";
            case GEOJSON: return "geojson";
            case TIFF: return "tiff";
            case TIF: return "tif";
        }
        return "";
    }

    static FileFormat from_string(const std::string& s) noexcept
    {
        return from_string(s.c_str());
    }
    static FileFormat from_string(const char* s) noexcept
    {
        if(strcasecmp(s, "off") == 0)
            return OFF;
        else if(strcasecmp(s, "obj") == 0)
            return OBJ;
        else if(strcasecmp(s, "asc") == 0)
            return ASC;
        else if(strcasecmp(s, "xyz") == 0)
            return XYZ;
        else if(strcasecmp(s, "terrain") == 0)
            return TERRAIN;
        else if(strcasecmp(s, "json") == 0)
            return JSON;
        else if(strcasecmp(s, "geojson") == 0)
            return GEOJSON;
        else if(strcasecmp(s, "tiff") == 0)
            return TIFF;
        else if(strcasecmp(s, "tif") == 0)
            return TIF;
        else
            return NONE;
    }

    static FileFormat from_fileext(const std::string& s) noexcept
    {
        return from_fileext(s.c_str());
    }
    static FileFormat from_fileext(const char* s) noexcept
    {
        if(*s == '.')
        {
            return from_string(s + 1);
        }
        else
        {
            return from_string(s);
        }
    }

    MeshMode optimal_mesh_mode() const
    {
        switch(m_value)
        {
            case OBJ: //fallthrough
            case OFF: //fallthrough
            case TERRAIN: return MeshMode::decomposed;
            default: return MeshMode::none;
        }
    }

  private:
    Value m_value = NONE;
};

} //namespace tntn
