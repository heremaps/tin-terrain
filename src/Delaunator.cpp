
#include "delaunator_cpp/Delaunator.h"
#include <algorithm>
#include <cstdio>
#include <limits>
#include <tuple>
#include <exception>
#include <cmath>
#include <iostream>
#include <cassert>

using namespace std;

namespace delaunator_cpp {
namespace {
const double max_double = numeric_limits<double>::max();
double dist(const double& ax, const double& ay, const double& bx, const double& by)
{
    const double dx = ax - bx;
    const double dy = ay - by;
    return dx * dx + dy * dy;
}

double circumradius(const double& ax,
                    const double& ay,
                    const double& bx,
                    const double& by,
                    const double& cx,
                    const double& cy)
{
    const double dx = bx - ax;
    const double dy = by - ay;
    const double ex = cx - ax;
    const double ey = cy - ay;

    const double bl = dx * dx + dy * dy;
    const double cl = ex * ex + ey * ey;
    const double d = dx * ey - dy * ex;

    const double x = (ey * bl - dy * cl) * 0.5 / d;
    const double y = (dx * cl - ex * bl) * 0.5 / d;

    if(bl && cl && d)
    {
        return x * x + y * y;
    }
    else
    {
        return max_double;
    }
}

double area(const double px,
            const double py,
            const double qx,
            const double qy,
            const double rx,
            const double ry)
{
    return (qy - py) * (rx - qx) - (qx - px) * (ry - qy);
}

tuple<double, double> circumcenter(const double& ax,
                                   const double& ay,
                                   const double& bx,
                                   const double& by,
                                   const double& cx,
                                   const double& cy)
{
    const double dx = bx - ax;
    const double dy = by - ay;
    const double ex = cx - ax;
    const double ey = cy - ay;

    const double bl = dx * dx + dy * dy;
    const double cl = ex * ex + ey * ey;
    const double d = dx * ey - dy * ex;

    const double x = ax + (ey * bl - dy * cl) * 0.5 / d;
    const double y = ay + (dx * cl - ex * bl) * 0.5 / d;

    return make_tuple(x, y);
}

double compare(const vector<double>& coords, uint64_t i, uint64_t j, double cx, double cy)
{
    const double d1 = dist(coords[2 * i], coords[2 * i + 1], cx, cy);
    const double d2 = dist(coords[2 * j], coords[2 * j + 1], cx, cy);
    const double diff1 = d1 - d2;
    const double diff2 = coords[2 * i] - coords[2 * j];
    const double diff3 = coords[2 * i + 1] - coords[2 * j + 1];

    if(diff1)
    {
        return diff1;
    }
    else if(diff2)
    {
        return diff2;
    }
    else
    {
        return diff3;
    }
}

bool in_circle(
    double ax, double ay, double bx, double by, double cx, double cy, double px, double py)
{
    const double dx = ax - px;
    const double dy = ay - py;
    const double ex = bx - px;
    const double ey = by - py;
    const double fx = cx - px;
    const double fy = cy - py;

    const double ap = dx * dx + dy * dy;
    const double bp = ex * ex + ey * ey;
    const double cp = fx * fx + fy * fy;

    return (dx * (ey * cp - bp * fy) - dy * (ex * cp - bp * fx) + ap * (ex * fy - ey * fx)) < 0;
}
} // namespace

bool Delaunator::triangulate(const vector<double>& coords)
{
    m_center_x = 0;
    m_center_y = 0;
    m_hash_size = 0;

    m_hash.clear();
    m_hull.clear();
    triangles.clear();
    halfedges.clear();

    //coords = move(in_coords);
    const int64_t n = coords.size() / 2;
    double max_x = -1 * max_double;
    double max_y = -1 * max_double;
    double min_x = max_double;
    double min_y = max_double;

    std::vector<int64_t> ids;
    ids.reserve(n);
    for(int64_t i = 0; i < n; i++)
    {
        const double x = coords[2 * i];
        const double y = coords[2 * i + 1];
        // print64_tf("%f %f", x, y);

        if(x < min_x) min_x = x;
        if(y < min_y) min_y = y;
        if(x > max_x) max_x = x;
        if(y > max_y) max_y = y;
        // ids[i] = i;
        ids.push_back(i);
    }
    const double cx = (min_x + max_x) / 2;
    const double cy = (min_y + max_y) / 2;
    double min_dist = max_double;
    uint64_t i0;
    uint64_t i1;
    uint64_t i2;

    // pick a seed point64_t close to the centroid
    for(uint64_t i = 0; i < n; i++)
    {
        assert((2 * i + 1 < coords.size()) && i >= 0);

        const double d = dist(cx, cy, coords[2 * i], coords[2 * i + 1]);
        if(d < min_dist)
        {
            i0 = i;
            min_dist = d;
        }
    }

    min_dist = max_double;

    // find the point64_t closest to the seed
    for(uint64_t i = 0; i < n; i++)
    {
        if(i == i0) continue;
        const double d =
            dist(coords[2 * i0], coords[2 * i0 + 1], coords[2 * i], coords[2 * i + 1]);
        if(d < min_dist && d > 0)
        {
            i1 = i;
            min_dist = d;
        }
    }

    double min_radius = max_double;

    // find the third point64_t which forms the smallest circumcircle with the first two
    for(uint64_t i = 0; i < n; i++)
    {
        if(i == i0 || i == i1) continue;

        const double r = circumradius(coords[2 * i0],
                                      coords[2 * i0 + 1],
                                      coords[2 * i1],
                                      coords[2 * i1 + 1],
                                      coords[2 * i],
                                      coords[2 * i + 1]);

        if(r < min_radius)
        {
            i2 = i;
            min_radius = r;
        }
    }

    if(min_radius == max_double)
    {
        return false;
        //throw runtime_error("no triangulation: min_radius == max_double");
    }

    if(area(coords[2 * i0],
            coords[2 * i0 + 1],
            coords[2 * i1],
            coords[2 * i1 + 1],
            coords[2 * i2],
            coords[2 * i2 + 1]) < 0)
    {
        const double tmp = i1;
        i1 = i2;
        i2 = tmp;
    }

    const double i0x = coords[2 * i0];
    const double i0y = coords[2 * i0 + 1];
    const double i1x = coords[2 * i1];
    const double i1y = coords[2 * i1 + 1];
    const double i2x = coords[2 * i2];
    const double i2y = coords[2 * i2 + 1];

    tie(m_center_x, m_center_y) = circumcenter(i0x, i0y, i1x, i1y, i2x, i2y);

    std::sort(ids.begin(), ids.end(), [&](int64_t i, int64_t j) {
        return compare(coords, i, j, m_center_x, m_center_y) < 0;
    });

    m_hash_size = ceil(sqrt(n));
    m_hash.reserve(m_hash_size);
    for(int64_t i = 0; i < m_hash_size; i++)
        m_hash.push_back(-1);

    m_hull.reserve(coords.size());

    int64_t e = insert_node(i0, coords);
    hash_edge(e);
    m_hull[e].t = 0;

    e = insert_node(i1, e, coords);
    hash_edge(e);
    m_hull[e].t = 1;

    e = insert_node(i2, e, coords);
    hash_edge(e);
    m_hull[e].t = 2;

    const int64_t max_triangles = 2 * n - 5;
    triangles.reserve(max_triangles * 3);
    halfedges.reserve(max_triangles * 3);
    add_triangle(i0, i1, i2, -1, -1, -1);
    double xp = NAN;
    double yp = NAN;
    for(int64_t k = 0; k < n; k++)
    {
        const int64_t i = ids[k];

        assert(i >= 0 && i < coords.size() / 2);

        const double x = coords[2 * i];
        const double y = coords[2 * i + 1];
        if(x == xp && y == yp) continue;
        xp = x;
        yp = y;
        if((x == i0x && y == i0y) || (x == i1x && y == i1y) || (x == i2x && y == i2y)) continue;

        const int64_t start_key = hash_key(x, y);
        int64_t key = start_key;
        int64_t start = -1;
        do
        {
            assert(key >= 0 && key < m_hash.size());
            start = m_hash[key];
            key = (key + 1) % m_hash_size;
        } while((start < 0 || m_hull[start].removed) && (key != start_key));

        e = start;
        assert(e >= 0 && e < m_hull.size());
        while(area(x,
                   y,
                   m_hull[e].x,
                   m_hull[e].y,
                   m_hull[m_hull[e].next].x,
                   m_hull[m_hull[e].next].y) >= 0)
        {
            e = m_hull[e].next;
            assert(e >= 0 && e < m_hull.size());
            if(e == start)
            {
                return false;
                //throw runtime_error("Something is wrong with the input point64_ts.");
            }
        }

        const bool walk_back = e == start;

        // add the first triangle from the point64_t
        int64_t t = add_triangle(m_hull[e].i, i, m_hull[m_hull[e].next].i, -1, -1, m_hull[e].t);

        m_hull[e].t = t; // keep track of boundary triangles on the hull
        e = insert_node(i, e, coords);

        // recursively flip triangles from the point64_t until they satisfy the Delaunay condition
        m_hull[e].t = legalize(t + 2, coords);
        if(m_hull[m_hull[m_hull[e].prev].prev].t == halfedges[t + 1])
        {
            m_hull[m_hull[m_hull[e].prev].prev].t = t + 2;
        }
        // walk forward through the hull, adding more triangles and flipping recursively
        int64_t q = m_hull[e].next;
        while(area(x,
                   y,
                   m_hull[q].x,
                   m_hull[q].y,
                   m_hull[m_hull[q].next].x,
                   m_hull[m_hull[q].next].y) < 0)
        {
            t = add_triangle(m_hull[q].i,
                             i,
                             m_hull[m_hull[q].next].i,
                             m_hull[m_hull[q].prev].t,
                             -1,
                             m_hull[q].t);
            m_hull[m_hull[q].prev].t = legalize(t + 2, coords);
            remove_node(q);
            q = m_hull[q].next;

            assert(q >= 0 && q < m_hull.size());
        }
        if(walk_back)
        {
            // walk backward from the other side, adding more triangles and flipping
            q = m_hull[e].prev;

            assert(q >= 0 && q < m_hull.size());

            while(area(x,
                       y,
                       m_hull[m_hull[q].prev].x,
                       m_hull[m_hull[q].prev].y,
                       m_hull[q].x,
                       m_hull[q].y) < 0)
            {
                t = add_triangle(m_hull[m_hull[q].prev].i,
                                 i,
                                 m_hull[q].i,
                                 -1,
                                 m_hull[q].t,
                                 m_hull[m_hull[q].prev].t);
                legalize(t + 2, coords);
                m_hull[m_hull[q].prev].t = t;
                remove_node(q);
                q = m_hull[q].prev;
                assert(q >= 0 && q < m_hull.size());
            }
        }
        hash_edge(e);
        hash_edge(m_hull[e].prev);
    }

    return true;
};

int64_t Delaunator::remove_node(int64_t node)
{
    m_hull[m_hull[node].prev].next = m_hull[node].next;
    m_hull[m_hull[node].next].prev = m_hull[node].prev;
    m_hull[node].removed = true;
    return m_hull[node].prev;
}

int64_t Delaunator::legalize(int64_t a, const vector<double>& coords)
{
    // int64_t halfedges_size = halfedges.size();
    const int64_t b = halfedges[a];

    const int64_t a0 = a - a % 3;
    const int64_t b0 = b - b % 3;

    const int64_t al = a0 + (a + 1) % 3;
    const int64_t ar = a0 + (a + 2) % 3;
    const int64_t bl = b0 + (b + 2) % 3;

    const int64_t p0 = triangles[ar];
    const int64_t pr = triangles[a];
    const int64_t pl = triangles[al];
    const int64_t p1 = triangles[bl];

    const bool illegal = in_circle(coords[2 * p0],
                                   coords[2 * p0 + 1],
                                   coords[2 * pr],
                                   coords[2 * pr + 1],
                                   coords[2 * pl],
                                   coords[2 * pl + 1],
                                   coords[2 * p1],
                                   coords[2 * p1 + 1]);

    if(illegal)
    {
        triangles[a] = p1;
        triangles[b] = p0;
        link(a, halfedges[bl]);
        link(b, halfedges[ar]);
        link(ar, bl);

        const int64_t br = b0 + (b + 1) % 3;

        legalize(a, coords);
        return legalize(br, coords);
    }
    return ar;
}

int64_t Delaunator::insert_node(int64_t i, int64_t prev, const vector<double>& coords)
{
    const int64_t node = insert_node(i, coords);
    m_hull[node].next = m_hull[prev].next;
    m_hull[node].prev = prev;
    m_hull[m_hull[node].next].prev = node;
    m_hull[prev].next = node;
    return node;
};

int64_t Delaunator::insert_node(int64_t i, const vector<double>& coords)
{
    int64_t node = m_hull.size();
    DelaunatorPoint p;
    p.i = i;
    p.x = coords[2 * i];
    p.y = coords[2 * i + 1];
    p.prev = node;
    p.next = node;
    p.removed = false;
    m_hull.push_back(move(p));
    return node;
}

int64_t Delaunator::hash_key(double x, double y)
{
    const double dx = x - m_center_x;
    const double dy = y - m_center_y;
    // use pseudo-angle: a measure that monotonically increases
    // with real angle, but doesn't require expensive trigonometry

    double den = std::abs(dx) + std::abs(dy);
    double p = 0;

    //potential division by zero!
    //previously: const double p = 1 - dx / (std::abs(dx) + std::abs(dy));
    if(den != 0)
    {
        p = 1 - dx / den;
    }

    double nom = (2 + (dy < 0 ? -p : p));
    int64_t key = std::floor((m_hash_size - 1) *
                             (nom / 4.0)); // this will behave differently from js version!
    //int64_t     key =  std::floor(  (m_hash_size) * (nom / 4.0) ); // this will behave differently from js version!

    return key;
}

void Delaunator::hash_edge(int64_t e)
{
    m_hash[hash_key(m_hull[e].x, m_hull[e].y)] = e;
}

int64_t Delaunator::add_triangle(
    int64_t i0, int64_t i1, int64_t i2, int64_t a, int64_t b, int64_t c)
{
    const int64_t t = triangles.size();
    triangles.push_back(i0);
    triangles.push_back(i1);
    triangles.push_back(i2);
    link(t, a);
    link(t + 1, b);
    link(t + 2, c);
    return t;
}

void Delaunator::link(int64_t a, int64_t b)
{
    int64_t s = halfedges.size();
    if(a == s)
    {
        halfedges.push_back(b);
    }
    else if(a < s)
    {
        halfedges[a] = b;
    }
    else
    {
        throw runtime_error("Cannot link edge");
    }
    if(b != -1)
    {
        int64_t s = halfedges.size();
        if(b == s)
        {
            halfedges.push_back(a);
        }
        else if(b < s)
        {
            halfedges[b] = a;
        }
        else
        {
            throw runtime_error("Cannot link edge");
        }
    }
};
} // namespace delaunator_cpp
