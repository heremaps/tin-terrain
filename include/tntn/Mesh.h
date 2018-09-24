#pragma once

#include "tntn/geometrix.h"
#include "tntn/util.h"
#include "tntn/MeshMode.h"

#include <array>
#include <vector>

#include "glm/glm.hpp"

namespace tntn {

class Mesh
{
  private:
    //disallow copy and assign, force passing by pointer or move semantics
    Mesh(const Mesh& other) = delete;
    Mesh& operator=(const Mesh& other) = delete;

  public:
    Mesh() = default;
    Mesh(Mesh&& other) = default;
    Mesh& operator=(Mesh&& other) = default;

    void clear();
    Mesh clone() const;

    void from_decomposed(std::vector<Vertex>&& vertices, std::vector<Face>&& faces);
    void from_triangles(std::vector<Triangle>&& triangles);
    void add_triangle(const Triangle& t, const bool decompose = false);

    size_t poly_count() const;
    bool empty() const { return poly_count() == 0; }

    bool has_triangles() const { return !m_triangles.empty(); }
    bool has_decomposed() const { return !m_vertices.empty() && !m_faces.empty(); }

    void generate_triangles();
    void generate_decomposed();
    void clear_triangles();
    void clear_decomposed();

    SimpleRange<const Triangle*> triangles() const;
    SimpleRange<const Face*> faces() const;
    SimpleRange<const Vertex*> vertices() const;

    void grab_triangles(std::vector<Triangle>& into);
    void grab_decomposed(std::vector<Vertex>& vertices, std::vector<Face>& faces);

    bool compose_triangle(const Face& f, Triangle& out) const;

    /**
     Two meshes are semantically equal iff all their triangles are semantically equal
     */
    bool semantic_equal(const Mesh& other) const;

    void get_bbox(BBox3D& bbox) const;

    bool check_tin_properties() const;

    bool is_square() const;
    bool check_for_holes_in_square_mesh() const;

  private:
    bool semantic_equal_tri_tri(const Mesh& other) const;
    bool semantic_equal_dec_dec(const Mesh& other) const;
    bool semantic_equal_tri_dec(const Mesh& other) const;

    std::vector<Vertex> m_vertices;
    std::vector<Face> m_faces;
    std::vector<Triangle> m_triangles;

    void decompose_triangle(const Triangle& t);
};

} // namespace tntn
