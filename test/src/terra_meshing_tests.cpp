#include "catch.hpp"

#include <algorithm>
#include <cmath>
#include <random>

#include "tntn/TerraMesh.h"
#include "tntn/geometrix.h"
#include "tntn/SurfacePoints.h"
#include "tntn/terra_meshing.h"
#include "tntn/MeshIO.h"

namespace tntn {
namespace unittests {

TEST_CASE("terra ccw works", "[tntn]")
{
    glm::dvec2 a(0, 0);
    glm::dvec2 b(1, 0);
    glm::dvec2 c(1, 1);

    CHECK(terra::ccw(a, b, c));
    CHECK(!terra::ccw(a, c, b));
}

TEST_CASE("terra meshing on artificial terrain", "[tntn]")
{
    auto terrain_fn = [](int x, int y) -> double { return sin(x) * sin(y); };

    auto sp = std::make_unique<SurfacePoints>();

    const int w = 10;
    const int h = 20;

    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            sp->push_back({x, y, terrain_fn(x, y)});
        }
    }

    auto mesh = generate_tin_terra(std::move(sp), 0.1);
    REQUIRE(mesh != nullptr);
    CHECK(mesh->check_tin_properties());

    //write_mesh_as_obj("terrain.obj", *mesh);
}

#if 1

TEST_CASE("terra meshing on artificial terrain with missing points (random deletion)", "[tntn]")
{
    auto terrain_fn = [](int x, int y) -> double { return 10 * sin(x * 0.1) * sin(y * 0.1); };

    const int w = 100;
    const int h = 200;

    std::vector<Vertex> vertices;
    vertices.reserve(h * w);
    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            vertices.push_back({x, y, terrain_fn(x, y)});
        }
    }

    //remove a sequence of points
    std::mt19937 generator(42); //fixed seed

    while(vertices.size() > h * w / 16)
    {
        std::uniform_int_distribution<size_t> dist(0, vertices.size() - 1);
        const size_t point_to_delete = dist(generator);
        vertices.erase(vertices.begin() + point_to_delete);
    }

    auto sp = std::make_unique<SurfacePoints>();
    sp->load_from_memory(std::move(vertices));
    auto mesh = generate_tin_terra(std::move(sp), 0.1);
    REQUIRE(mesh != nullptr);
    CHECK(mesh->check_tin_properties());

    //write_mesh_as_obj("terrain.obj", *mesh);
}
#endif

#if 1
TEST_CASE("terra meshing on artificial terrain with missing points (random insertion)", "[tntn]")
{
    auto terrain_fn = [](int x, int y) -> double { return 10 * sin(x * 0.1) * sin(y * 0.1); };

    const int w = 100;
    const int h = 200;

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
    CHECK(mesh->check_tin_properties());

    //write_mesh_as_obj("terrain.obj", *mesh);
}
#endif

template<typename T>
static void remove_elements(std::vector<T>& v, std::vector<size_t> positions_to_remove)
{
    size_t num_removed = 0;

    for(size_t i = 0; i < positions_to_remove.size(); i++)
    {
        const size_t pos = positions_to_remove[i];
        const size_t swap_pos = v.size() - 1 - num_removed;

        if(pos >= v.size() || swap_pos >= v.size())
        {
            continue;
        }

        std::swap(v[pos], v[swap_pos]);

        for(size_t k = i + 1; k < positions_to_remove.size(); k++)
        {
            if(positions_to_remove[k] == swap_pos)
            {
                positions_to_remove[k] = pos;
            }
        }

        num_removed++;
    }
    v.erase(v.end() - num_removed, v.end());
}

TEST_CASE("terra meshing on 5x5 raster with missing points", "[tntn]")
{

    auto terrain_fn = [](int x, int y) -> double { return sin(x) * sin(y); };

    const int w = 5;
    const int h = 5;

    std::vector<Vertex> vertices;
    vertices.reserve(h * w);
    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            vertices.push_back({x * 2.0, y * 2.0, terrain_fn(x * 2.0, y * 2.0) * 2.0});
        }
    }

#if 1
    remove_elements(vertices,
                    {
                        0,
                        w * h - 1,
                    });
#endif

    auto sp = std::make_unique<SurfacePoints>();
    sp->load_from_memory(std::move(vertices));
    auto mesh = generate_tin_terra(std::move(sp), 0.001);
    REQUIRE(mesh != nullptr);
    CHECK(mesh->check_tin_properties());

    //write_mesh_as_obj("terrain.obj", *mesh);
}

TEST_CASE("terra meshing on 2x2 raster with missing point", "[tntn]")
{

    const int w = 2;
    const int h = 2;

    std::vector<Vertex> vertices;
    vertices.resize(w * h);
    vertices[0] = {0, 0, 0 * w + 0};
    vertices[1] = {0, 1, 1 * w + 0};
    vertices[2] = {1, 0, 0 * w + 1};
    vertices[3] = {1, 1, 1 * w + 1};

#if 1
    remove_elements(vertices,
                    {
                        0,
                        w * h - 1,
                    });
#endif

    auto sp = std::make_unique<SurfacePoints>();
    sp->load_from_memory(std::move(vertices));
    auto mesh = generate_tin_terra(std::move(sp), 0.001);
    REQUIRE(mesh != nullptr);
    CHECK(mesh->check_tin_properties());

    //write_mesh_as_obj("terrain.obj", *mesh);
}

} // namespace unittests
} // namespace tntn
