#include "tntn/SurfacePoints.h"
#include "tntn/logging.h"
#include "tntn/gdal_init.h"

#include <vector>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <iomanip>

#include "gdal.h"
#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()

namespace tntn {

void SurfacePoints::clear()
{
    m_points.clear();
    m_bbox.reset();
}

size_t SurfacePoints::size() const
{
    return m_points.size();
}

bool SurfacePoints::load_from_xyz_file(const std::string& filename)
{
    std::ifstream f(filename);
    if(!f.is_open())
    {
        TNTN_LOG_ERROR("error opening input file {}", filename);
        return false;
    }

    clear();

    double x, y, z;
    while(f >> x >> y >> z)
    {
        // skip nodata values
        if(std::isnan(z) || z < -10000.0 || z > 10000.0)
        {
            continue;
        }
        push_back({x, y, z});
    }

    TNTN_LOG_DEBUG("loaded input file {} with {} points", filename, m_points.size());
    TNTN_LOG_DEBUG(" range of values X: {} - {}", m_bbox.min.x, m_bbox.max.x);
    TNTN_LOG_DEBUG(" range of values Y: {} - {}", m_bbox.min.y, m_bbox.max.y);
    TNTN_LOG_DEBUG(" range of values Z: {} - {}", m_bbox.min.z, m_bbox.max.z);

    return true;
}

void SurfacePoints::load_from_memory(std::vector<Vertex>&& points)
{
    m_points = std::move(points);
    m_bbox.reset();
    for(const auto& vertex : m_points)
    {
        m_bbox.add(vertex);
    }

    TNTN_LOG_DEBUG("loaded input {} points", m_points.size());
    TNTN_LOG_DEBUG(" range of values X: {} - {}", m_bbox.min.x, m_bbox.max.x);
    TNTN_LOG_DEBUG(" range of values Y: {} - {}", m_bbox.min.y, m_bbox.max.y);
    TNTN_LOG_DEBUG(" range of values Z: {} - {}", m_bbox.min.z, m_bbox.max.z);
}

void SurfacePoints::load_from_raster(const RasterDouble& raster)
{
    clear();

    const auto nodataval = raster.get_no_data_value();

    m_points.reserve(raster.get_width() * raster.get_height());
    m_bbox.reset();

    raster.to_vertices([this, nodataval](const double x, const double y, const double z) {
        if(z != nodataval)
        {
            Vertex v(x, y, z);
            m_points.emplace_back(v);
            m_bbox.add(v);
        }
    });
}

void SurfacePoints::swap(SurfacePoints& other) noexcept
{
    std::swap(m_points, other.m_points);
    std::swap(m_bbox, other.m_bbox);
}

SurfacePoints SurfacePoints::clone() const
{
    SurfacePoints ret;
    ret.m_points = m_points;
    ret.m_bbox = m_bbox;
    return ret;
}

SimpleRange<const Vertex*> SurfacePoints::points() const
{
    if(m_points.empty())
    {
        return {nullptr, nullptr};
    }
    else
    {
        return {m_points.data(), m_points.data() + m_points.size()};
    }
}

double SurfacePoints::find_non_zero_min_diff(const std::vector<double>& values,
                                             double& min,
                                             double& max)
{
    min = std::numeric_limits<double>::max();
    max = -std::numeric_limits<double>::max();
    if(values.size() < 1) return 0;

    double min_diff = 0;

    //initialize search
    double last = values[0];
    double local_min = last;
    double local_max = last;

    for(int i = 1; i < values.size(); i++)
    {
        double v = values[i];

        TNTN_LOG_TRACE("v: {:6}", v);

        double d = fabs(last - v);
        if(d > 0)
        {
            if(min_diff == 0)
            {
                min_diff = d;
            }
            else if(d < min_diff)
            {
                min_diff = d;
            }
        }

        if(v < local_min) local_min = v;
        if(v > local_max) local_max = v;

        last = v;
    }

    min = local_min;
    max = local_max;
    return min_diff;
}

static void get_xy_values_from_points(const std::vector<Vertex>& points,
                                      std::vector<double>& x_values,
                                      std::vector<double>& y_values)
{
    x_values.clear();
    y_values.clear();

    //find all x and y coordinates (determine the raster grid)
    std::unordered_set<double> x_values_set;
    std::unordered_set<double> y_values_set;

    x_values_set.reserve(sqrt(points.size()) + 1);
    y_values_set.reserve(sqrt(points.size()) + 1);

    for(const auto& p : points)
    {
        x_values_set.insert(p.x);
        y_values_set.insert(p.y);
    }

    x_values.reserve(x_values_set.size());
    std::copy(x_values_set.begin(), x_values_set.end(), std::back_inserter(x_values));
    std::sort(x_values.begin(), x_values.end());

    y_values.reserve(y_values_set.size());
    std::copy(y_values_set.begin(), y_values_set.end(), std::back_inserter(y_values));
    std::sort(y_values.begin(), y_values.end(), std::greater<double>());
}

// reshapes point cloud back to raster
// big assumption: this point cloud was derived from a 2D regular spaced raster
// please note: currently not performing any checks on this assumption
std::unique_ptr<RasterDouble> SurfacePoints::to_raster() const
{
    auto raster = std::make_unique<RasterDouble>();

    // sort x y coordinates seperately
    std::vector<double> x_values;
    std::vector<double> y_values;
    get_xy_values_from_points(m_points, x_values, y_values);

    // find max difference between adjacent values
    // this gives us a rough idea of x y raster spacing

    double max_x, min_x;
    double min_dx = find_non_zero_min_diff(x_values, min_x, max_x);

    double max_y, min_y;
    double min_dy = find_non_zero_min_diff(y_values, min_y, max_y);

    //recover width and height
    int w = min_dx == 0 ? 1 : 1 + (max_x - min_x) / min_dx;
    int h = min_dy == 0 ? 1 : 1 + (max_y - min_y) / min_dy;

    //bring raster to size and set all NaN
    {
        raster->allocate(w, h);
        raster->set_no_data_value(-std::numeric_limits<float>::max());
        raster->set_all(raster->get_no_data_value());
    }

    // TODO: double check / write unit tests etc
    raster->set_pos_x(min_x);
    raster->set_pos_y(min_y);
    raster->set_cell_size((min_dx + min_dy) / 2.0);

    for(const auto& p : m_points)
    {
        int c = min_dx == 0 ? 0 : ((p.x - min_x) / min_dx);
        int r = min_dy == 0 ? 0 : (h - 1 - (p.y - min_y) / min_dy);

        if(c >= 0 && c < w && r >= 0 && r < h)
        {
            raster->value(r, c) = p.z;
        }
    }

    return raster;
}

std::unique_ptr<Raster<Vertex>> SurfacePoints::to_vxraster() const
{
    auto vxraster = std::make_unique<Raster<Vertex>>();

    std::vector<double> x_values;
    std::vector<double> y_values;
    get_xy_values_from_points(m_points, x_values, y_values);

    //bring raster to size and set all NaN
    {
        vxraster->allocate(x_values.size(), y_values.size());
        const Vertex no_data_vx = {std::numeric_limits<float>::min(),
                                   std::numeric_limits<float>::min(),
                                   std::numeric_limits<float>::min()};
        vxraster->set_no_data_value(no_data_vx);
        vxraster->set_all(no_data_vx);
    }

    // find max difference between adjacent values
    // this gives us a rough idea of x y raster spacing

    double max_x, min_x;
    double min_dx = find_non_zero_min_diff(x_values, min_x, max_x);

    double max_y, min_y;
    double min_dy = find_non_zero_min_diff(y_values, min_y, max_y);

    vxraster->set_pos_x(min_x);
    vxraster->set_pos_y(min_y);
    vxraster->set_cell_size((min_dx + min_dy) / 2.0);

    TNTN_LOG_DEBUG("raster: ({}, {}, {})",
                   vxraster->get_pos_x(),
                   vxraster->get_pos_y(),
                   vxraster->get_cell_size());

    //assign each point to a raster cell
    {
        for(const auto& p : m_points)
        {
            const auto x_it = std::lower_bound(x_values.begin(), x_values.end(), p.x);
            const size_t x_i = std::distance(x_values.begin(), x_it);

            const auto y_it =
                std::lower_bound(y_values.begin(), y_values.end(), p.y, std::greater<double>());
            const size_t y_i = std::distance(y_values.begin(), y_it);

            Vertex& v = vxraster->value(y_i, x_i);
            v = p;
        }
    }

    return vxraster;
}

static bool create_points_from_raster(
    GDALRasterBand* band, double ox, double oy, double sx, double sy, SurfacePoints& points)
{
    int width = band->GetXSize();
    int height = band->GetYSize();

    std::vector<double> pixel_data(width * height);

    if(band->RasterIO(
           GF_Read, 0, 0, width, height, &pixel_data.front(), width, height, GDT_Float64, 0, 0) !=
       CE_None)
    {
        TNTN_LOG_ERROR("Can't access raster band");
        return false;
    }

    double nodata = band->GetNoDataValue();

    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {

            double point_value = pixel_data[y * width + x];

            if(point_value == nodata) continue;

            double px = ox + sx * x;
            double py = oy + sy * y;
            double pz = point_value;

            points.push_back({px, py, pz});
        }
    }

    return true;
}

bool SurfacePoints::load_from_gdal(const std::string& filename)
{
    initialize_gdal_once();

    TNTN_LOG_INFO("Loading input file {} with GDAL...", filename);
    std::shared_ptr<GDALDataset> ds(
        static_cast<GDALDataset*>(GDALOpen(filename.c_str(), GA_ReadOnly)), &GDALClose);
    if(ds == nullptr)
    {
        TNTN_LOG_ERROR("Can't open input raster {}", filename);
        return false;
    }

    double gt[6];
    if(ds->GetGeoTransform(gt) != CE_None)
    {
        TNTN_LOG_ERROR("Input raster is missing geotransformation matrix");
        return false;
    }

    GDALRasterBand* band = ds->GetRasterBand(1);
    if(!band)
    {
        TNTN_LOG_ERROR("Can not read input raster band");
        return false;
    }

    const double x_pos = gt[0];
    const double y_pos = gt[3];
    const double csx = gt[1];
    const double csy = gt[5];

    TNTN_LOG_INFO("Converting band 1 from GDAL raster to points...");
    if(!create_points_from_raster(band, x_pos, y_pos, csx, csy, *this))
    {
        TNTN_LOG_ERROR("error transforming {} to points", filename);
        return false;
    }

    return true;
}
} //namespace tntn
