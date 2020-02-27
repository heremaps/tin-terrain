#pragma once

#include <array>
#include <vector>
#include <algorithm>
#include <utility>
#include <string>

#include "tntn/util.h"

#include "glm/glm.hpp"

namespace tntn {

typedef glm::dvec3 Vertex;
typedef std::array<Vertex, 3> Triangle;

typedef size_t VertexIndex; //0-based index into an array/vector of vertices
typedef std::array<VertexIndex, 3> Face;

typedef glm::dvec3 Normal;
	
struct Edge
{
    Edge() = default;
    Edge(const Edge& other) = default;

    Edge(const VertexIndex a, const VertexIndex b) : first(std::min(a, b)), second(std::max(a, b))
    {
    }
    Edge& operator=(const std::pair<VertexIndex, VertexIndex>& other)
    {
        assign(other.first, other.second);
        return *this;
    }
    Edge& operator=(const Edge& other) = default;

    bool operator==(const Edge& other) const
    {
        return first == other.first && second == other.second;
    }
    bool operator!=(const Edge& other) const
    {
        return first != other.first || second != other.second;
    }

    void assign(const VertexIndex a, const VertexIndex b)
    {
        first = std::min(a, b);
        second = std::max(a, b);
    }

    bool shares_point(const Edge& other) const
    {
        return this->first == other.first || this->first == other.second ||
            this->second == other.first || this->second == other.second;
    }

    bool intersects2D(const Edge& other, const std::vector<Vertex>& vertices) const;

    VertexIndex first;
    VertexIndex second;
};

//for sorting vertices
template<typename CompareT = std::less<double>>
struct vertex_compare
{
    bool operator()(const Vertex& left, const Vertex& right) const
    {
        CompareT compare;
        for(int i = 0; i < 3; i++)
        {
            if(compare(left[i], right[i]))
            {
                return true;
            }
            else if(compare(right[i], left[i]))
            {
                return false;
            }
        }
        return false;
    }
};

//for sorting triangles
template<typename CompareT = std::less<double>>
struct triangle_compare
{
    bool operator()(Triangle left, Triangle right) const
    {
        vertex_compare<CompareT> compare;
        for(int i = 0; i < 3; i++)
        {
            if(compare(left[i], right[i]))
            {
                return true;
            }
            else if(compare(right[i], left[i]))
            {
                return false;
            }
        }
        return false;
    }
};

/**
 semantic comparison of triangles

 two triangles are equal if they are built from the same set of vertices and
 their normals are facing into the same direction
*/
struct triangle_semantic_equal
{
    bool operator()(const Triangle& l, const Triangle& _r) const;
};

bool is_facing_upwards(const Triangle& t);
bool is_facing_upwards(const Face& f, const std::vector<Vertex>& vertices);

struct BBox2D
{
    static constexpr double eps = 0.000000001;

    BBox2D();
    BBox2D(glm::vec2 a, glm::vec2 b);
    BBox2D(glm::dvec2 a, glm::dvec2 b);
    BBox2D(glm::vec3 a, glm::vec3 b);
    BBox2D(glm::dvec3 a, glm::dvec3 b);
    BBox2D(const Triangle& t);

    void reset();

    void add(glm::vec2 point);
    void add(glm::dvec2 point);
    void add(glm::vec3 point);
    void add(glm::dvec3 point);
    void add(const Triangle& t);

    void grow(const double delta);

    template<typename Iterator>
    void add(Iterator begin, const Iterator end)
    {
        std::for_each(begin, end, [this](const auto& v) { add(v); });
    }

    bool intersects(const BBox2D& other, const double epsilon = eps) const;
    bool contains(const glm::dvec2 point, const double epsilon = eps) const;
    bool is_equal(const BBox2D& bb) const;
    bool is_on_border(const glm::dvec2 point, const double epsilon = eps) const;
    std::string to_string() const;

    glm::dvec2 min = {0, 0};
    glm::dvec2 max = {0, 0};
};

struct BBox3D
{
    static constexpr double eps = 0.000000001;

    BBox3D();
    BBox3D(glm::vec3 a, glm::vec3 b);
    BBox3D(glm::dvec3 a, glm::dvec3 b);
    BBox3D(const Triangle& t);

    void reset();

    BBox2D to2D() const { return BBox2D(min.xy(), max.xy()); }

    void add(glm::vec3 point);
    void add(glm::dvec3 point);
    void add(const Triangle& t);

    void grow(const double delta);

    template<typename Iterator>
    void add(Iterator begin, const Iterator end)
    {
        std::for_each(begin, end, [this](const auto& v) { add(v); });
    }

    bool contains(const glm::dvec3 point, const double epsilon = eps) const;

    std::string to_string() const;

    glm::dvec3 min;
    glm::dvec3 max;
};

void clip_25D_triangles_to_01_quadrant(std::vector<Triangle>& tv);

} //namespace tntn

namespace std {

//for using Vertex as a key in unordered_map or unordered_set
template<>
struct hash<::tntn::Vertex>
{
    std::size_t operator()(const ::tntn::Vertex& v) const
    {
        size_t seed = 0;
        for(int i = 0; i < 3; i++)
        {
            ::tntn::hash_combine(seed, v[i]);
        }
        return seed;
    }
};

//for using Edge as a key in unordered_map or unordered_set
template<>
struct hash<::tntn::Edge>
{
    std::size_t operator()(const ::tntn::Edge& e) const
    {
        size_t seed = 0;
        ::tntn::hash_combine(seed, e.first);
        ::tntn::hash_combine(seed, e.second);
        return seed;
    }
};

} //namespace std
