#pragma once

#include "tntn/Mesh.h"
#include "tntn/Raster.h"
#include "tntn/SuperTriangle.h"

namespace tntn {

class Mesh2Raster
{

  public:
    RasterDouble rasterise(Mesh& mesh,
                           int out_width,
                           int out_height,
                           int original_width = -1,
                           int original_height = -1);
    void rasterise_triangle(RasterDouble& raster, SuperTriangle& tri);

    static double findRMSError(const RasterDouble& r1,
                               const RasterDouble& r2,
                               RasterDouble& errorMap,
                               double& maxError);
    static RasterDouble measureError(const RasterDouble& r1,
                                     const RasterDouble& r2,
                                     double& mean,
                                     double& std,
                                     double& max_abs_error);

    BBox2D getBoundingBox() { return m_bb; }

  private:
    static BBox2D findBoundingBox(Mesh& mesh);

    Triangle scaleTriangle(const Triangle& t, float w, float h);
    Triangle scaleTriangle(const Triangle& t, RasterDouble& raster);

    Vertex scaleVertex(const Vertex& v, float w, float h);
    Vertex scaleVertex(const Vertex& v, RasterDouble& raster);

  private:
    BBox2D m_bb;
};

} // namespace tntn
