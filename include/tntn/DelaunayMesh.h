#pragma once

#include "tntn/QuadEdge.h"
#include "tntn/DelaunayTriangle.h"
#include "tntn/ObjPool.h"

#include "tntn/SurfacePoints.h"
#include "tntn/Mesh.h"

#include <random>

namespace tntn {
namespace terra {

class DelaunayMesh
{
  private:
    std::shared_ptr<ObjPool<QuadEdge>> m_edges;
    std::shared_ptr<ObjPool<DelaunayTriangle>> m_triangles;
    std::mt19937 m_random_gen;

  protected:
    qe_ptr m_starting_edge;
    dt_ptr m_first_face;

    dt_ptr make_face(qe_ptr e);

    void delete_edge(qe_ptr e);

    qe_ptr connect(qe_ptr a, qe_ptr b);
    void swap(qe_ptr e);

    bool ccw_boundary(qe_ptr e);
    bool on_edge(const Point2D, qe_ptr e);

    unsigned int next_random_number()
    {
        return m_random_gen() % std::numeric_limits<unsigned int>::max();
    }

  public:
    DelaunayMesh() :
        m_edges(ObjPool<QuadEdge>::create()),
        m_triangles(ObjPool<DelaunayTriangle>::create()),
        m_random_gen(42) //fixed seed for deterministics sequence of random numbers
    {
        m_edges->reserve(4096);
        m_triangles->reserve(1024);
    }

    void init_mesh(const Point2D a, const Point2D b, const Point2D c, const Point2D d);

    // convenience function using our 2D bounding box
    void init_mesh(const BBox2D& bb)
    {
        const glm::dvec2& a = bb.min;
        const glm::dvec2& d = bb.max;
        const glm::dvec2& b = glm::dvec2(bb.max.x, bb.min.y);
        const glm::dvec2& c = glm::dvec2(bb.min.x, bb.max.y);
        init_mesh(a, b, c, d);
    }

    // virtual function for customization
    virtual bool should_swap(const Point2D, qe_ptr e);
    virtual void scan_triangle(dt_ptr t);

    bool is_interior(qe_ptr e);

    // add a new point to the mesh (add vertex and edges to surrounding vertices)
    qe_ptr spoke(const Point2D x, qe_ptr e);
    void optimize(const Point2D x, qe_ptr e);

    qe_ptr locate(const Point2D x) { return locate(x, m_starting_edge); }
    qe_ptr locate(const Point2D, qe_ptr hint);
    void insert(const Point2D x, dt_ptr tri);

    //void overEdges(edge_callback, void *closure=NULL);
    //void overFaces(face_callback, void *closure=NULL);
};

} //namespace terra
} //namespace tntn
