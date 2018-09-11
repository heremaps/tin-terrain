#include "tntn/MeshIO.h"
#include "tntn/OFFReader.h"
#include "tntn/logging.h"
#include "tntn/File.h"
#include "tntn/QuantizedMeshIO.h"

#include "fmt/format.h"
#include <iostream>
#include <fstream>
#include <set>

namespace tntn {

bool write_mesh_to_file(const char* filename, const Mesh& m, const FileFormat& f)
{
    if(f == FileFormat::OFF)
    {
        return write_mesh_as_off(filename, m);
    }
    else if(f == FileFormat::OBJ)
    {
        return write_mesh_as_obj(filename, m);
    }
    else if(f == FileFormat::TERRAIN)
    {
        return write_mesh_as_qm(filename, m);
    }
    else if(f == FileFormat::JSON || f == FileFormat::GEOJSON)
    {
        return write_mesh_as_geojson(filename, m);
    }
    else
    {
        TNTN_LOG_ERROR("unsupported file format {} for mesh output", f.to_cstring());
        return false;
    }
}

std::unique_ptr<Mesh> load_mesh_from_obj(const char* filename)
{
    auto mesh = std::make_unique<Mesh>();

    std::ifstream f(filename);
    if(!f.is_open())
    {
        return std::unique_ptr<Mesh>();
    }

    std::vector<Vertex> vertices;
    std::vector<Face> faces;

    char t;
    double x, y, z;
    while(f >> t >> x >> y >> z)
    {
        if(t == 'v')
        {
            vertices.push_back({x, y, z});
        }
        else if(t == 'f')
        {
            const size_t sx = static_cast<size_t>(x);
            const size_t sy = static_cast<size_t>(y);
            const size_t sz = static_cast<size_t>(z);

            faces.push_back({{sx - 1, sy - 1, sz - 1}});
        }
    }

    mesh->from_decomposed(std::move(vertices), std::move(faces));

    return mesh;
}

std::string make_geojson_face(const Vertex& v1, const Vertex& v2, const Vertex& v3)
{
    std::string format_string =
        "{{\n \"type\" : \"Feature\" , \"properties\" : {{ \"id\" : 0 }} , \"geometry\" :\n {{ \n \"type\" :  \
                                \"LineString\", \"coordinates\" : \n \
                                [ \n [ {:.18f} , {:.18f} ], \n [ {:.18f}, {:.18f} ], \n [ {:.18f}, {:.18f} ],\n [ {:.18f}, {:.18f} ] \
                                \n ] \n }} \n }} \n";

    //std::string format_string = "[ {:.18f} , {:.18f} ], \n [ {:.18f}, {:.18f} ], \n [ {:.18f}, {:.18f} ],\n [ {:.18f}, {:.18f} ]";

    return fmt::format(format_string, v1.x, v1.y, v2.x, v2.y, v3.x, v3.y, v1.x, v1.y);
}

std::string make_geojson_vertex(const Vertex& v)
{
    std::string format_string =
        "{{ \n  \
        \"type\": \"Feature\",\n \
        \"properties\": {{}},\n \
        \"geometry\": {{\n \
            \"type\": \"Point\",\n \
            \"coordinates\": [\n \
                            {:.18f}, \n \
                            {:.18f} \n \
                            ]\n \
        }} \
    }}";

    return fmt::format(format_string, v.x, v.y);
}

bool write_mesh_as_geojson(FileLike& out_file, const Mesh& m)
{
    if(!m.has_decomposed())
    {
        TNTN_LOG_ERROR("mesh is not in decomposed format, please decompose first");
        return false;
    }

    auto faces_range = m.faces();
    auto vertices_range = m.vertices();

    fmt::memory_buffer line_buffer;
    line_buffer.reserve(1000);

    File::position_type write_pos = 0;

    line_buffer.resize(0);
    fmt::format_to(line_buffer, "{{\n");
    fmt::format_to(line_buffer, "\"type\": \"FeatureCollection\",\n");
    fmt::format_to(
        line_buffer,
        "\"crs\": {{ \"type\": \"name\", \"properties\": {{ \"name\": \"urn:ogc:def:crs:OGC:1.3:CRS84\" }} }},\n");
    fmt::format_to(line_buffer, "\"features\": [\n");
    if(!out_file.write(write_pos, line_buffer.data(), line_buffer.size())) return false;
    write_pos += line_buffer.size();

    TNTN_LOG_INFO("number of faces {}", faces_range.distance());

    int count = 0;
    //for (const Face *f = faces_range.begin; f != faces_range.end; f++)

    // write points
    for(int i = 0; i < vertices_range.distance(); i++)
    {
        const Vertex& v = vertices_range.begin[i];

        line_buffer.resize(0);

        std::string vertex_string = make_geojson_vertex(v);

        fmt::format_to(line_buffer, "{},", vertex_string);

        if(!out_file.write(write_pos, line_buffer.data(), line_buffer.size())) return false;

        write_pos += line_buffer.size();
    }

    // write triangels
    for(int i = 0; i < faces_range.distance(); i++)
    {

        TNTN_LOG_TRACE("write face {}...", i);

        const Face& face = faces_range.begin[i];

        line_buffer.resize(0);

        const Vertex& v1 = vertices_range.begin[face[0]];
        const Vertex& v2 = vertices_range.begin[face[1]];
        const Vertex& v3 = vertices_range.begin[face[2]];

        std::string face_string = make_geojson_face(v1, v2, v3);

        if(i == faces_range.distance() - 1)
        {
            TNTN_LOG_DEBUG("write end seg v2-v3");
            fmt::format_to(line_buffer, "{} \n ] \n }}", face_string);
        }
        else
        {
            fmt::format_to(line_buffer, "{},", face_string);
        }

        if(!out_file.write(write_pos, line_buffer.data(), line_buffer.size())) return false;

        write_pos += line_buffer.size();

        TNTN_LOG_DEBUG("done");
    }

    out_file.flush();

    TNTN_LOG_DEBUG("lines: {}", count);
    TNTN_LOG_DEBUG("write complete");

    return out_file.is_good();
}

bool write_mesh_as_geojson(const char* filename, const Mesh& m)
{
    File f;
    if(!f.open(filename, File::OM_RWCF))
    {
        return false;
    }
    return write_mesh_as_geojson(f, m);
}

bool write_mesh_as_obj(const char* filename, const Mesh& m)
{
    File f;
    if(!f.open(filename, File::OM_RWCF))
    {
        return false;
    }
    return write_mesh_as_obj(f, m);
}

bool write_mesh_as_obj(FileLike& out_file, const Mesh& m)
{
    if(!m.has_decomposed())
    {
        TNTN_LOG_ERROR("mesh is not in decomposed format, please decompose first");
        return false;
    }

    auto faces_range = m.faces();
    auto vertices_range = m.vertices();

    fmt::memory_buffer line_buffer;
    line_buffer.reserve(128);

    File::position_type write_pos = 0;
    for(const Vertex* v = vertices_range.begin; v != vertices_range.end; v++)
    {
        line_buffer.resize(0);
        fmt::format_to(line_buffer, "v {:.18f} {:.18f} {:.18f}\n", v->x, v->y, v->z);
        if(!out_file.write(write_pos, line_buffer.data(), line_buffer.size()))
        {
            return false;
        }
        write_pos += line_buffer.size();
    }

    for(const Face* f = faces_range.begin; f != faces_range.end; f++)
    {
        line_buffer.resize(0);
        fmt::format_to(line_buffer, "f {} {} {}\n", (*f)[0] + 1, (*f)[1] + 1, (*f)[2] + 1);
        if(!out_file.write(write_pos, line_buffer.data(), line_buffer.size()))
        {
            return false;
        }
        write_pos += line_buffer.size();
    }

    return out_file.is_good();
}

std::unique_ptr<Mesh> load_mesh_from_off(const char* filename)
{
    File f;
    if(!f.open(filename, File::OM_R))
    {
        TNTN_LOG_ERROR("unable to open input file {}", filename);
        return std::unique_ptr<Mesh>();
    }
    return load_mesh_from_off(f);
}

std::unique_ptr<Mesh> load_mesh_from_off(FileLike& f)
{
    OFFReader reader;

    if(!reader.readFile(f))
    {
        TNTN_LOG_ERROR("unable to read input from FileLike with name {}", f.name());
        return std::unique_ptr<Mesh>();
    }

    return reader.convertToMesh();
}

struct EdgeCompareLess
{
    bool operator()(const std::pair<VertexIndex, VertexIndex>& left,
                    const std::pair<VertexIndex, VertexIndex>& right) const
    {
        const VertexIndex lmin = std::min(left.first, left.second);
        const VertexIndex lmax = std::max(left.first, left.second);

        const VertexIndex rmin = std::min(right.first, right.second);
        const VertexIndex rmax = std::max(right.first, right.second);

        if(lmin < rmin)
        {
            return true;
        }
        else if(lmin > rmin)
        {
            return false;
        }
        else /*if(lmin == rmin)*/
        {
            if(lmax < rmax)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
};

static size_t calculate_num_edges(const SimpleRange<const Face*> faces)
{

    std::set<std::pair<VertexIndex, VertexIndex>, EdgeCompareLess> edges;

    for(const Face* fp = faces.begin; fp != faces.end; fp++)
    {
        edges.insert(std::make_pair((*fp)[0], (*fp)[1]));
        edges.insert(std::make_pair((*fp)[1], (*fp)[2]));
        edges.insert(std::make_pair((*fp)[2], (*fp)[0]));
    }

    return edges.size();
}

bool write_mesh_as_off(const char* filename, const Mesh& m)
{
    if(!m.has_decomposed())
    {
        TNTN_LOG_ERROR("mesh is not in decomposed format, please decompose first");
        return false;
    }

    File out_file;
    if(!out_file.open(filename, File::OM_RWCF))
    {
        return false;
    }

    const bool mesh_write_ok = write_mesh_as_off(out_file, m);
    const bool close_ok = out_file.close();
    return mesh_write_ok && close_ok;
}

bool write_mesh_as_off(FileLike& out_file, const Mesh& m)
{
    if(!m.has_decomposed())
    {
        TNTN_LOG_ERROR("mesh is not in decomposed format, please decompose first");
        return false;
    }

    if(!out_file.is_good())
    {
        TNTN_LOG_ERROR("output file is not in a good state");
        return false;
    }

    auto faces_range = m.faces();
    auto vertices_range = m.vertices();

    fmt::memory_buffer line_buffer;
    line_buffer.reserve(128);

    FileLike::position_type write_offset = 0;
    out_file.write(write_offset, "OFF\n", 4);
    write_offset += 4;

    const size_t num_vertices = vertices_range.distance();
    const size_t num_faces = faces_range.distance();
    const size_t num_edges = calculate_num_edges(faces_range);

    line_buffer.resize(0);
    fmt::format_to(line_buffer, "{} {} {}\n", num_vertices, num_faces, num_edges);
    out_file.write(write_offset, line_buffer.data(), line_buffer.size());
    write_offset += line_buffer.size();

    for(const Vertex* v = vertices_range.begin; v != vertices_range.end; v++)
    {
        line_buffer.resize(0);
        fmt::format_to(line_buffer, "{:.18f} {:.18f} {:.18f}\n", v->x, v->y, v->z);
        out_file.write(write_offset, line_buffer.data(), line_buffer.size());
        write_offset += line_buffer.size();
    }

    for(const Face* f = faces_range.begin; f != faces_range.end; f++)
    {
        line_buffer.resize(0);
        fmt::format_to(line_buffer, "3 {} {} {}\n", (*f)[0], (*f)[1], (*f)[2]);
        out_file.write(write_offset, line_buffer.data(), line_buffer.size());
        write_offset += line_buffer.size();
    }

    return true;
}

} //namespace tntn
