#pragma once

#include "glm/glm.hpp"

namespace tntn {

#define R_EARTH 6378137.0
#define PI 3.14159265358979323846

struct BoundingBox
{
    glm::dvec2 min;
    glm::dvec2 max;
    double width() const { return glm::abs(max.x - min.x); }
    double height() const { return glm::abs(max.y - min.y); }
    bool containsX(double x) const { return x >= min.x && x <= max.x; }
    bool containsY(double y) const { return y >= min.y && y <= max.y; }
    bool contains(double x, double y) const { return containsX(x) && containsY(y); }
};

class MercatorProjection
{
    int m_TileSize;
    double m_Res;

  public:
    constexpr static double INV_360 =
        0.0027777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777777; //1.0/360.0;
    constexpr static double INV_180 =
        0.0055555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555; //1.0/180.0;
    constexpr static double HALF_CIRCUMFERENCE = 20037508.342789243076571549020; //PI * R_EARTH;

    MercatorProjection(int _tileSize = 256);
    ~MercatorProjection() {}

    int tileSize() { return m_TileSize; }
    double tileSizeInMeters(int zoom) { return 2.0 * HALF_CIRCUMFERENCE / (1 << zoom); }
    glm::dvec2 LonLatToMeters(const glm::dvec2 _lonLat) const;
    glm::dvec2 MetersToLonLat(const glm::dvec2 _meters) const;
    glm::dvec2 PixelsToMeters(const glm::dvec2 _pix, const int _zoom) const;
    glm::dvec2 MetersToPixel(const glm::dvec2 _meters, const int _zoom) const;
    glm::ivec2 PixelsToTileXY(const glm::dvec2 _pix) const;
    glm::ivec2 MetersToTileXY(const glm::dvec2 _meters, const int _zoom) const;
    glm::dvec2 PixelsToRaster(const glm::dvec2 _pix, const int _zoom) const;
    BoundingBox TileBounds(const int _tx, const int _ty, const int _zoom) const;
};

} //namespace tntn
