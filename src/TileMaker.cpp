#include "tntn/TileMaker.h"
#include "tntn/MercatorProjection.h"

#include "tntn/geometrix.h"
#include "tntn/Mesh.h"
#include "tntn/MeshIO.h"
#include "tntn/logging.h"

#include "glm/glm.hpp"
#include "glm/gtx/normal.hpp"

#include <vector>
#include <string>
#include <array>
#include <cstdlib>

namespace tntn {

static bool triangle_could_be_in_tile(const Triangle& t, const BBox2D& tile_bounds)
{
    BBox2D triangle_bounds(t);
    return triangle_bounds.intersects(tile_bounds);
}

// Load an OBJ file
bool TileMaker::loadObj(const char* filename)
{
    auto mesh = load_mesh_from_obj(filename);
    if(!mesh || mesh->empty())
    {
        TNTN_LOG_ERROR("resulting mesh is empty");
        return false;
    }

    loadMesh(std::move(mesh));
    return true;
}

void TileMaker::loadMesh(std::unique_ptr<Mesh> mesh)
{
    m_mesh = std::move(mesh);
}

bool TileMaker::normals_enabled() const
{
    return m_normals_enabled;
}

// Dump a tile into an terrain tile in format determined by a MeshWriter
bool TileMaker::dumpTile(int tx, int ty, int zoom, const char* filename, MeshWriter& mesh_writer)
{
    MercatorProjection projection;

    // Tile bounds with buffer
    const BoundingBox tileBounds = projection.TileBounds(tx, ty, zoom);
    const double buffer = tileBounds.width() / 4.0;
    const BBox2D tileBoundsWithBuffer = {
        glm::dvec2(tileBounds.min.x - buffer, tileBounds.min.y - buffer),
        glm::dvec2(tileBounds.max.x + buffer, tileBounds.max.y + buffer)};

    // Find all triangles within the tile bounds
    std::vector<Triangle> trianglesInTile;
    m_mesh->generate_triangles();
    const auto triangles_range = m_mesh->triangles();

    for(const Triangle* tp = triangles_range.begin; tp != triangles_range.end; tp++)
    {
        if(triangle_could_be_in_tile(*tp, tileBoundsWithBuffer))
        {
            trianglesInTile.push_back(*tp);
        }
    }

    TNTN_LOG_DEBUG("before clipping: {} triangles in tile", trianglesInTile.size());

    // Convert to 0-1 scale (upper right quadrant)
    const glm::dvec2 tileOrigin = {tileBounds.min.x, tileBounds.min.y};

    BBox3D tileSpaceBbox;

    tileSpaceBbox.min.x = tileBounds.min.x;
    tileSpaceBbox.min.y = tileBounds.min.y;
    tileSpaceBbox.max.x = tileBounds.max.x;
    tileSpaceBbox.max.y = tileBounds.max.y;

    for(auto& triangle : trianglesInTile)
    {
        for(int i = 0; i < 3; i++)
        {
            if(triangle[i].z < tileSpaceBbox.min.z) tileSpaceBbox.min.z = triangle[i].z;
            if(triangle[i].z > tileSpaceBbox.max.z) tileSpaceBbox.max.z = triangle[i].z;
        }
    }

    const double tileInverseScaleX = 1.0 / tileBounds.width();
    const double tileInverseScaleY = 1.0 / tileBounds.height();
    //TODO fix potential division by zero
    const double tileInverseScaleZ = 1.0 / (tileSpaceBbox.max.z - tileSpaceBbox.min.z);

    for(auto& triangle : trianglesInTile)
    {
        for(int i = 0; i < 3; i++)
        {
            triangle[i].x = (triangle[i].x - tileOrigin.x) * tileInverseScaleX;
            triangle[i].y = (triangle[i].y - tileOrigin.y) * tileInverseScaleY;
            triangle[i].z = (triangle[i].z - tileSpaceBbox.min.z) * tileInverseScaleZ;
        }
    }

    // Clip the triangles to upper right quadrant
    clip_25D_triangles_to_01_quadrant(trianglesInTile);

    TNTN_LOG_INFO("after clipping: {} triangles in tile", trianglesInTile.size());
    TNTN_LOG_DEBUG("tile mesh bbox {}: ", tileSpaceBbox.to_string());

    if(trianglesInTile.size() == 0)
    {
        //ignore empty meshes
        return true;
    }

    Mesh tileMesh;
    tileMesh.from_triangles(std::move(trianglesInTile));
    tileMesh.generate_decomposed();

    if(normals_enabled())
    {
        tileMesh.compute_vertex_normals();
    }

    return mesh_writer.write_mesh_to_file(filename, tileMesh, tileSpaceBbox);
}

} //namespace tntn
