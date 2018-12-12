#include "catch.hpp"

#include "tntn/Raster.h"
#include "tntn/RasterIO.h"
#include "tntn/RasterOverviews.h"

#include "test_common.h"

#include <memory>
#include <cstdlib>
#include <boost/filesystem.hpp>

namespace tntn {
namespace unittests {


TEST_CASE("raster_overviews_tests_1", "[tntn]")
{
    auto raster = std::make_unique<RasterDouble>();
    bool load_result;

    fs::path valid_raster_file = fixture_path("valid_raster.tif");

    load_result = load_raster_file(valid_raster_file.string(), *raster);
    REQUIRE(load_result == true);

    int min_zoom = 0;
    int max_zoom = 20;
    RasterOverviews overviews(std::move(raster), min_zoom, max_zoom);

    RasterOverview overview;

    int zooms[1] = {15};
    double resolutions[1] = {3.916942};

    int counter = 0;

    while(overviews.next(overview))
    {
       REQUIRE(counter < 1);
       REQUIRE(overview.zoom_level == zooms[counter]);
       REQUIRE(double_eq(overview.resolution, resolutions[counter], 0.01));

       counter++;
    }
}


TEST_CASE("raster_overviews_tests_2", "[tntn]")
{
    auto raster = std::make_unique<RasterDouble>();
    bool load_result;

    fs::path valid_raster_file = fixture_path("egm84-epsg3857.tif");

    load_result = load_raster_file(valid_raster_file.string(), *raster);
    REQUIRE(load_result == true);

    int min_zoom = 0;
    int max_zoom = 20;
    RasterOverviews overviews(std::move(raster), min_zoom, max_zoom);

    RasterOverview overview;

    int zooms[2] = {1, 0};
    double resolutions[2] = {64187.510960, 128375.021920};

    int counter = 0;

    while(overviews.next(overview))
    {
       REQUIRE(counter < 2);
       REQUIRE(overview.zoom_level == zooms[counter]);
       REQUIRE(double_eq(overview.resolution, resolutions[counter], 0.01));

       counter++;
    }
}


} // namespace unittests
} // namespace tntn
