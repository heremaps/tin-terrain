#include "catch.hpp"

#include "tntn/raster_tools.h"
#include "tntn/Raster.h"

namespace tntn {
namespace unittests {

TEST_CASE("sample_nearest_valid_avg works", "[tntn]")
{
    RasterDouble raster;
    raster.allocate(11, 11);
    raster.set_no_data_value(-99999);
    raster.set_all(raster.get_no_data_value());

    const auto w = raster.get_width();
    const auto h = raster.get_height();

    raster.value(0, 0) = 3;
    raster.value(h - 1, 0) = 6;
    raster.value(0, w - 1) = 12;
    raster.value(h - 1, w - 1) = 24;

    raster.value(5, 5) = NAN;

    const double avg_sample = raster_tools::sample_nearest_valid_avg(raster, 5, 5, 3);

    CHECK(avg_sample == (3 + 6 + 12 + 24) / 4.0);
}

} // namespace unittests
} // namespace tntn
