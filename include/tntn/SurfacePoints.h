#pragma once

#include <vector>
#include <string>
#include "tntn/geometrix.h"
#include "tntn/util.h"
#include "tntn/Raster.h"
#include <map>

namespace tntn {

class SurfacePoints
{
  private:
    //disallow copy and assign
    SurfacePoints(const SurfacePoints& other) = delete;
    SurfacePoints& operator=(const SurfacePoints& other) = delete;

  public:
    SurfacePoints() = default;
    SurfacePoints(SurfacePoints&& other) { swap(other); }
    SurfacePoints& operator=(SurfacePoints&& other)
    {
        swap(other);
        return *this;
    }

    void swap(SurfacePoints& other) noexcept;
    SurfacePoints clone() const;

    void clear();
    size_t size() const;
    bool empty() const { return size() == 0; }

    bool load_from_xyz_file(const std::string& filename);
    bool load_from_gdal(const std::string& filename);
    void load_from_memory(std::vector<Vertex>&& points);
    void load_from_raster(const RasterDouble& raster);

    void reserve(size_t size) { m_points.reserve(size); }
    void push_back(const Vertex& p)
    {
        m_points.push_back(p);
        m_bbox.add(p);
    }
    SimpleRange<const Vertex*> points() const;

    BBox3D bounding_box() const { return m_bbox; }

    std::unique_ptr<RasterDouble> to_raster() const;
    std::unique_ptr<Raster<Vertex>> to_vxraster() const;

  private:
    static double find_non_zero_min_diff(const std::vector<double>& values,
                                         double& min,
                                         double& max);

  private:
    std::vector<Vertex> m_points;
    BBox3D m_bbox;
};

} //namespace tntn
