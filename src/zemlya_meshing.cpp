#include "tntn/zemlya_meshing.h"
#include "tntn/MeshIO.h"
#include "tntn/ZemlyaMesh.h"

namespace tntn {

std::unique_ptr<Mesh> generate_tin_zemlya(std::unique_ptr<RasterDouble> raster, double max_error)
{
    zemlya::ZemlyaMesh g;
    g.load_raster(std::move(raster));
    g.greedy_insert(max_error);
    return g.convert_to_mesh();
}

std::unique_ptr<Mesh> generate_tin_zemlya(std::unique_ptr<SurfacePoints> surface_points,
                                          double max_error)
{
    auto raster = surface_points->to_raster();
    surface_points.reset();

    return generate_tin_zemlya(std::move(raster), max_error);
}

std::unique_ptr<Mesh> generate_tin_zemlya(const SurfacePoints& surface_points, double max_error)
{
    auto raster = surface_points.to_raster();
    return generate_tin_zemlya(std::move(raster), max_error);
}

} //namespace tntn
