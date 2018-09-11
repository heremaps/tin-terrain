#include "tntn/TerraUtils.h"
#include "tntn/logging.h"
#include "tntn/raster_tools.h"

namespace tntn {
namespace terra {

void TerraBaseMesh::repair_point(int px, int py)
{
    double& p = m_raster->value(py, px);
    const double z = raster_tools::sample_nearest_valid_avg(*m_raster, py, px);
    const double no_data_value = m_raster->get_no_data_value();
    if(is_no_data(z, no_data_value))
    {
        p = 0.0;
    }
    else
    {
        TNTN_LOG_DEBUG("fill missing point: ({}, {}, {})", px, py, z);
        p = z;
    }
}

void TerraBaseMesh::load_raster(std::unique_ptr<RasterDouble> raster)
{
    m_raster = std::move(raster);
}

} // namespace terra
} // namespace tntn
