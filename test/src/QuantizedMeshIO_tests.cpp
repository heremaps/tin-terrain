#include "catch.hpp"

#include <random>

#include "tntn/QuantizedMeshIO.h"
#include "tntn/MeshIO.h"
#include "tntn/terra_meshing.h"
#include "tntn/geometrix.h"

using namespace tntn::detail;

namespace tntn {
namespace unittests {

TEST_CASE("zig_zag_encode on reference values", "[tntn]")
{
    CHECK(zig_zag_encode(0) == 0);
    CHECK(zig_zag_encode(-1) == 1);
    CHECK(zig_zag_encode(1) == 2);
    CHECK(zig_zag_encode(-2) == 3);
    CHECK(zig_zag_encode(2) == 4);

    CHECK(zig_zag_encode(16383) == 16383 * 2);
    CHECK(zig_zag_encode(-16383) == 16383 * 2 - 1);

    CHECK(zig_zag_encode(16384) == 16384 * 2);
    CHECK(zig_zag_encode(-16384) == 16384 * 2 - 1);

    CHECK(zig_zag_encode(16385) == 16385 * 2);
    CHECK(zig_zag_encode(-16385) == 16385 * 2 - 1);

    CHECK(zig_zag_encode(32767) == 32767 * 2);
    CHECK(zig_zag_encode(-32767) == 32767 * 2 - 1);

    CHECK(zig_zag_encode(-32768) == 32768 * 2 - 1);
}

TEST_CASE("zig_zag_decode on reference values", "[tntn]")
{
    CHECK(zig_zag_decode(0) == 0);
    CHECK(zig_zag_decode(1) == -1);
    CHECK(zig_zag_decode(2) == 1);
    CHECK(zig_zag_decode(3) == -2);
    CHECK(zig_zag_decode(4) == 2);

    CHECK(zig_zag_decode(16383 * 2) == 16383);
    CHECK(zig_zag_decode(16383 * 2 - 1) == -16383);

    CHECK(zig_zag_decode(16384 * 2) == 16384);
    CHECK(zig_zag_decode(16384 * 2 - 1) == -16384);

    CHECK(zig_zag_decode(16385 * 2) == 16385);
    CHECK(zig_zag_decode(16385 * 2 - 1) == -16385);

    CHECK(zig_zag_decode(32767 * 2) == 32767);
    CHECK(zig_zag_decode(32767 * 2 - 1) == -32767);

    CHECK(zig_zag_decode(32768 * 2 - 1) == -32768);
}

#if 1
TEST_CASE("quantized mesh writer/loader round trip on small mesh", "[tntn]")
{
    const int xscale = 2;
    const int yscale = 1;
    const double scale = (xscale + yscale) / 2.0;

    auto terrain_fn = [=](int x, int y) -> double {
        return 10 * scale * sin(x * 0.1 / scale) * sin(y * 0.1 / scale);
    };

    const int w = 100 * xscale;
    const int h = 100 * yscale;

    std::vector<Vertex> vertices;
    vertices.reserve(h * w);
    std::mt19937 generator(42); //fixed seed
    std::uniform_int_distribution<size_t> dist(0, 16);

    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            if(dist(generator) == 0)
            {
                vertices.push_back({x, y, terrain_fn(x, y)});
            }
        }
    }

    auto sp = std::make_unique<SurfacePoints>();
    sp->load_from_memory(std::move(vertices));
    auto mesh = generate_tin_terra(std::move(sp), 0.1);
    REQUIRE(mesh != nullptr);
    //CHECK(mesh->check_tin_properties());

    const auto original_faces_count = mesh->faces().distance();

    mesh->generate_triangles();
    CHECK(mesh->triangles().distance() == original_faces_count);

    auto mf = std::make_shared<MemoryFile>();
    //write_mesh_as_obj("terrain.obj", *mesh);
    //write_mesh_as_qm("terrain.terrain", *mesh);
    CHECK(write_mesh_as_qm(mf, *mesh));

    auto loaded_mesh = load_mesh_from_qm(mf);
    REQUIRE(loaded_mesh != nullptr);
    CHECK(loaded_mesh->faces().distance() == original_faces_count);

    //TODO: investigate why this doesn't hold. Probably because of quantization differences?
    //CHECK(loaded_mesh->semantic_equal(*mesh));

    //write_mesh_as_obj("terrain.terrain.obj", *mesh);
}
#endif

#if 1
TEST_CASE("quantized mesh writer/loader round trip on empty mesh", "[tntn]")
{
    auto mesh = std::make_shared<Mesh>();

    auto mf = std::make_shared<MemoryFile>();

    write_mesh_as_qm(mf, *mesh);
    auto loaded_mesh = load_mesh_from_qm(mf);
    REQUIRE(loaded_mesh == nullptr);
    // CHECK(loaded_mesh->semantic_equal(*mesh));
}
#endif

#if 0
TEST_CASE("load reference quantized mesh", "[tntn]")
{
    auto loaded_mesh = load_mesh_from_qm("/Users/dietrich/Downloads/no-extensions.terrain");
    REQUIRE(loaded_mesh != nullptr);
    loaded_mesh->generate_triangles();
    REQUIRE(loaded_mesh->check_tin_properties());

    write_mesh_as_obj("no-extensions.obj", *loaded_mesh);
    write_mesh_as_qm("no-extensions.rewrite.terrain", *loaded_mesh);
    auto reloaded_mesh = load_mesh_from_qm("no-extensions.rewrite.terrain");
    REQUIRE(reloaded_mesh != nullptr);
    write_mesh_as_obj("no-extensions.rewrite.terrain.obj", *reloaded_mesh);
    REQUIRE(reloaded_mesh->check_tin_properties());

    CHECK(loaded_mesh->semantic_equal(*reloaded_mesh));
}
#endif

} // namespace unittests
} // namespace tntn
