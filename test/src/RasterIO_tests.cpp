#include "catch.hpp"

#include "tntn/Raster.h"
#include "tntn/RasterIO.h"

#include <memory>
#include <cstdlib>
#include <boost/filesystem.hpp>

namespace tntn {
namespace unittests {

namespace fs = boost::filesystem;

// Takes a path relative to fixture directory and returns an absolute file path
fs::path fixture_path(const fs::path& fragment)
{
    // OTN_FIXTURES_PATH is a macro defined via CMakeLists for test suite.
    static fs::path base_fixture_path(OTN_FIXTURES_PATH);
    return base_fixture_path / fs::path(fragment);
}

bool double_eq(double a, double b, double eps)
{
    return std::abs(a - b) < eps;
}

TEST_CASE("load_raster_file", "[tntn]")
{
    RasterDouble raster;
    bool load_result;

    fs::path bogus_file = fixture_path("there.is.no.such.raster.file");

    load_result = load_raster_file(bogus_file.string(), raster);
    REQUIRE(load_result == false);

    fs::path valid_raster_file = fixture_path("valid_raster.tif");

    load_result = load_raster_file(valid_raster_file.string(), raster);
    REQUIRE(load_result == true);
    REQUIRE(!raster.empty());
    REQUIRE(raster.get_width() == 100);
    REQUIRE(raster.get_height() == 80);
    REQUIRE(raster.get_no_data_value() == -3.4028234663852886e+38);

    REQUIRE(double_eq(raster.get_cell_size(), 3.91694, 0.00001));
    REQUIRE(double_eq(raster.get_pos_x(), -13636677.018, 0.001));
    REQUIRE(double_eq(raster.get_pos_y(), 4543905.659, 0.001));
}

} // namespace unittests
} // namespace tntn
