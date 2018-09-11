#pragma once

#include <memory>
#include <string>
#include "tntn/Raster.h"

namespace tntn {

typedef std::unique_ptr<RasterDouble> UniqueRasterPointer;

struct RasterOverview
{
    int zoom_level;
    double resolution;
    UniqueRasterPointer raster;
};

class RasterOverviews
{
  private:
    UniqueRasterPointer m_base_raster;

    int m_min_zoom;
    int m_max_zoom;

    int m_estimated_min_zoom;
    int m_estimated_max_zoom;
    int m_current_zoom;

    int guess_max_zoom_level(double resolution);
    int guess_min_zoom_level(int max_zoom_level);
    void compute_zoom_levels();

  public:
    RasterOverviews(UniqueRasterPointer base_raster, int min_zoom, int max_zoom);
    ~RasterOverviews() = default;

    bool next(RasterOverview& overview);
};

} // namespace tntn
