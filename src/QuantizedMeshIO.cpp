#include "tntn/QuantizedMeshIO.h"

#include "tntn/OFFReader.h"
#include "tntn/logging.h"
#include "tntn/tntn_assert.h"
#include "tntn/BinaryIO.h"

#include <iostream>
#include <fstream>
#include <set>

#include <unordered_map>
#include "glm/glm.hpp"

#include <ogr_spatialref.h>
#include <gdal_priv.h>

//FIXME: remove collappsed vertices/triangles after quantization of mesh

namespace tntn {

typedef glm::dvec3 QMVertex;
typedef std::unordered_map<Vertex, unsigned int> VertexOrdering;

struct QuantizedMeshLog
{
    File::position_type QuantizedMeshHeader_start = 0;
    File::position_type VertexData_vertexCount_start = 0;
    uint32_t VertexData_vertexCount = 0;
    File::position_type VertexData_u_start = 0;
    File::position_type VertexData_v_start = 0;
    File::position_type VertexData_height_start = 0;
    int IndexData_bits = 0;
    File::position_type IndexData_triangleCount_start = 0;
    uint32_t IndexData_triangleCount = 0;
    File::position_type IndexData_indices_start = 0;

    std::string to_string() const
    {
        std::string out;
        out = "{\n";
        out += "\t\"QuantizedMeshHeader_start\": " + std::to_string(QuantizedMeshHeader_start) +
            "\n";
        out += "\t\"VertexData_vertexCount_start\": " +
            std::to_string(VertexData_vertexCount_start) + "\n";
        out += "\t\"VertexData_vertexCount\": " + std::to_string(VertexData_vertexCount) + "\n";
        out += "\t\"VertexData_u_start\": " + std::to_string(VertexData_u_start) + "\n";
        out += "\t\"VertexData_v_start\": " + std::to_string(VertexData_v_start) + "\n";
        out += "\t\"VertexData_height_start\": " + std::to_string(VertexData_height_start) + "\n";
        out += "\t\"IndexData_bits\": " + std::to_string(IndexData_bits) + "\n";
        out += "\t\"IndexData_triangleCount_start\": " +
            std::to_string(IndexData_triangleCount_start) + "\n";
        out += "\t\"IndexData_triangleCount\": " + std::to_string(IndexData_triangleCount) + "\n";
        out += "\t\"IndexData_indices_start\": " + std::to_string(IndexData_indices_start) + "\n";
        out += "}\n";
        return out;
    }
};

struct QuantizedMeshHeader
{
    // The center of the tile in Earth-centered Fixed coordinates.
    // double CenterX;
    // double CenterY;
    // double CenterZ;

    QMVertex center;

    // The minimum and maximum heights in the area covered by this tile.
    // The minimum may be lower and the maximum may be higher than
    // the height of any vertex in this tile in the case that the min/max vertex
    // was removed during mesh simplification, but these are the appropriate
    // values to use for analysis or visualization.
    float MinimumHeight;
    float MaximumHeight;

    // The tile’s bounding sphere.  The X,Y,Z coordinates are again expressed
    // in Earth-centered Fixed coordinates, and the radius is in meters.
    QMVertex bounding_sphere_center;
    // double BoundingSphereCenterX;
    // double BoundingSphereCenterY;
    // double BoundingSphereCenterZ;
    double BoundingSphereRadius;

    // The horizon occlusion point, expressed in the ellipsoid-scaled Earth-centered Fixed frame.
    // If this point is below the horizon, the entire tile is below the horizon.
    // See http://cesiumjs.org/2013/04/25/Horizon-culling/ for more information.
    QMVertex horizon_occlusion;
    // double HorizonOcclusionPointX;
    // double HorizonOcclusionPointY;
    // double HorizonOcclusionPointZ;
};

namespace detail {

uint16_t zig_zag_encode(int16_t i)
{
    return ((i >> 15) ^ (i << 1));
}

int16_t zig_zag_decode(uint16_t i)
{
    return static_cast<int16_t>(i >> 1) ^ -static_cast<int16_t>(i & 1);
}

} //namespace detail

using namespace detail;

constexpr int QUANTIZED_COORDINATE_SIZE = 32767;

static unsigned int scale_coordinate(const double v)
{
    const int scaled_v = static_cast<int>(v * QUANTIZED_COORDINATE_SIZE);
    return scaled_v;
}

static double unscale_coordinate(const int v)
{
    const double unscaled_v = static_cast<double>(v) / QUANTIZED_COORDINATE_SIZE;
    return unscaled_v;
}

static unsigned int quantize_coordinate(const double v, const double min, const double max)
{
    TNTN_ASSERT(v >= min && v <= max);
    const double delta = max - min;
    TNTN_ASSERT(delta > 0);
    const double offset_to_min = (v - min);
    TNTN_ASSERT(offset_to_min >= 0 && offset_to_min <= delta);
    return scale_coordinate(offset_to_min / delta);
}

static double dequantize_coordinate(const int v, const double min, const double max)
{
    const double unscaled_v = unscale_coordinate(v);

    const double delta = max - min;
    TNTN_ASSERT(delta > 0);

    const double offset_to_min = unscaled_v * delta;
    const double deq_coord = min + offset_to_min;
    return deq_coord;
}

static void add_alignment(BinaryIO& bio,
                          BinaryIOErrorTracker& e,
                          const int alignment,
                          const uint8_t value = 0xCA)
{
    const auto sp = bio.write_pos();
    const int pad_size = sp % alignment == 0 ? 0 : alignment - (sp % alignment);

    for(int i = 0; i < pad_size; i++)
    {
        bio.write_byte(value, e);
    }
}

template<typename T>
struct get_alignment_helper
{
};

template<>
struct get_alignment_helper<uint32_t>
{
    enum
    {
        value = 4
    };
};
template<>
struct get_alignment_helper<uint16_t>
{
    enum
    {
        value = 2
    };
};

template<typename IndexType>
static void write_faces(BinaryIO& bio,
                        BinaryIOErrorTracker& e,
                        QuantizedMeshLog& log,
                        const SimpleRange<const Triangle*>& tris,
                        const VertexOrdering& order)
{
    typedef IndexType index_t;

    log.IndexData_bits = sizeof(index_t) * 8;

    const uint32_t ntriangles = tris.distance();

    std::vector<index_t> indices;

    indices.reserve(static_cast<size_t>(ntriangles) * 3);

    // High-water mark encode triangle indices
    index_t watermark = 0;
    for(auto it = tris.begin; it != tris.end; ++it)
    {
        for(int n = 0; n < 3; ++n)
        {
            const Vertex& v = (*it)[n];
            const auto order_v_it = order.find(v);
            TNTN_ASSERT(order_v_it != order.end());
            TNTN_ASSERT(order_v_it->second <= std::numeric_limits<index_t>::max());

            const index_t index = order_v_it->second;
            TNTN_ASSERT((int64_t)watermark - (int64_t)index >= 0);
            const index_t delta = watermark - index;

            indices.push_back(delta);
            if(index == watermark)
            {
                watermark++;
            }
        }
    }

    const int alignment = get_alignment_helper<index_t>::value;
    add_alignment(bio, e, alignment);

    log.IndexData_triangleCount_start = bio.write_pos();
    log.IndexData_triangleCount = ntriangles;
    bio.write_uint32(ntriangles, e);
    if(ntriangles > 0)
    {
        log.IndexData_indices_start = bio.write_pos();
        bio.write_array(indices, e);
    }
}

template<typename IndexType>
static void write_indices(BinaryIO& bio,
                          BinaryIOErrorTracker& e,
                          const std::vector<uint32_t>& indices)
{
    const uint32_t nindices = indices.size();

    bio.write_uint32(nindices, e);

    //TODO specialize write for 16/32bit so 32bit write is fast
    for(uint32_t i = 0; i < nindices; i++)
    {
        const IndexType index = indices[i];
        bio.write(index, e);
    }
}

bool write_mesh_as_qm(const char* filename, const Mesh& m, bool compress)
{
    BBox3D bbox;
    m.get_bbox(bbox);
    return write_mesh_as_qm(filename, m, bbox, false, compress);
}

bool write_mesh_as_qm(const char* filename,
                      const Mesh& m,
                      const BBox3D& bbox,
                      bool mesh_is_rescaled, bool comprress)
{
    if (comprress) {
        auto f = std::make_shared<GZipWriteFile>(filename);
        return write_mesh_as_qm(f, m, bbox, mesh_is_rescaled);
    }

    auto f = std::make_shared<File>();
    f->open(filename, File::OM_RWCF);
    return write_mesh_as_qm(f, m, bbox, mesh_is_rescaled);
}

bool write_mesh_as_qm(const std::shared_ptr<FileLike>& f, const Mesh& m)
{
    BBox3D bbox;
    m.get_bbox(bbox);
    return write_mesh_as_qm(f, m, bbox, false);
}

static void write_qmheader(BinaryIO& bio,
                           BinaryIOErrorTracker& e,
                           const QuantizedMeshHeader& qmheader)
{
    bio.write_double(qmheader.center.x, e);
    bio.write_double(qmheader.center.y, e);
    bio.write_double(qmheader.center.z, e);

    bio.write_float(qmheader.MinimumHeight, e);
    bio.write_float(qmheader.MaximumHeight, e);

    bio.write_double(qmheader.bounding_sphere_center.x, e);
    bio.write_double(qmheader.bounding_sphere_center.y, e);
    bio.write_double(qmheader.bounding_sphere_center.z, e);
    bio.write_double(qmheader.BoundingSphereRadius, e);

    bio.write_double(qmheader.horizon_occlusion.x, e);
    bio.write_double(qmheader.horizon_occlusion.y, e);
    bio.write_double(qmheader.horizon_occlusion.z, e);
}

bool point_to_ecef(Vertex& p)
{
    OGRSpatialReference mercator;
    OGRSpatialReference ecef;

    mercator.importFromEPSG(3857);
    ecef.importFromEPSG(4978);

    std::unique_ptr<OGRCoordinateTransformation> tr(
        OGRCreateCoordinateTransformation(&mercator, &ecef));

    if(tr == nullptr)
    {
        return false;
    }

    return (bool)tr->Transform(1, &p.x, &p.y, &p.z);
}

bool write_mesh_as_qm(const std::shared_ptr<FileLike>& f,
                      const Mesh& m,
                      const BBox3D& bbox,
                      bool mesh_is_rescaled)
{
    if(!m.empty() && !m.has_triangles())
    {
        TNTN_LOG_ERROR("Mesh has to be triangulated in order to be written as QM");
        return false;
    }

    BinaryIO bio(f, Endianness::LITTLE);
    BinaryIOErrorTracker e;
    QuantizedMeshLog log;

    // Write QM Header
    Vertex c = (bbox.max + bbox.min) / 2.0;

    if(!point_to_ecef(c))
    {
        TNTN_LOG_ERROR("Conversion of tile center to ECEF coordinate system failed");
        return false;
    }

    QuantizedMeshHeader header;
    header.center = c;
    header.bounding_sphere_center = c;
    header.BoundingSphereRadius = glm::distance(bbox.min.xy(), bbox.max.xy());

    header.MinimumHeight = bbox.min.z;
    header.MaximumHeight = bbox.max.z;

    // FIXME: is there a better choice for a horizon occlusion point?
    // Currently it's the center of tile elevated to bbox's max Z
    header.horizon_occlusion = c;
    header.horizon_occlusion.z = bbox.max.z;

    log.QuantizedMeshHeader_start = bio.write_pos();
    write_qmheader(bio, e, header);
    if(e.has_error())
    {
        TNTN_LOG_ERROR("{} in file {}", e.to_string(), f->name());
        return false;
    }
    TNTN_ASSERT(
        bio.write_pos() ==
        sizeof(QuantizedMeshHeader)); //might not be true for some platforms, mostly for debugging

    // Write QM vertex data
    std::vector<uint32_t> northlings;
    std::vector<uint32_t> eastlings;
    std::vector<uint32_t> southlings;
    std::vector<uint32_t> westlings;
    VertexOrdering vertices_order;
    std::vector<uint16_t> us;
    std::vector<uint16_t> vs;
    std::vector<uint16_t> hs;

    const uint32_t nvertices = m.vertices().distance();

    us.reserve(nvertices);
    vs.reserve(nvertices);
    hs.reserve(nvertices);

    int u = 0;
    int v = 0;
    int h = 0;
    int prev_u = 0;
    int prev_v = 0;
    int prev_h = 0;
    int vertex_index = 0;

    auto triangles = m.triangles();

    vertices_order.reserve(triangles.distance() / 2);
    for(auto it = triangles.begin; it != triangles.end; ++it)
    {
        for(int n = 0; n < 3; ++n)
        {
            const Vertex node = (*it)[n];

            const auto vertices_order_it = vertices_order.find(node);
            if(vertices_order_it != vertices_order.end()) continue;

            vertices_order.emplace_hint(vertices_order_it, std::make_pair(node, vertex_index));

            // Rescale coordinates
            if(mesh_is_rescaled)
            {
                u = scale_coordinate(node.x);
                v = scale_coordinate(node.y);
                h = scale_coordinate(node.z);
            }
            else
            {
                u = quantize_coordinate(node.x, bbox.min.x, bbox.max.x);
                v = quantize_coordinate(node.y, bbox.min.y, bbox.max.y);
                h = quantize_coordinate(node.z, bbox.min.z, bbox.max.z);
            }
            TNTN_ASSERT(u >= 0 && u <= QUANTIZED_COORDINATE_SIZE);
            TNTN_ASSERT(v >= 0 && v <= QUANTIZED_COORDINATE_SIZE);
            TNTN_ASSERT(h >= 0 && h <= QUANTIZED_COORDINATE_SIZE);

            if(u == 0)
            {
                westlings.push_back(vertex_index);
            }
            else if(u == QUANTIZED_COORDINATE_SIZE)
            {
                eastlings.push_back(vertex_index);
            }

            if(v == 0)
            {
                northlings.push_back(vertex_index);
            }
            else if(v == QUANTIZED_COORDINATE_SIZE)
            {
                southlings.push_back(vertex_index);
            }

            TNTN_ASSERT(u - prev_u >= -32768 && u - prev_u <= 32767);
            TNTN_ASSERT(v - prev_v >= -32768 && v - prev_v <= 32767);
            TNTN_ASSERT(h - prev_h >= -32768 && h - prev_h <= 32767);

            us.push_back(zig_zag_encode(u - prev_u));
            vs.push_back(zig_zag_encode(v - prev_v));
            hs.push_back(zig_zag_encode(h - prev_h));

            prev_u = u;
            prev_v = v;
            prev_h = h;

            vertex_index++;
        }
    }

    log.VertexData_vertexCount_start = bio.write_pos();
    log.VertexData_vertexCount = nvertices;
    bio.write_uint32(nvertices, e);
    if(e.has_error())
    {
        TNTN_LOG_ERROR("{} in file {}", e.to_string(), f->name());
        return false;
    }

    log.VertexData_u_start = bio.write_pos();
    bio.write_array_uint16(us, e);

    log.VertexData_v_start = bio.write_pos();
    bio.write_array_uint16(vs, e);

    log.VertexData_height_start = bio.write_pos();
    bio.write_array_uint16(hs, e);

    if(e.has_error())
    {
        TNTN_LOG_ERROR("{} in file {}", e.to_string(), f->name());
        return false;
    }

    // Write triangle indices data
    if(nvertices <= 65536)
    {
        write_faces<uint16_t>(bio, e, log, m.triangles(), vertices_order);
        write_indices<uint16_t>(bio, e, westlings);
        write_indices<uint16_t>(bio, e, southlings);
        write_indices<uint16_t>(bio, e, eastlings);
        write_indices<uint16_t>(bio, e, northlings);
    }
    else
    {
        write_faces<uint32_t>(bio, e, log, m.triangles(), vertices_order);
        write_indices<uint32_t>(bio, e, westlings);
        write_indices<uint32_t>(bio, e, southlings);
        write_indices<uint32_t>(bio, e, eastlings);
        write_indices<uint32_t>(bio, e, northlings);
    }

    if(e.has_error())
    {
        TNTN_LOG_ERROR("{} in file {}", e.to_string(), f->name());
        return false;
    }

    TNTN_LOG_INFO("writer log: {}", log.to_string());
    return true;
}

static BBox3D bbox_from_header(const QuantizedMeshHeader& qmheader)
{
    const glm::dvec2 top_left = qmheader.bounding_sphere_center - qmheader.BoundingSphereRadius;
    const glm::dvec2 bottom_right =
        qmheader.bounding_sphere_center + qmheader.BoundingSphereRadius;

    BBox3D out(glm::dvec3(top_left, qmheader.MinimumHeight),
               glm::dvec3(bottom_right, qmheader.MaximumHeight));

    return out;
}

static std::vector<Vertex> decode_qm_vertices(const BBox3D& bbox,
                                              const std::vector<uint16_t>& u_buffer,
                                              const std::vector<uint16_t>& v_buffer,
                                              const std::vector<uint16_t>& height_buffer)
{
    if(!(u_buffer.size() == v_buffer.size() && v_buffer.size() == height_buffer.size()))
    {
        TNTN_ASSERT(false); //should never happen
        TNTN_LOG_ERROR("u,v,height must have the same size");
        return std::vector<Vertex>();
    }
    std::vector<Vertex> vx_buffer;
    vx_buffer.reserve(u_buffer.size());

    int u = 0;
    int v = 0;
    int height = 0;
    for(size_t i = 0; i < u_buffer.size(); i++)
    {
        u += zig_zag_decode(u_buffer[i]);
        v += zig_zag_decode(v_buffer[i]);
        height += zig_zag_decode(height_buffer[i]);

        const double x = dequantize_coordinate(u, bbox.min.x, bbox.max.x);
        const double y = dequantize_coordinate(v, bbox.min.y, bbox.max.y);
        const double z = dequantize_coordinate(height, bbox.min.z, bbox.max.z);

        vx_buffer.emplace_back(x, y, z);
    }

    return vx_buffer;
}

template<typename ElemT>
static void decode_qm_faces_impl(const std::vector<ElemT>& indices,
                                 std::vector<Face>& face_buffer)
{
    int highest = 0;
    for(size_t i = 0; i < indices.size(); i += 3)
    {
        Face f;
        for(size_t k = 0; k < 3; k++)
        {
            const int code = indices[i + k];
            f[k] = highest - code;
            if(code == 0)
            {
                highest++;
            }
        }
        face_buffer.push_back(f);
    }
}

static std::vector<Face> decode_qm_faces(const std::vector<uint16_t>& indices16,
                                         const std::vector<uint32_t>& indices32)
{
    std::vector<Face> face_buffer;
    face_buffer.reserve(indices16.size() / 3 + indices32.size() / 3);
    decode_qm_faces_impl(indices16, face_buffer);
    decode_qm_faces_impl(indices32, face_buffer);
    return face_buffer;
}

static void read_qmheader(BinaryIO& bio, BinaryIOErrorTracker& e, QuantizedMeshHeader& qmheader)
{
    bio.read_double(qmheader.center.x, e);
    bio.read_double(qmheader.center.y, e);
    bio.read_double(qmheader.center.z, e);

    bio.read_float(qmheader.MinimumHeight, e);
    bio.read_float(qmheader.MaximumHeight, e);

    bio.read_double(qmheader.bounding_sphere_center.x, e);
    bio.read_double(qmheader.bounding_sphere_center.y, e);
    bio.read_double(qmheader.bounding_sphere_center.z, e);
    bio.read_double(qmheader.BoundingSphereRadius, e);

    bio.read_double(qmheader.horizon_occlusion.x, e);
    bio.read_double(qmheader.horizon_occlusion.y, e);
    bio.read_double(qmheader.horizon_occlusion.z, e);
}

static bool read_qm_data(const std::shared_ptr<FileLike>& f,
                         QuantizedMeshHeader& qmheader,
                         std::vector<uint16_t>& vertex_data_u,
                         std::vector<uint16_t>& vertex_data_v,
                         std::vector<uint16_t>& vertex_data_height,
                         std::vector<uint16_t>& indices16,
                         std::vector<uint32_t>& indices32)
{
    if(!f->is_good())
    {
        TNTN_LOG_ERROR("input file {} is not open/good", f->name());
        return false;
    }

    BinaryIO bio(f, Endianness::LITTLE);
    BinaryIOErrorTracker e;
    QuantizedMeshLog log;

    log.QuantizedMeshHeader_start = bio.read_pos();
    read_qmheader(bio, e, qmheader);
    if(e.has_error())
    {
        TNTN_LOG_ERROR("{} on file {} during QuantizedMeshHeader", e.to_string(), f->name());
        return false;
    }

    //VertexData
    uint32_t vertex_count = 0;
    log.VertexData_vertexCount_start = bio.read_pos();
    bio.read_uint32(vertex_count, e);
    if(e.has_error())
    {
        TNTN_LOG_ERROR("{} on file {} during VertexData::vertexCount", e.to_string(), f->name());
        return false;
    }
    TNTN_LOG_DEBUG("vertex_count: {}", vertex_count);
    log.VertexData_vertexCount = vertex_count;

    if(vertex_count > 0)
    {
        log.VertexData_u_start = bio.read_pos();
        bio.read_array_uint16(vertex_data_u, vertex_count, e);
        if(e.has_error())
        {
            TNTN_LOG_ERROR("{} on file {} during VertexData::u", e.to_string(), f->name());
            return false;
        }

        log.VertexData_v_start = bio.read_pos();
        bio.read_array_uint16(vertex_data_v, vertex_count, e);
        if(e.has_error())
        {
            TNTN_LOG_ERROR("{} on file {} during VertexData::v", e.to_string(), f->name());
            return false;
        }

        log.VertexData_height_start = bio.read_pos();
        bio.read_array_uint16(vertex_data_height, vertex_count, e);
        if(e.has_error())
        {
            TNTN_LOG_ERROR("{} on file {} during VertexData::height", e.to_string(), f->name());
            return false;
        }
    }

    // padding
    const size_t alignment = vertex_count <= 65536 ? 2 : 4;
    const auto read_pos = bio.read_pos();
    if(read_pos % alignment != 0)
    {
        int pad_size = alignment - (read_pos % alignment);
        bio.read_skip(pad_size, e);
    }

    //Index Data
    uint32_t triangle_count = 0;
    log.IndexData_triangleCount_start = bio.read_pos();
    bio.read_uint32(triangle_count, e);
    if(e.has_error())
    {
        TNTN_LOG_ERROR(
            "{} on file {} during IndexData(16/32)::triangleCount", e.to_string(), f->name());
        return false;
    }
    log.IndexData_triangleCount = triangle_count;

    if(triangle_count > 0)
    {
        if(vertex_count <= 65536)
        {
            log.IndexData_bits = 16;
            log.IndexData_indices_start = bio.read_pos();
            bio.read_array_uint16(indices16, static_cast<size_t>(triangle_count) * 3, e);
            if(e.has_error())
            {
                TNTN_LOG_ERROR(
                    "{} on file {} during IndexData16::indices", e.to_string(), f->name());
                return false;
            }
        }
        else
        {
            log.IndexData_bits = 32;
            log.IndexData_indices_start = bio.read_pos();
            bio.read_array_uint32(indices32, static_cast<size_t>(triangle_count) * 3, e);
            if(e.has_error())
            {
                TNTN_LOG_ERROR(
                    "{} on file {} during IndexData32::indices", e.to_string(), f->name());
                return false;
            }
        }
    }
    //TODO edge indices

    //TODO extensions

    if(e.has_error())
    {
        TNTN_LOG_ERROR("{} on file {}", e.to_string(), f->name());
        return false;
    }

    TNTN_LOG_INFO("reader log: {}", log.to_string());
    return true;
}

std::unique_ptr<Mesh> load_mesh_from_qm(const char* filename)
{
    auto f = std::make_shared<File>();
    if(!f->open(filename, File::OM_R))
    {
        TNTN_LOG_ERROR("unable to open quantized mesh in file {}", filename);
        return std::unique_ptr<Mesh>();
    }
    return load_mesh_from_qm(f);
}

std::unique_ptr<Mesh> load_mesh_from_qm(const std::shared_ptr<FileLike>& f)
{
    QuantizedMeshHeader qmheader;
    std::vector<uint16_t> vertex_data_u;
    std::vector<uint16_t> vertex_data_v;
    std::vector<uint16_t> vertex_data_height;
    std::vector<uint16_t> indices16;
    std::vector<uint32_t> indices32;

    TNTN_LOG_DEBUG("reading QuantizedMesh data...");
    if(!read_qm_data(
           f, qmheader, vertex_data_u, vertex_data_v, vertex_data_height, indices16, indices32))
    {
        TNTN_LOG_ERROR("error reading quantized mesh data from file {}", f->name());
        return std::unique_ptr<Mesh>();
    }
    TNTN_LOG_DEBUG(
        "before decoding: VertexData::u.size={} VertexData::v.size={} VertexData::height.size={} IndicesData16::indices.size={} IndicesData32::indices.size={}",
        vertex_data_u.size(),
        vertex_data_v.size(),
        vertex_data_height.size(),
        indices16.size(),
        indices32.size());

    const BBox3D header_bbox = bbox_from_header(qmheader);
    TNTN_LOG_DEBUG("bbox derived from header: {}", header_bbox.to_string());

    std::vector<Vertex> vertices =
        decode_qm_vertices(header_bbox, vertex_data_u, vertex_data_v, vertex_data_height);

    std::vector<Face> faces = decode_qm_faces(indices16, indices32);

    TNTN_LOG_DEBUG("{} vertices, {} faces after decoding", vertices.size(), faces.size());

    auto mesh = std::make_unique<Mesh>();
    mesh->from_decomposed(std::move(vertices), std::move(faces));

    {
        BBox3D mesh_bbox;
        mesh->get_bbox(mesh_bbox);
        TNTN_LOG_DEBUG("bbox derived from resulting mesh: {}", mesh_bbox.to_string());
    }
    return mesh;
}

} // namespace tntn
