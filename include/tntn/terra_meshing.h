#pragma once

#include "tntn/SurfacePoints.h"
#include "tntn/Mesh.h"
#include "tntn/Raster.h"

#include <memory>

namespace tntn {

std::unique_ptr<Mesh> generate_tin_terra(std::unique_ptr<RasterDouble> raster, double max_error, int max_iterations=0);

std::unique_ptr<Mesh> generate_tin_terra(std::unique_ptr<SurfacePoints> surface_points,
                                         double max_error, int max_iterations=0);
std::unique_ptr<Mesh> generate_tin_terra(const SurfacePoints& surface_points, double max_error, int max_iterations=0);

} //namespace tntn
