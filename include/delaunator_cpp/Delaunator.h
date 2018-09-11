#pragma once
#include <vector>
#include <exception>
#include <deque>
#include <memory>
#include <cstdint>

namespace delaunator_cpp {
struct DelaunatorPoint
{
    int64_t i;
    double x;
    double y;
    int64_t t;
    int64_t prev;
    int64_t next;
    bool removed;
};

class Delaunator
{

  public:
    bool triangulate(const std::vector<double>& coords);

    std::vector<uint64_t> triangles;
    std::vector<int64_t> halfedges;

  private:
    double m_center_x;
    double m_center_y;
    int64_t m_hash_size;

    std::vector<int64_t> m_hash;
    std::vector<DelaunatorPoint> m_hull;

  private:
    int64_t hash_key(double x, double y);
    void hash_edge(int64_t e);

    int64_t add_triangle(int64_t i0, int64_t i1, int64_t i2, int64_t a, int64_t b, int64_t c);

    void link(int64_t a, int64_t b);
    int64_t legalize(int64_t a, const std::vector<double>& coords);

    int64_t remove_node(int64_t node);
    int64_t insert_node(int64_t i, const std::vector<double>& coords);
    int64_t insert_node(int64_t i, int64_t prev, const std::vector<double>& coords);
};

} // namespace delaunator_cpp
