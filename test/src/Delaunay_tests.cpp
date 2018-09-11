#include "catch.hpp"

#include <chrono>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>

#include "tntn/File.h"

#include "tntn/geometrix.h"
#include "tntn/Points2Mesh.h"

#include "tntn/MeshIO.h"

#include "delaunator_cpp/Delaunator.h"

// data
#include "vertex_points.h"
#include "triangle_indices.h"

namespace tntn {
namespace unittests {

TEST_CASE("delaunator test", "[tntn]")
{

    // super simple - one triangle
    // points in anti-clockwise order

    std::vector<double> ps_simple{0, 0, 1, 1, 1, 0};

    delaunator_cpp::Delaunator dn;
    if(!dn.triangulate(ps_simple))
    {
        CHECK(false);
    }

    CHECK(dn.triangles.size() == 3);

    for(int i = 0; i < dn.triangles.size(); i += 3)
    {
        for(int j = 0; j < 3; j++)
        {
            int pi = dn.triangles[j];
            CHECK(pi == j);
        }
    }

    std::vector<double> vertex_list_big;
    init_vertex_list(vertex_list_big);

    //delaunator_cpp::Delaunator dn_big;
    if(!dn.triangulate(vertex_list_big))
    {
        CHECK(false);
    }

    std::vector<int> tri_big;
    init_triangle_list(tri_big);

    CHECK(dn.triangles.size() == tri_big.size());
    for(int i = 0; i < dn.triangles.size(); i++)
    {
        CHECK(dn.triangles[i] == tri_big[i]);
    }
}

TEST_CASE("simple delaunay", "[tntn]")
{
    std::vector<Vertex> vlist;

    Vertex v;

    // A (i = 0)
    v.x = 0;
    v.y = 0;
    v.z = 0;

    vlist.push_back(v);

    // B  (i = 1)
    v.x = 100;
    v.y = 0;
    v.z = 0;

    vlist.push_back(v);

    // C  (i = 2)
    v.x = 100;
    v.y = 100;
    v.z = 0;

    vlist.push_back(v);

    // D  (i = 3)
    v.x = 0;
    v.y = 100;
    v.z = 0;

    vlist.push_back(v);

    std::vector<Face> faces;
    generate_delaunay_faces(vlist, faces);

    CHECK(faces.size() == 2);

    int h = 10;
    int w = 20;

    vlist.clear();
    for(int r = 0; r < h; r++)
    {
        for(int c = 0; c < w; c++)
        {
            Vertex vvv;
            vvv.x = c;
            vvv.y = r;

            float dx = c - w / 2;
            float dy = r - h / 2;
            if(dx * dx + dy * dy < 30 * 30)
            {
                vvv.z = 20;
            }
            else
                vvv.z = 0;

            vlist.push_back(vvv);
        }
    }

    faces.clear();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    generate_delaunay_faces(vlist, faces);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    CHECK(faces.size() == (w - 1) * (h - 1) * 2);

    //std::cout <<"delaunay on " << vlist.size()/1000.0 << "k vertices in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000.0 << "seconds" << std::endl;
}

} //namespace unittests
} //namespace tntn
