#include "tntn/simple_meshing.h"
#include "tntn/RasterIO.h"
#include "tntn/MeshIO.h"
#include "tntn/SurfacePoints.h"
#include "tntn/Points2Mesh.h"
#include "tntn/tntn_assert.h"
#include "tntn/logging.h"
#include "tntn/raster_tools.h"

#if defined(TNTN_USE_ADDONS) && TNTN_USE_ADDONS
#    include "tntn/Raster2Mesh.h"
#endif

namespace tntn {

std::unique_ptr<Mesh> generate_tin_curvature(const RasterDouble& raster, double threshold)
{
#if defined(TNTN_USE_ADDONS) && TNTN_USE_ADDONS
    TNTN_LOG_INFO("curvature point reduction...");

    Raster2Mesh cur(threshold);
    std::vector<Vertex> verList = cur.generate_vertices(raster);

    TNTN_LOG_INFO("delaunay meshing of {} points", verList.size());
    std::unique_ptr<Mesh> pMesh = generate_delaunay_mesh(std::move(verList));

    TNTN_LOG_INFO("done");

    return pMesh;
#else
    TNTN_LOG_ERROR("curvature point reduction not available");
    return std::make_unique<Mesh>();
#endif
}

static void dense_quadwalk_make_face(const size_t this_vx_index,
                                     const size_t vertices_per_row,
                                     std::vector<Face>& faces)
{
    /*
     +--+ upper-right
     |\A|
     |B\|
     +--+ lower-right
     */
    //remember: ccw order
    //Face A
    Face f;
    f[0] = this_vx_index; //lower-right
    f[1] = this_vx_index - vertices_per_row; //upper-right
    f[2] = this_vx_index - vertices_per_row - 1; //upper-left
    faces.push_back(f);

    //Face B
    f[0] = this_vx_index; //lower-right
    f[1] = this_vx_index - vertices_per_row - 1; //upper-left
    f[2] = this_vx_index - 1; //lower-left
    faces.push_back(f);
}

static void dense_quadwalk_make_quads_for_row(const RasterDouble& raster,
                                              const int r,
                                              const int w,
                                              const int step,
                                              const int vx_r,
                                              const int vertices_per_row,
                                              std::vector<Vertex>& vertices,
                                              std::vector<Face>& faces)
{
    const double* const row_ptr = raster.get_ptr(r);
    const double y = raster.row2y(r);

    //first column, only insert vertex
    {
        double z = row_ptr[0];
        if(raster.is_no_data(z) || std::isnan(z))
        {
            z = raster_tools::sample_nearest_valid_avg(raster, r, 0);
        }
        vertices.push_back({raster.col2x(0), y, z});
    }

    //every following column, insert vertex and 2 faces
    for(int vx_c = 1; vx_c < vertices_per_row; vx_c++)
    {
        const int c = std::min(vx_c * step, w - 1);

        double z = row_ptr[c];
        if(raster.is_no_data(z) || std::isnan(z))
        {
            z = raster_tools::sample_nearest_valid_avg(raster, r, c);
        }
        vertices.push_back({raster.col2x(c), y, z});
        const size_t this_vx_index = vx_r * vertices_per_row + vx_c;
        dense_quadwalk_make_face(this_vx_index, vertices_per_row, faces);
    }
}

std::unique_ptr<Mesh> generate_tin_dense_quadwalk(const RasterDouble& raster, int step)
{
    TNTN_ASSERT(step > 0);
    if(step <= 0)
    {
        TNTN_LOG_ERROR("step width for dense meshing must be at least 1");
        return std::unique_ptr<Mesh>();
    }
    const int h = raster.get_height();
    const int w = raster.get_width();
    if(h < 2 || w < 2)
    {
        TNTN_LOG_ERROR("raster to small, must have at least 2x2 cells");
        return std::make_unique<Mesh>();
    }

    const int vertices_per_column = (h - 1) / step + ((h - 1) % step ? 1 : 0) + 1;
    const int vertices_per_row = (w - 1) / step + ((w - 1) % step ? 1 : 0) + 1;
    TNTN_LOG_DEBUG(
        "generating regular mesh with {}x{} vertices...", vertices_per_row, vertices_per_column);
    TNTN_ASSERT(vertices_per_column >= 2);
    TNTN_ASSERT(vertices_per_row >= 2);

    std::vector<Vertex> vertices;
    vertices.reserve(vertices_per_row * vertices_per_column);
    std::vector<Face> faces;
    faces.reserve((vertices_per_row - 1) * (vertices_per_column - 1) * 2);

    //first row, just generate vertices
    {
        const double* const row_ptr = raster.get_ptr(0);
        const double y = raster.row2y(0);

        for(int vx_c = 0; vx_c < vertices_per_row; vx_c++)
        {
            const int c = std::min(vx_c * step, w - 1);
            double z = row_ptr[c];
            if(raster.is_no_data(z) || std::isnan(z))
            {
                z = raster_tools::sample_nearest_valid_avg(raster, 0, c);
            }
            vertices.push_back({raster.col2x(c), y, z});
        }
    }

    //every following row, generate vertices and faces
    for(int vx_r = 1; vx_r < vertices_per_column; vx_r++)
    {
        const int r = std::min(vx_r * step, h - 1);
        dense_quadwalk_make_quads_for_row(
            raster, r, w, step, vx_r, vertices_per_row, vertices, faces);
    }

    auto mesh = std::make_unique<Mesh>();
    mesh->from_decomposed(std::move(vertices), std::move(faces));
    return mesh;
}

} // namespace tntn
