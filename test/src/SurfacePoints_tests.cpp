#include "catch.hpp"

#include "tntn/SurfacePoints.h"

namespace tntn {
namespace unittests {

TEST_CASE("VertexRaster from points", "[tntn]")
{
    const int w = 4;

    //build a 4x4 raster as points
    SurfacePoints sp;
    for(int y = 0; y < w; y++)
    {
        for(int x = 0; x < w; x++)
        {
            sp.push_back({x, y, y * w + x});
        }
    }

    auto raster = sp.to_vxraster();

    REQUIRE(raster->get_width() == w);
    REQUIRE(raster->get_height() == w);

    for(int y = 0; y < w; y++)
    {
        for(int x = 0; x < w; x++)
        {
            CHECK(raster->value(y, x).z == (w - y - 1) * w + x);
        }
    }
    CHECK(raster->value(0, 0) == Vertex{0, w - 1, (w - 1) * w});
}

TEST_CASE("SurfacePoints::load_from_raster", "[tntn]")
{
    SurfacePoints sp;

    RasterDouble r;
    r.allocate(10, 10);

    //per convention: lower left corner
    r.set_pos_x(100);
    r.set_pos_y(200);
    r.set_cell_size(1.0);

    for(int y = 0; y < r.get_height(); y++)
    {
        for(int x = 0; x < r.get_width(); x++)
        {
            r.value(r.get_height() - y - 1, x) = y * r.get_width() + x;
        }
    }

    sp.load_from_raster(r);

    auto points = sp.points();

    int found_points = 0;
    for(auto p = points.begin; p != points.end; p++)
    {
        const auto pt = *p;
        if(pt.x == 100 && pt.y == 200)
        {
            //lower left corner point
            found_points++;
            CHECK(pt.z == 0 * 10 + 0);
        }

        if(pt.x == 109 && pt.y == 209)
        {
            //upper right corner point
            found_points++;
            CHECK(pt.z == 9 * 10 + 9);
        }

        if(pt.x == 105 && pt.y == 205)
        {
            //center point
            found_points++;
            CHECK(pt.z == 5 * 10 + 5);
        }
    }
    CHECK(found_points == 3);
}

TEST_CASE("SurfacePoints::to_raster orders points correct orientation", "[tntn]")
{
    SurfacePoints sp;
    const int w = 100;
    const int h = 200;
    sp.reserve(w * h);
    for(int y = 100; y < 100 + h; y++)
    {
        for(int x = 100; x < 100 + w; x++)
        {
            sp.push_back({x, y, y * w + x});
        }
    }

    auto raster = sp.to_raster();

    REQUIRE(raster->get_width() == w);
    REQUIRE(raster->get_height() == h);

    CHECK(raster->value(0, 0) == (100 + 199) * w + 100);
    CHECK(raster->value(199, 0) == 100 * w + 100);

    CHECK(raster->value(0, 99) == (100 + 199) * w + 100 + 99);
    CHECK(raster->value(199, 99) == 100 * w + 100 + 99);
}

TEST_CASE("SurfacePoints::to_raster width or height 1", "[tntn]")
{
    SurfacePoints sp;
    const int w = 1;
    const int h = 1;
    sp.reserve(w * h);
    for(int y = 100; y < 100 + h; y++)
    {
        for(int x = 100; x < 100 + w; x++)
        {
            sp.push_back({x, y, y * w + x});
        }
    }

    auto raster = sp.to_raster();

    REQUIRE(raster->get_width() == w);
    REQUIRE(raster->get_height() == h);
    CHECK(raster->value(0, 0) == 100 * 1 + 100);
}

TEST_CASE("SurfacePoints::to_vxraster orders points correct orientation", "[tntn]")
{
    SurfacePoints sp;
    const int w = 100;
    const int h = 200;
    sp.reserve(w * h);
    for(int y = 100; y < 100 + h; y++)
    {
        for(int x = 100; x < 100 + w; x++)
        {
            sp.push_back({x, y, y * w + x});
        }
    }

    auto raster = sp.to_vxraster();

    REQUIRE(raster->get_width() == w);
    REQUIRE(raster->get_height() == h);

    CHECK(raster->value(0, 0).z == (100 + 199) * w + 100);
    CHECK(raster->value(199, 0).z == 100 * w + 100);

    CHECK(raster->value(0, 99).z == (100 + 199) * w + 100 + 99);
    CHECK(raster->value(199, 99).z == 100 * w + 100 + 99);
}

} // namespace unittests
} // namespace tntn
