#pragma once

#include <memory>

#include "tntn/Raster.h"
#include "tntn/Mesh.h"

namespace tntn {

std::unique_ptr<Mesh> generate_tin_curvature(const RasterDouble& raster, double threshold);
std::unique_ptr<Mesh> generate_tin_dense_quadwalk(const RasterDouble& raster, int step = 1);

} // namespace tntn
