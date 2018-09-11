#include "catch.hpp"

#include "tntn/geometrix.h"
#include "glm/gtx/normal.hpp"

namespace tntn {
namespace unittests {

TEST_CASE("triangle_semantic_equal equal triangles are detected as equal", "[tntn]")
{
    Triangle t1 = {{
        {0, 0, 0},
        {1, 1, 1},
        {2, 2, 4},
    }};

    Triangle t2 = {{
        {1, 1, 1},
        {2, 2, 4},
        {0, 0, 0},
    }};

    Triangle t3 = {{
        {2, 2, 4},
        {0, 0, 0},
        {1, 1, 1},
    }};

    triangle_semantic_equal eq;
    CHECK(eq(t1, t1));
    CHECK(eq(t1, t2));
    CHECK(eq(t1, t3));

    CHECK(eq(t2, t2));
    CHECK(eq(t2, t3));
    CHECK(eq(t2, t1));

    CHECK(eq(t3, t3));
    CHECK(eq(t3, t1));
    CHECK(eq(t3, t2));
}

TEST_CASE("triangle_semantic_equal triangle with different vertex is not equal", "[tntn]")
{
    Triangle t1 = {{
        {0, 0, 0},
        {1, 1, 1},
        {2, 2, 4},
    }};
    Triangle t2 = {{
        {0, 0, 0},
        {1, 1, 1},
        {3, 3, 9},
    }};

    triangle_semantic_equal eq;
    CHECK(!eq(t1, t2));
    CHECK(!eq(t2, t1));
}

TEST_CASE("triangle_semantic_equal triangle with different winding order is not equal", "[tntn]")
{
    Triangle t1 = {{
        {0, 0, 0},
        {1, 1, 1},
        {2, 2, 4},
    }};
    Triangle t2 = {{
        {0, 0, 0},
        {2, 2, 4},
        {1, 1, 1},
    }};

    triangle_semantic_equal eq;
    CHECK(!eq(t1, t2));
    CHECK(!eq(t2, t1));
}

static bool is_front_facing(const Triangle& t)
{
    glm::dvec3 n = glm::triangleNormal(t[0], t[1], t[2]);
    return n.z > 0;
}

static bool all_front_facing(std::vector<Triangle>& tv)
{
    for(const auto& t : tv)
    {
        if(!is_front_facing(t))
        {
            return false;
        }
    }
    return true;
}

TEST_CASE("BBox2D - add point to default initialized", "[tntn]")
{
    BBox2D bbox;
    bbox.add(glm::dvec2(0, 0));
    CHECK(bbox.min == glm::dvec2(0, 0));
    CHECK(bbox.max == glm::dvec2(0, 0));
}

} // namespace unittests
} // namespace tntn
