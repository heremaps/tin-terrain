#include "catch.hpp"

#include <iostream>

#include "tntn/Mesh.h"
#include "tntn/MeshIO.h"
#include "tntn/OFFReader.h"

namespace tntn {
namespace unittests {

TEST_CASE("OFFReader interop with write_mesh_as_off", "[tntn]")
{
    Mesh m;

    m.add_triangle({{{0, 0, 0}, {1, 1, 1}, {2, 2, 2}}});

    m.add_triangle({{{0, 0, 0}, {2, 2, 2}, {3, 3, 3}}});

    m.add_triangle({{{1, 1, 1}, {2, 2, 2}, {3, 3, 3}}});

    m.add_triangle({{{3, 3, 3}, {2, 2, 2}, {1, 1, 1}}});

    m.add_triangle({{{M_PI, -M_PI, 0.123456789123456789123456789123456789},
                     {std::numeric_limits<double>::max(),
                      M_PI * M_PI * M_PI * M_PI,
                      std::numeric_limits<double>::min()},
                     {-std::numeric_limits<double>::max(),
                      -M_PI * M_PI,
                      -std::numeric_limits<double>::min()}}});

    m.generate_decomposed();

    MemoryFile out_mem;
    REQUIRE(write_mesh_as_off(out_mem, m));

    CHECK(out_mem.size() > 0);

    auto m2 = load_mesh_from_off(out_mem);
    REQUIRE(m2 != nullptr);

    // do dimensions argee
    CHECK(m2->poly_count() == 5);
    CHECK(m2->poly_count() == m.poly_count());

    // simple value check on vertices
    SimpleRange<const Vertex*> v_src = m.vertices();
    SimpleRange<const Vertex*> v_dst = m2->vertices();

    CHECK(v_src.distance() == v_dst.distance());

    for(int i = 0; i < v_dst.distance(); i++)
    {
        double prec = 1e-15;

        double dx = std::abs(v_src.begin[i].x - v_dst.begin[i].x);
        double dy = std::abs(v_src.begin[i].y - v_dst.begin[i].y);
        double dz = std::abs(v_src.begin[i].z - v_dst.begin[i].z);

        CHECK(dx <= prec);
        CHECK(dy <= prec);
        CHECK(dz <= prec);
    }

    // semantic equal
    //CHECK(m.semantic_equal(*m2));
}

} // namespace unittests
} // namespace tntn
