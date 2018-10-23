#include "tntn/RasterOverviews.h"
#include "tntn/raster_tools.h"
#include "tntn/SurfacePoints.h"
#include "tntn/gdal_init.h"

#include <cmath>
#include <gdal_priv.h>

namespace tntn {

const int MINIMAL_RASTER_SIZE = 128;
const double EARTH_RADIUS = 6378137.0;

RasterOverviews::RasterOverviews(UniqueRasterPointer input_raster, int min_zoom, int max_zoom) :
    m_base_raster(std::move(input_raster)),
    m_min_zoom(min_zoom),
    m_max_zoom(max_zoom),
    m_estimated_min_zoom(0),
    m_estimated_max_zoom(0)
{
    compute_zoom_levels();
    m_current_zoom = m_max_zoom;
}

// Guesses (numerically) maximal zoom level from a raster resolution
int RasterOverviews::guess_max_zoom_level(double resolution)
{
 
    // At zoom level 0, we have 1 tile for the whole earth. Therefore if a raster at zoom level 0
    // has MINIMAL_RASTER_SIZE pixels, a pixel corresponds to radius / MINIMAL_RASTER_SIZE meters.
    // the maximum zoom level is then calculated, by obtaining the maximum resolution (i.e. the size of a pixel in
    // the raster in meters) of the raster and seeing how often it fits by a factor of 2 into a single pixel at zoom
    // level 0 

    // Size of one pixel in meters at z=0
    const double pixel_size_z0 = EARTH_RADIUS * 2.0 * M_PI / MINIMAL_RASTER_SIZE;
    return static_cast<int>(round(log2(pixel_size_z0 / resolution)));
}

// Guesses (numerically) minimal zoom level from a raster resolution and it's size
int RasterOverviews::guess_min_zoom_level(int max_zoom_level)
{
    // This calculates the minimum zoom level, at which the input raster is just one pixel.
    const double num_pixels_at_max_zoom_level = MINIMAL_RASTER_SIZE * (1 << max_zoom_level);

    int raster_width = m_base_raster->get_width();
    int raster_height = m_base_raster->get_height();

    TNTN_LOG_DEBUG("guess_min_zoom_level: raster_width: {}, raster_height: {}",
                       raster_width,
                       raster_height);

    int zoom_x = static_cast<int>(floor(log2(num_pixels_at_max_zoom_level / raster_width)));
    int zoom_y = static_cast<int>(floor(log2(num_pixels_at_max_zoom_level / raster_height)));

    return std::max(0, std::min(zoom_x, zoom_y));
}

void RasterOverviews::compute_zoom_levels()
{
    m_estimated_max_zoom = guess_max_zoom_level(fabs(m_base_raster->get_cell_size()));
    m_estimated_min_zoom = guess_min_zoom_level(m_estimated_max_zoom);

    m_min_zoom = std::max(m_min_zoom, m_estimated_min_zoom);
    m_max_zoom = std::min(std::max(0, m_max_zoom), m_estimated_max_zoom);

    if(m_max_zoom < m_min_zoom)
    {
        std::swap(m_min_zoom, m_max_zoom);
    }
}

bool RasterOverviews::next(RasterOverview& overview)
{
    if(m_current_zoom < m_min_zoom) return false;

    int window_size = (1 << (m_estimated_max_zoom - m_current_zoom));
    auto output_raster = std::make_unique<RasterDouble>();

    if(window_size == 1)
    {
        *output_raster = m_base_raster->clone();
    }
    else
    {
        *output_raster = raster_tools::integer_downsample_mean(*m_base_raster, window_size);
    }

    overview.zoom_level = m_current_zoom--;
    overview.resolution = output_raster->get_cell_size();
    overview.raster = std::move(output_raster);

    TNTN_LOG_DEBUG("Generated next overview at zoom {}, window size {}, min zoom level {}, max zoom level {}",
                   m_current_zoom + 1,
                   window_size,
                   m_min_zoom,
                   m_max_zoom);

    return true;
}

} // namespace tntn
