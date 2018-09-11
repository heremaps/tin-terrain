#include "tntn/Mesh.h"

#include <utility>
#include <memory>
#include <limits>
#include <unordered_map>
#include <unordered_set>

#include "tntn/tntn_assert.h"
#include "tntn/logging.h"
#include "tntn/geometrix.h"

namespace tntn {

void Mesh::clear()
{
    m_triangles.clear();
    m_vertices.clear();
    m_faces.clear();
}

void Mesh::clear_triangles()
{
    m_triangles.clear();
}

void Mesh::clear_decomposed()
{
    m_vertices.clear();
    m_faces.clear();
}

Mesh Mesh::clone() const
{
    Mesh out;
    out.m_faces = m_faces;
    out.m_vertices = m_vertices;
    out.m_triangles = m_triangles;
    return out;
}

void Mesh::from_decomposed(std::vector<Vertex>&& vertices, std::vector<Face>&& faces)
{
    clear();
    m_vertices = std::move(vertices);
    m_faces = std::move(faces);
}

void Mesh::from_triangles(std::vector<Triangle>&& triangles)
{
    m_triangles = std::move(triangles);
}

size_t Mesh::poly_count() const
{
    TNTN_ASSERT((!has_triangles() || !has_decomposed()) ||
                (has_triangles() && has_decomposed() && m_triangles.size() == m_faces.size()));
    return has_triangles() ? m_triangles.size() : m_faces.size();
}

void Mesh::add_triangle(const Triangle& t, const bool decompose)
{
    m_triangles.push_back(t);
    if(decompose || has_decomposed())
    {
        decompose_triangle(t);
    }
}

void Mesh::generate_triangles()
{

    if(has_triangles())
    {
        return;
    }

    TNTN_LOG_DEBUG("generate triangels...");

    m_triangles.reserve(m_faces.size());

    Triangle t;
    for(const auto& face : m_faces)
    {
        bool is_valid = true;
        for(int i = 0; i < 3; i++)
        {
            const VertexIndex vi = face[i];
            if(vi < m_vertices.size())
            {
                t[i] = m_vertices[vi];
            }
            else
            {
                is_valid = false;
            }
        }
        if(is_valid)
        {
            m_triangles.push_back(t);
        }
    }

    TNTN_LOG_DEBUG("done");
}

void Mesh::generate_decomposed()
{
    if(has_decomposed()) return;

    TNTN_LOG_DEBUG("generate decomposed...");

    m_faces.reserve(m_triangles.size());
    m_vertices.reserve(m_triangles.size() * 0.66);

    std::unordered_map<Vertex, VertexIndex> vertex_lookup;
    vertex_lookup.reserve(m_vertices.capacity());

    for(const auto& t : m_triangles)
    {
        Face f;
        for(int i = 0; i < 3; i++)
        {
            const Vertex& v = t[i];
            const auto v_it = vertex_lookup.find(v);
            if(v_it != vertex_lookup.end())
            {
                //vertex already in m_vertices;
                f[i] = v_it->second;
            }
            else
            {
                //new vertex
                f[i] = m_vertices.size();
                vertex_lookup.emplace_hint(v_it, v, m_vertices.size());
                m_vertices.push_back(v);
            }
        }
        m_faces.push_back(f);
    }

    TNTN_LOG_DEBUG("done");
}

void Mesh::decompose_triangle(const Triangle& t)
{
    Face f = {{std::numeric_limits<size_t>::max(),
               std::numeric_limits<size_t>::max(),
               std::numeric_limits<size_t>::max()}};

    int found = 0;
    for(size_t j = 0; j < m_vertices.size() && found < 3; j++)
    {
        const auto& v = m_vertices[j];
        for(int i = 0; i < 3; i++)
        {
            if(t[i] == v)
            {
                f[i] = j;
                found++;
            }
        }
    }
    if(found != 3)
    {
        for(int i = 0; i < 3; i++)
        {
            if(f[i] == std::numeric_limits<size_t>::max())
            {
                f[i] = m_vertices.size();
                m_vertices.push_back(t[i]);
            }
        }
    }

    m_faces.push_back(f);
}

bool Mesh::compose_triangle(const Face& f, Triangle& out) const
{
    //check validity of indices
    for(int i = 0; i < 3; i++)
    {
        if(f[i] >= m_vertices.size()) return false;
    }
    for(int i = 0; i < 3; i++)
    {
        out[i] = m_vertices[f[i]];
    }
    return true;
}

SimpleRange<const Triangle*> Mesh::triangles() const
{
    if(m_triangles.empty())
    {
        return {nullptr, nullptr};
    }
    return {m_triangles.data(), m_triangles.data() + m_triangles.size()};
}

SimpleRange<const Face*> Mesh::faces() const
{
    if(m_faces.empty())
    {
        return {nullptr, nullptr};
    }
    return {m_faces.data(), m_faces.data() + m_faces.size()};
}

SimpleRange<const Vertex*> Mesh::vertices() const
{
    if(m_vertices.empty())
    {
        return {nullptr, nullptr};
    }
    return {m_vertices.data(), m_vertices.data() + m_vertices.size()};
}

void Mesh::grab_triangles(std::vector<Triangle>& into)
{
    into.clear();
    into.swap(m_triangles);
}

void Mesh::grab_decomposed(std::vector<Vertex>& vertices, std::vector<Face>& faces)
{
    vertices.clear();
    faces.clear();

    vertices.swap(m_vertices);
    faces.swap(m_faces);
}

bool Mesh::semantic_equal(const Mesh& other) const
{
    if(poly_count() != other.poly_count())
    {
        return false;
    }

    if(has_triangles() && other.has_triangles())
    {
        //both triangles
        return semantic_equal_tri_tri(other);
    }
    else if(has_decomposed() && other.has_decomposed())
    {
        //both vertices/faces
        return semantic_equal_dec_dec(other);
    }
    else
    {
        //one triangles, one faces
        if(has_triangles() && other.has_decomposed())
        {
            return semantic_equal_tri_dec(other);
        }
        else if(has_decomposed() && other.has_triangles())
        {
            return other.semantic_equal_tri_dec(*this);
        }
        else
        {
            //empty meshes
            TNTN_ASSERT(empty() && other.empty());
            return true;
        }
    }
}

bool Mesh::semantic_equal_tri_tri(const Mesh& other) const
{
    const auto other_tris = other.triangles();
    std::vector<const Triangle*> other_tri_ptrs;
    other_tri_ptrs.reserve(other_tris.distance());
    other_tris.for_each([&other_tri_ptrs](const Triangle& t) { other_tri_ptrs.push_back(&t); });

    //for each triangle find and equivalent triangle in other
    // if found, remove from list so that cardinalities are honored
    triangle_semantic_equal eq;
    for(const Triangle& t : m_triangles)
    {
        bool found_semantic_equal = false;
        for(size_t i = 0; i < other_tri_ptrs.size(); i++)
        {
            if(other_tri_ptrs[i] != nullptr && eq(t, *other_tri_ptrs[i]))
            {
                other_tri_ptrs[i] = nullptr;
                found_semantic_equal = true;
                break;
            }
        }
        if(!found_semantic_equal) return false;
    }
    return true;
}

bool Mesh::semantic_equal_dec_dec(const Mesh& other) const
{
    const auto other_faces = other.faces();
    std::vector<const Face*> other_face_ptrs;
    other_face_ptrs.reserve(other_faces.distance());
    other_faces.for_each([&other_face_ptrs](const Face& f) { other_face_ptrs.push_back(&f); });

    //for each face, find equivalend face in other
    // if found, remove from list so that cardinalities are honored
    triangle_semantic_equal eq;
    Triangle t1;
    Triangle t2;
    for(const Face& f : m_faces)
    {
        if(!compose_triangle(f, t1))
        {
            continue;
        }
        bool found_semantic_equal = false;
        for(size_t i = 0; i < other_face_ptrs.size(); i++)
        {
            if(other_face_ptrs[i] != nullptr)
            {
                if(!other.compose_triangle(*other_face_ptrs[i], t2))
                {
                    other_face_ptrs[i] = nullptr;
                    continue;
                }
                if(eq(t1, t2))
                {
                    other_face_ptrs[i] = nullptr;
                    found_semantic_equal = true;
                    break;
                }
            }
        }
        if(!found_semantic_equal) return false;
    }
    return true;
}

bool Mesh::semantic_equal_tri_dec(const Mesh& other) const
{
    TNTN_ASSERT(this->has_triangles());
    TNTN_ASSERT(other.has_decomposed());

    const auto other_faces = other.faces();
    std::vector<const Face*> other_face_ptrs;
    other_face_ptrs.reserve(other_faces.distance());
    other_faces.for_each([&other_face_ptrs](const Face& f) { other_face_ptrs.push_back(&f); });

    //for each triangle, find equivalend face in other
    // if found, remove from list so that cardinalities are honored
    triangle_semantic_equal eq;
    Triangle t2;
    for(const Triangle& t1 : m_triangles)
    {
        bool found_semantic_equal = false;
        for(size_t i = 0; i < other_face_ptrs.size(); i++)
        {
            if(other_face_ptrs[i] != nullptr)
            {
                if(!other.compose_triangle(*other_face_ptrs[i], t2))
                {
                    other_face_ptrs[i] = nullptr;
                    continue;
                }
                if(eq(t1, t2))
                {
                    other_face_ptrs[i] = nullptr;
                    found_semantic_equal = true;
                    break;
                }
            }
        }
        if(!found_semantic_equal) return false;
    }
    return true;
}

void Mesh::get_bbox(BBox3D& bbox) const
{
    bbox.add(m_vertices.begin(), m_vertices.end());
}

static bool face_edge_crosses_other_edge(const size_t fi,
                                         const std::vector<Face>& faces,
                                         const std::vector<Vertex>& vertices)
{
    const Face f = faces[fi];
    const std::array<Edge, 3> face_edges = {{
        {f[0], f[1]},
        {f[1], f[2]},
        {f[2], f[0]},
    }};

    Triangle ft;
    ft[0] = vertices[f[0]];
    ft[1] = vertices[f[1]];
    ft[2] = vertices[f[2]];

    BBox2D ft_bbox(ft);

    for(size_t oi = fi + 1; oi < faces.size(); oi++)
    {
        const Face o = faces[oi];

        const Triangle ot = {{
            vertices[o[0]],
            vertices[o[1]],
            vertices[o[2]],
        }};

        BBox2D ot_bbox(ot);
        if(ft_bbox.intersects(ot_bbox))
        {
            for(const Edge& e : face_edges)
            {
                Edge oe = {o[0], o[1]};
                if(!e.shares_point(oe) && e.intersects2D(oe, vertices))
                {
                    return true;
                }

                oe.assign(o[1], o[2]);
                if(!e.shares_point(oe) && e.intersects2D(oe, vertices))
                {
                    return true;
                }
                oe.assign(o[2], o[0]);
                if(!e.shares_point(oe) && e.intersects2D(oe, vertices))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool Mesh::check_tin_properties() const
{
    TNTN_LOG_DEBUG("checking mesh consistency / TIN properties...");
    if(!has_decomposed())
    {
        return false;
    }

    //O(n) checks first
    {
        const size_t vertices_size = m_vertices.size();
        std::vector<char> vertex_used(vertices_size);
        for(const Face& f : m_faces)
        {
            //all vertex indices are valid
            if(f[0] >= vertices_size || f[1] >= vertices_size || f[2] >= vertices_size)
            {
                TNTN_LOG_DEBUG(
                    "mesh is NOT a regular/propper TIN, some face indices are not valid");
                return false;
            }

            //check if vertices are referenced
            vertex_used[f[0]] = 1;
            vertex_used[f[1]] = 1;
            vertex_used[f[2]] = 1;

            //no face has two collapsed corner points
            if(f[0] == f[1] || f[0] == f[2] || f[1] == f[2])
            {
                TNTN_LOG_DEBUG(
                    "mesh is NOT a regular/propper TIN, some faces have collapsed corner points");
                return false;
            }

            //face is oriented upwards
            if(!is_facing_upwards(f, m_vertices))
            {
                TNTN_LOG_DEBUG(
                    "triangle is NOT a regular/propper TIN, some faces are not oriented upwards");
                return false;
            }
        }

        for(const char is_used : vertex_used)
        {
            if(!is_used)
            {
                TNTN_LOG_DEBUG(
                    "mesh is NOT a regular/propper TIN, some vertices are not referenced by a face");
                return false;
            }
        }
    }

    //no duplicate vertices (this is O(n*log(n))
    {
        std::vector<Vertex> vertices_sorted;
        vertices_sorted.resize(m_vertices.size());
        std::partial_sort_copy(m_vertices.begin(),
                               m_vertices.end(),
                               vertices_sorted.begin(),
                               vertices_sorted.end(),
                               vertex_compare<>());
        auto it = std::adjacent_find(vertices_sorted.begin(), vertices_sorted.end());
        if(it != vertices_sorted.end())
        {
            TNTN_LOG_DEBUG("mesh is NOT a regular/propper TIN, there are duplicate vertices");
            return false;
        }
    }

    //no overlapping triangles ( this is O(n^2) :/ )
    for(size_t fi = 0; fi < m_faces.size(); fi++)
    {
        //edges are either the same or do not intersect
        if(face_edge_crosses_other_edge(fi, m_faces, m_vertices))
        {
            TNTN_LOG_DEBUG("mesh is NOT a regular/propper TIN, some triangles are overlapping");
            return false;
        }
    }

#if 0
    //euler characteristics
    //https://en.wikipedia.org/wiki/Euler_characteristic
    {
        //TODO: transform mesh to topological sphere (not a geometrical sphere) by connecting all outer vertices into one

        //collapse bbox boundary vertices into a single pole
        BBox2D bbox;
        bbox.add(m_vertices.begin(), m_vertices.end());
        const Vertex pole = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        auto vertices = m_vertices;
        std::vector<VertexIndex> collapsed_vertices;
        for(size_t vi = 0; vi<vertices.size(); vi++) {
            Vertex &v = vertices[vi];
            if(bbox.is_on_border(v)) {
                v = pole;
                collapsed_vertices.push_back(vi);
            }
        }

        //WIP ... needs to be continued


        std::unordered_set<Edge> edges;
        for(const Face &f : m_faces) {
            edges.emplace(f[0], f[1]);
            edges.emplace(f[1], f[2]);
            edges.emplace(f[2], f[0]);
        }
        const int64_t E = edges.size();
        const int64_t V = m_vertices.size();
        const int64_t F = m_faces.size();

        const int64_t chi = V - E + F;
        if(chi != 2) {
            TNTN_LOG_DEBUG("mesh is NOT a regular/propper TIN, some triangles are missing (euler charachteristics violated)");
            return false;
        }
    }
#endif
    TNTN_LOG_DEBUG("mesh is a regular/propper TIN");
    return true;
}

} //namespace tntn
