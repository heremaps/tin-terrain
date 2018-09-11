#pragma once

#include "tntn/Raster.h"
#include <vector>
#include <string>
#include "tntn/geometrix.h"

namespace tntn {

struct raster_tools //just a namespace
{
    static RasterDouble integer_downsample_mean(const RasterDouble& src, int window_size);

    static RasterDouble convolution_filter(const RasterDouble& src,
                                           std::vector<double> kernel,
                                           int size);

    static RasterDouble max_filter(const RasterDouble& src, int size, double pos, double factor);

    static void flip_data_x(RasterDouble& r);

    static void flip_data_y(RasterDouble& r);

    static void find_minmax(const RasterDouble& raster, double& min, double& max);

    static BBox3D get_bounding_box3d(const RasterDouble& raster);

    static double sample_nearest_valid_avg(const RasterDouble& src,
                                           const unsigned int row,
                                           const unsigned int column,
                                           int min_averaging_samples = 1);
};

} // namespace tntn
