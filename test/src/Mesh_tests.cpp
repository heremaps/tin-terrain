#include "catch.hpp"

#include <algorithm>
#include "tntn/Mesh.h"
#include "tntn/geometrix.h"

namespace tntn {
namespace unittests {

TEST_CASE("Mesh from triangles", "[tntn]")
{
    Mesh m;

    std::vector<Triangle> triangles;

    triangles.push_back({{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {2.0, 2.0, 2.0}}});

    m.from_triangles(std::move(triangles));

    CHECK(m.poly_count() == 1);

    CHECK(!m.has_decomposed());

    m.generate_decomposed();

    CHECK(m.has_decomposed());
}

TEST_CASE("Mesh from decomposed", "[tntn]")
{
    Mesh m;

    std::vector<Vertex> vertices;
    std::vector<Face> faces;

    vertices.push_back({0.0, 0.0, 0.0});
    vertices.push_back({1.0, 1.0, 1.0});
    vertices.push_back({2.0, 2.0, 2.0});

    faces.push_back({{0, 1, 2}});

    m.from_decomposed(std::move(vertices), std::move(faces));

    CHECK(m.poly_count() == 1);

    CHECK(!m.has_triangles());

    m.generate_triangles();

    CHECK(m.has_triangles());
}

TEST_CASE("Mesh from added triangles", "[tntn]")
{
    Mesh m;

    m.add_triangle({{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {2.0, 2.0, 2.0}}}, true);

    m.add_triangle({{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {3.0, 3.0, 3.0}}}, true);

    CHECK(m.poly_count() == 2);

    auto vertices = m.vertices();
    CHECK(std::distance(vertices.begin, vertices.end) == 4);
}

#if 1
TEST_CASE("Mesh triangle/decomposed round tripping", "[tntn]")
{
    Mesh tr_m;
    const int c = 10;
    for(int x = 0; x < c; x++)
    {
        for(int y = 0; y < c; y++)
        {
            tr_m.add_triangle({{{x, y, x * y},
                                {x + 1, y + 1, (x + 1) * (y + 1)},
                                {x + 2, y + 2, (x + 2) * (y + 2)}}});
        }
    }
    const size_t poly_count = c * c;

    CHECK(tr_m.poly_count() == poly_count);

    Mesh de_m = tr_m.clone();
    de_m.generate_decomposed();
    de_m.clear_triangles();

    CHECK(de_m.poly_count() == tr_m.poly_count());

    Mesh tr_m2 = de_m.clone();
    tr_m2.generate_triangles();

    std::vector<Triangle> new_triangles;
    tr_m2.grab_triangles(new_triangles);
    std::sort(new_triangles.begin(), new_triangles.end(), triangle_compare<>());

    std::vector<Triangle> original_triangles;
    tr_m.grab_triangles(original_triangles);
    std::sort(original_triangles.begin(), original_triangles.end(), triangle_compare<>());

    CHECK(new_triangles.size() == poly_count);
    CHECK(new_triangles == original_triangles);
}
#endif

TEST_CASE("Mesh move construction", "[tntn]")
{
    Mesh m1;
    m1.add_triangle({{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {2.0, 2.0, 2.0}}});

    CHECK(m1.poly_count() == 1);

    Mesh m2(std::move(m1));

    CHECK(m1.poly_count() == 0);
    CHECK(m2.poly_count() == 1);
}

TEST_CASE("Mesh move assignment", "[tntn]")
{
    Mesh m1;
    m1.add_triangle({{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {2.0, 2.0, 2.0}}});

    CHECK(m1.poly_count() == 1);

    Mesh m2;
    m2 = std::move(m1);

    CHECK(m1.poly_count() == 0);
    CHECK(m2.poly_count() == 1);
}

TEST_CASE("Mesh::check_tin_properties works on simple mesh")
{
    Mesh m;

    m.from_decomposed(
        {
            {0, 0, 1},
            {1, 0, 2},
            {1, 1, 3},
            {0, 1, 4},
            {0.5, -1, 5},
        },
        {
            {{0, 1, 2}}, //ccw
            {{0, 2, 3}}, //ccw
            {{0, 4, 1}}, //ccw
        });

    CHECK(m.check_tin_properties());
}

TEST_CASE("Mesh::check_tin_properties detects not-front facing faces")
{
    Mesh m;

    m.from_decomposed(
        {
            {0, 0, 1},
            {1, 0, 2},
            {1, 1, 3},
        },
        {
            {{0, 2, 1}}, //cw
        });

    CHECK(!m.check_tin_properties());
}

TEST_CASE("Mesh::check_tin_properties detects invalid vertex indices")
{
    Mesh m;

    m.from_decomposed(
        {
            {0, 1, 2},
            {1, 2, 3},
            {2, 3, 4},
        },
        {
            {{0, 1, 2}},
            {{0, 1, 3}},
        });

    CHECK(!m.check_tin_properties());
}

TEST_CASE("Mesh::check_tin_properties detects collapsed vertices in face")
{
    Mesh m;

    m.from_decomposed(
        {
            {0, 1, 2},
            {1, 2, 3},
            {2, 3, 4},
        },
        {
            {{0, 1, 2}},
            {{0, 0, 2}},
        });

    CHECK(!m.check_tin_properties());
}

TEST_CASE("Mesh::check_tin_properties detects duplicate vertices")
{
    Mesh m;

    m.from_decomposed(
        {
            {0, 1, 2},
            {1, 2, 3},
            {0, 1, 2},
        },
        {
            {{0, 1, 2}},
        });

    CHECK(!m.check_tin_properties());
}

TEST_CASE("Mesh::check_tin_properties detects unreferenced vertices")
{
    Mesh m;

    m.from_decomposed(
        {
            {0, 1, 2},
            {1, 2, 3},
            {2, 3, 4},
            {3, 4, 5},
        },
        {
            {{0, 1, 2}},
        });

    CHECK(!m.check_tin_properties());
}

} //namespace unittests
} //namespace tntn
