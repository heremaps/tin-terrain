#pragma once

#include "tntn/MercatorProjection.h"
#include "tntn/SurfacePoints.h"
#include "tntn/MeshWriter.h"

#include <vector>
#include <memory>

namespace tntn {

struct Partition
{
    BoundingBox bbox;
    glm::ivec2 tmin;
    glm::ivec2 tmax;
};

std::vector<Partition> create_partitions_for_zoom_level(const RasterDouble& dem, int zoom);

bool create_tiles_for_zoom_level(const RasterDouble& dem,
                                 const std::vector<Partition>& partitions,
                                 int zoom,
                                 bool include_normals,
                                 const std::string& output_basedir,
                                 const double method_parameter,
                                 const std::string& meshing_method,
                                 MeshWriter& mesh_writer);

} //namespace tntn
