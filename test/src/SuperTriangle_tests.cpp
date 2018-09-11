#include "catch.hpp"

#include "tntn/SuperTriangle.h"
#include "tntn/logging.h"

namespace tntn {
namespace unittests {

TEST_CASE("SuperTriangle test", "[tntn]")
{
    float z = 1234;

    Vertex v1;
    v1.x = 0;
    v1.y = 0;
    v1.z = z;

    Vertex v2;
    v2.x = 0;
    v2.y = 10;
    v2.z = z;

    Vertex v3;
    v3.x = 10;
    v3.y = 10;
    v3.z = z;

    SuperTriangle t(v1, v2, v3);

    bool inside = false;
    double ih;

    inside = t.interpolate(0.5, 0.5, ih);
    TNTN_LOG_DEBUG("ih: {}", ih);

    CHECK(ih == z);

    double ih_1, ih_2, ih_3;
    t.interpolate(0.0, 0.0, ih_1);
    t.interpolate(100.0, 100.0, ih_2);
    t.interpolate(10, 0, ih_3);
}

} // namespace unittests
} // namespace tntn
