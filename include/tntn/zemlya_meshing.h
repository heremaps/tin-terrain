#pragma once

#include "tntn/SurfacePoints.h"
#include "tntn/Mesh.h"

#include <memory>

namespace tntn {

std::unique_ptr<Mesh> generate_tin_zemlya(std::unique_ptr<RasterDouble> raster, double max_error);
std::unique_ptr<Mesh> generate_tin_zemlya(std::unique_ptr<SurfacePoints> surface_points,
                                          double max_error);
std::unique_ptr<Mesh> generate_tin_zemlya(const SurfacePoints& surface_points, double max_error);

} //namespace tntn
