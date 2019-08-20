#include "tntn/raster_tools.h"
#include <vector>
#include <algorithm>

#include <algorithm>
#include <cmath>

namespace tntn {
/** downsample image by an integer factor
     takes the mean of all pixels in a window_size by window_size sub window
     @param src - source raster image
     @param window_size - factor to downsample by (will truncate output size to nearest integer)
     @return downsampled raster image
    */
RasterDouble raster_tools::integer_downsample_mean(const RasterDouble& src, int win)
{
    int w = src.get_width();
    int h = src.get_height();

    int ws = w / win;
    int hs = h / win;

    double ndv = src.get_no_data_value();

    RasterDouble dst(ws, hs);

    dst.copy_parameters(src);
    dst.set_cell_size(src.get_cell_size() * win);
    dst.set_all(ndv);

    for(int rs = 0; rs < hs; rs++)
    {
        for(int cs = 0; cs < ws; cs++)
        {
            int count = 0;
            double sum = 0;

            for(int i = 0; i < win; i++)
            {
                for(int j = 0; j < win; j++)
                {
                    double sv = src.value(rs * win + i, cs * win + j);

                    if(sv != ndv)
                    {
                        sum += src.value(rs * win + i, cs * win + j);
                        count++;
                    }
                }
            }

            if(count == 0)
                dst.value(rs, cs) = ndv;
            else if(sum > 0)
                dst.value(rs, cs) = sum / (double)(count);
        }
    }

    return dst;
}

RasterDouble raster_tools::convolution_filter(const RasterDouble& src,
                                              std::vector<double> kernel,
                                              int size)
{
    RasterDouble dst;

    if(size - 2 * (size / 2) != 1)
    {
        TNTN_LOG_ERROR("kernel size must be odd");
        return dst;
    }

    int w = src.get_width();
    int h = src.get_height();

    dst.allocate(w, h);
    dst.copy_parameters(src);
    dst.set_all(dst.get_no_data_value());

    double src_ndv = src.get_no_data_value();
    double* pS = src.get_ptr(0);
    int s2 = size / 2;

    // not optimsed yet!
    for(int r = s2; r < h - s2; r++)
    {
        double* pD = dst.get_ptr(r);

        for(int c = s2; c < w - s2; c++)
        {
            pD[c] = 0;

            for(int i = 0; i < size; i++) // filter rows
            {
                for(int j = 0; j < size; j++) // filter colums
                {
                    if(pS[(r + i - s2) * w + (c + j - s2)] != src_ndv)
                    {
                        pD[c] += pS[(r + i - s2) * w + (c + j - s2)] * kernel[i * size + j];
                    }
                }
            }
        }
    }

    return dst;
}

RasterDouble raster_tools::max_filter(const RasterDouble& src,
                                      int size,
                                      double pos,
                                      double factor)
{
    RasterDouble dst;

    if(size - 2 * (size / 2) != 1)
    {
        TNTN_LOG_ERROR("kernel size must be odd");
        return dst;
    }

    int w = src.get_width();
    int h = src.get_height();

    dst.allocate(w, h);

    double* pS = src.get_ptr(0);

    int s2 = size / 2;

    dst.set_all(dst.get_no_data_value());

    for(int r = s2; r < h - s2; r++)
    {
        double* pD = dst.get_ptr(r);

        for(int c = s2; c < w - s2; c++)
        {
            double max = -std::numeric_limits<double>::max();
            for(int i = 0; i < size; i++) // filter rows
            {
                for(int j = 0; j < size; j++) // filter colums
                {
                    if(pS[(r + i - s2) * w + (c + j - s2)] > max)
                    {
                        max = pS[(r + i - s2) * w + (c + j - s2)];
                    }
                }
            }

            if(pS[r * w + c] >= max * factor)
            {
                pD[c] = pos;
            }
        }
    }

    return dst;
}

void raster_tools::flip_data_x(RasterDouble& raster)
{
    const int height = raster.get_height();
    const int width = raster.get_width();

    for(int row = 0; row < height; row++)
    {
        double* begin = raster.get_ptr(row);
        double* end = begin + width;
        std::reverse(begin, end);
    }
}

void raster_tools::flip_data_y(RasterDouble& raster)
{
    int row = 0;
    const int height = raster.get_height();
    const int width = raster.get_width();
    const int half_height = height / 2;

    while(row < half_height)
    {
        double* a_begin = raster.get_ptr(row);
        double* a_end = a_begin + width;
        double* b_begin = raster.get_ptr_ll(row);

        std::swap_ranges(a_begin, a_end, b_begin);
        row++;
    }
}

void raster_tools::find_minmax(const RasterDouble& raster, double& min_val, double& max_val)
{
    if(raster.empty())
    {
        return;
    }

    double min = raster.value(0, 0);
    double max = raster.value(0, 0);

    const auto pixel_start = raster.get_ptr();
    const auto pixel_end = pixel_start + raster.get_height() * raster.get_width();

    for(auto pixel = pixel_start; pixel != pixel_end; ++pixel)
    {
        if(raster.is_no_data(*pixel))
        {
            continue;
        }

        min = std::min(*pixel, min);
        max = std::max(*pixel, max);
    }

    min_val = min;
    max_val = max;
}

/**
	 Treat raster as a DEM and return 3D bounding box

     @return 3d bounding box
	*/
BBox3D raster_tools::get_bounding_box3d(const RasterDouble& raster)
{
    double min_height = 0.0;
    double max_height = 0.0;

    auto bbox2d = raster.get_bounding_box();

    raster_tools::find_minmax(raster, min_height, max_height);

    BBox3D bbox3d;
    bbox3d.min = {bbox2d.min.x, bbox2d.min.y, min_height};
    bbox3d.max = {bbox2d.max.x, bbox2d.max.y, max_height};

    return bbox3d;
}

static inline bool is_no_data(const double z, const double no_data_value)
{
    return z == no_data_value || std::isnan(z);
}

template<size_t Size>
static inline double average_nan_arr(const std::array<double, Size>& to_average)
{
    double sum = 0;
    int avg_count = 0;
    for(int i = 0; i < Size; i++)
    {
        const double v = to_average[i];
        if(!std::isnan(v))
        {
            sum += v;
            avg_count++;
        }
    }
    if(avg_count == 0)
    {
        return NAN;
    }
    return sum / avg_count;
}

static inline double safe_get_pixel(
    const RasterDouble& src, const int64_t w, const int64_t h, const int64_t r, const int64_t c)
{
    return r >= 0 && r < h && c >= 0 && c < w ? src.value(r, c) : NAN;
}

static double subsample_raster_3x3(const RasterDouble& src,
                                   const double no_data_value,
                                   const int64_t w,
                                   const int64_t h,
                                   const int64_t r,
                                   const int64_t c)
{

    double center_pixel;
    std::array<double, 4> cross_pixels; //weight = 0.35
    std::array<double, 4> diag_pixels; //weight = 0.15

    diag_pixels[0] = safe_get_pixel(src, w, h, r - 1, c - 1); //top-left
    cross_pixels[0] = safe_get_pixel(src, w, h, r - 1, c); //top
    diag_pixels[1] = safe_get_pixel(src, w, h, r - 1, c + 1); //top-right
    cross_pixels[1] = safe_get_pixel(src, w, h, r, c - 1); //center-left
    center_pixel = safe_get_pixel(src, w, h, r, c); //center-center
    cross_pixels[2] = safe_get_pixel(src, w, h, r, c + 1); //center-right
    diag_pixels[2] = safe_get_pixel(src, w, h, r + 1, c - 1); //bottom-left
    cross_pixels[3] = safe_get_pixel(src, w, h, r + 1, c); //bottom-center
    diag_pixels[3] = safe_get_pixel(src, w, h, r + 1, c + 1); //bottom-right

    //replace no_data_value with NAN
    center_pixel = center_pixel == no_data_value ? NAN : center_pixel;
    for(int i = 0; i < 4; i++)
    {
        if(diag_pixels[i] == no_data_value)
        {
            diag_pixels[i] = NAN;
        }
        if(cross_pixels[i] == no_data_value)
        {
            cross_pixels[i] = NAN;
        }
    }

    const double cross_avg = average_nan_arr(cross_pixels);
    const double diag_avg = average_nan_arr(diag_pixels);

    const std::array<double, 3 + 2 + 1> weighted = {{
        center_pixel,
        center_pixel,
        center_pixel,
        cross_avg,
        cross_avg,
        diag_avg,
    }};
    const double weighted_avg = average_nan_arr(weighted);
    return weighted_avg;
}

static constexpr int MAX_AVERAGING_SAMPLES = 64;

static inline double average(const std::array<double, MAX_AVERAGING_SAMPLES>& to_average,
                             const int avg_count)
{
    if(avg_count == 0)
    {
        return NAN;
    }
    double sum = 0;
    for(int i = 0; i < MAX_AVERAGING_SAMPLES; i++)
    {
        sum += to_average[i];
    }
    const double avg = sum / avg_count;
    return avg;
}

double raster_tools::sample_nearest_valid_avg(const RasterDouble& src,
                                              const unsigned int _row,
                                              const unsigned int _column,
                                              int min_averaging_samples)
{
    min_averaging_samples = std::min(min_averaging_samples, MAX_AVERAGING_SAMPLES);

    const int64_t row = _row;
    const int64_t column = _column;
    const int64_t w = src.get_width();
    const int64_t h = src.get_height();
    const int64_t max_radius = static_cast<int64_t>(std::sqrt(w * w + h * h));
    const auto no_data_value = src.get_no_data_value();

    double z = 0;
    if(row < h && column < w)
    {
        //valid coordinate
        z = src.value(row, column);
    }
    if(!is_no_data(z, no_data_value))
    {
        return z;
    }

    std::array<double, MAX_AVERAGING_SAMPLES> to_average;
    std::fill(to_average.begin(), to_average.end(), 0.0);
    int avg_count = 0;

    auto putpixel = [row, column, w, h, no_data_value, &src, &to_average, &avg_count](int x,
                                                                                      int y) {
        const int64_t dest_r = row + y;
        const int64_t dest_c = column + x;
        double z = subsample_raster_3x3(src, no_data_value, w, h, dest_r, dest_c);
        if(!is_no_data(z, no_data_value) && avg_count < MAX_AVERAGING_SAMPLES)
        {
            to_average[avg_count] = z;
            avg_count++;
        }
    };

    for(int64_t radius = 2; radius <= max_radius && avg_count < min_averaging_samples; radius++)
    {

        int64_t x = radius - 1;
        int64_t y = 0;
        int64_t dx = 1;
        int64_t dy = 1;
        int64_t err = dx - (radius / 2);

        while(x >= y)
        {
            putpixel(x, y);
            putpixel(y, x);
            putpixel(-y, x);
            putpixel(-x, y);
            putpixel(-x, -y);
            putpixel(-y, -x);
            putpixel(y, -x);
            putpixel(x, -y);

            if(err <= 0)
            {
                y++;
                err += dy;
                dy += 2;
            }
            else
            //if (err > 0)
            {
                x--;
                dx += 2;
                err += dx - (radius / 2);
            }
        }
    }

    //short path for only one sample
    if(avg_count == 1)
    {
        return to_average[0];
    }

    return average(to_average, avg_count);
}

} // namespace tntn
