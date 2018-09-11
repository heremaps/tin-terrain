#pragma once

#include "tntn/geometrix.h"
#include "tntn/Raster.h"

namespace tntn {
class SuperTriangle
{
  public:
    SuperTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3);
    SuperTriangle(const Triangle& tr);

    bool interpolate(const double x, const double y, double& z);
    void rasterise(RasterDouble& raster);

    BBox2D getBB();
    Triangle getTriangle();

  private:
    void init();
    void findBounds();
    void updateBounds(const Vertex& v);

  private:
    Triangle m_t;
    BBox2D m_bb;
    double m_wdem;
};
} // namespace tntn
