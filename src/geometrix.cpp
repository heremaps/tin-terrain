#include "tntn/geometrix.h"

#include "glm/glm.hpp"
#include "glm/gtx/normal.hpp"

#include <algorithm>

namespace tntn {

static bool epseq(const double a, double b, double epsilon)
{
    return fabs(a - b) < epsilon;
}

bool triangle_semantic_equal::operator()(const Triangle& l, const Triangle& _r) const
{
    Triangle r = _r;
#if 1
    if(l == r)
    {
        return true;
    }
    std::rotate(r.begin(), r.begin() + 1, r.end());
    if(l == r)
    {
        return true;
    }
    std::rotate(r.begin(), r.begin() + 1, r.end());
    if(l == r)
    {
        return true;
    }
    return false;
#else
    //compare winding
    const glm::dvec3 ln = glm::triangleNormal(l[0], l[1], l[2]);
    const glm::dvec3 rn = glm::triangleNormal(r[0], r[1], r[2]);
    if((ln.x > 0 && rn.x <= 0) || (ln.y > 0 && rn.y <= 0) || (ln.z > 0 && rn.z <= 0))
    {
        return false;
    }

    //compare point sets
    for(const auto& p : l)
    {
        bool found = false;
        for(int ri = 0; ri < 3; ri++)
        {
            if(r[ri] == p)
            {
                r[ri].x = NAN;
                r[ri].y = NAN;
                r[ri].z = NAN;
                found = true;
                break;
            }
        }

        if(!found)
        {
            return false;
        }
    }

    return true;
#endif
}

static inline bool is_facing_upwards_impl(const double t0_x,
                                          const double t0_y,
                                          const double t1_x,
                                          const double t1_y,
                                          const double t2_x,
                                          const double t2_y)
{
    //const glm::dvec3 n = glm::cross(t[0] - t[1], t[0] - t[2]);
    const double n_z = (t0_x - t1_x) * (t0_y - t2_y) - (t0_x - t2_x) * (t0_y - t1_y);
    return n_z >= 0;
}

bool is_facing_upwards(const Triangle& t)
{
    const double t0_x = t[0].x;
    const double t0_y = t[0].y;

    const double t1_x = t[1].x;
    const double t1_y = t[1].y;

    const double t2_x = t[2].x;
    const double t2_y = t[2].y;

    return is_facing_upwards_impl(t0_x, t0_y, t1_x, t1_y, t2_x, t2_y);
}

bool is_facing_upwards(const Face& f, const std::vector<Vertex>& vertices)
{
    const double t0_x = vertices[f[0]].x;
    const double t0_y = vertices[f[0]].y;

    const double t1_x = vertices[f[1]].x;
    const double t1_y = vertices[f[1]].y;

    const double t2_x = vertices[f[2]].x;
    const double t2_y = vertices[f[2]].y;

    return is_facing_upwards_impl(t0_x, t0_y, t1_x, t1_y, t2_x, t2_y);
}

static glm::dvec2 intersect_2D_linesegment_by_line(const glm::dvec2& p0,
                                                   const glm::dvec2& p1,
                                                   const glm::dvec2& l0,
                                                   const glm::dvec2& l1)
{
    //from https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection

    const double x_1 = p0.x;
    const double x_2 = p1.x;
    const double x_3 = l0.x;
    const double x_4 = l1.x;

    const double y_1 = p0.y;
    const double y_2 = p1.y;
    const double y_3 = l0.y;
    const double y_4 = l1.y;

    const double denom = (x_1 - x_2) * (y_3 - y_4) - (y_1 - y_2) * (x_3 - x_4);
    const double eps = 0.000000001;
    if(glm::abs(denom) < eps)
    {
        return glm::dvec2(NAN, NAN);
    }

    const double c_x =
        ((x_1 * y_2 - y_1 * x_2) * (x_3 - x_4) - (x_1 - x_2) * (x_3 * y_4 - y_3 * x_4)) / denom;
    const double c_y =
        ((x_1 * y_2 - y_1 * x_2) * (y_3 - y_4) - (y_1 - y_2) * (x_3 * y_4 - y_3 * x_4)) / denom;

    return glm::dvec2(c_x == -0.0 ? 0.0 : c_x, c_y == -0.0 ? 0.0 : c_y);
}

bool Edge::intersects2D(const Edge& other, const std::vector<Vertex>& vertices) const
{
    const glm::dvec2 p0 = vertices[this->first];
    const glm::dvec2 p1 = vertices[this->second];

    const glm::dvec2 l0 = vertices[other.first];
    const glm::dvec2 l1 = vertices[other.second];

    const BBox2D e1_bbox(p0, p1);
    const BBox2D e2_bbox(l0, l1);

    if(!e1_bbox.intersects(e2_bbox))
    {
        return false;
    }

    const glm::dvec2 intersection_point = intersect_2D_linesegment_by_line(p0, p1, l0, l1);
    if(std::isnan(intersection_point.x) || std::isnan(intersection_point.y))
    {
        return false;
    }

    return e1_bbox.contains(intersection_point) && e2_bbox.contains(intersection_point);
}

BBox2D::BBox2D() :
    min(std::numeric_limits<double>::infinity()),
    max(-std::numeric_limits<double>::infinity())
{
}

BBox2D::BBox2D(glm::dvec2 a, glm::dvec2 b) :
    min(std::min(a.x, b.x), std::min(a.y, b.y)),
    max(std::max(a.x, b.x), std::max(a.y, b.y))
{
}

BBox2D::BBox2D(glm::vec2 a, glm::vec2 b) : BBox2D(glm::dvec2(a.x, a.y), glm::dvec2(b.x, b.y)) {}

BBox2D::BBox2D(glm::vec3 a, glm::vec3 b) : BBox2D(glm::dvec2(a.x, a.y), glm::dvec2(b.x, b.y)) {}

BBox2D::BBox2D(glm::dvec3 a, glm::dvec3 b) : BBox2D(a.xy(), b.xy()) {}

BBox2D::BBox2D(const Triangle& t) : BBox2D(t[0].xy(), t[1].xy())
{
    add(t[2].xy());
}

void BBox2D::reset()
{
    min = glm::dvec2(std::numeric_limits<double>::infinity());
    max = glm::dvec2(-std::numeric_limits<double>::infinity());
}

void BBox2D::add(glm::vec2 p)
{
    add(glm::dvec2(p.x, p.y));
}

void BBox2D::add(glm::dvec2 p)
{
    min.x = std::min(min.x, p.x);
    min.y = std::min(min.y, p.y);
    max.x = std::max(max.x, p.x);
    max.y = std::max(max.y, p.y);
}

void BBox2D::add(glm::vec3 p)
{
    add(p.xy());
}

void BBox2D::add(glm::dvec3 p)
{
    add(p.xy());
}

void BBox2D::add(const Triangle& t)
{
    for(int i = 0; i < 3; i++)
    {
        add(t[i].xy());
    }
}

void BBox2D::grow(const double delta)
{
    min.x -= delta;
    min.y -= delta;
    max.x += delta;
    max.y += delta;
}

bool BBox2D::intersects(const BBox2D& other, const double epsilon) const
{
    //epsilon grows both rectangles

    //https://stackoverflow.com/questions/306316/determine-if-two-rectangles-overlap-each-other

    //is this above other?
    if(min.y - epsilon > other.max.y + epsilon) return false;

    //is this below other?
    if(max.y + epsilon < other.min.y - epsilon) return false;

    //is this left of other?
    if(max.x + epsilon < other.min.x - epsilon) return false;

    //is this right of other?
    if(min.x - epsilon > other.max.x + epsilon) return false;

    //otherwise they must intersect
    return true;
}

bool BBox2D::contains(const glm::dvec2 point, const double epsilon) const
{
    return (min.x - epsilon) <= point.x && (min.y - epsilon) <= point.y &&
        (max.x + epsilon) >= point.x && (max.y + epsilon) >= point.y;
}

bool BBox2D::is_equal(const BBox2D& bb) const
{
    if(bb.min.x != min.x) return false;
    if(bb.min.y != min.y) return false;
    if(bb.max.x != max.x) return false;
    if(bb.max.y != max.y) return false;
    return true;
}

bool BBox2D::is_on_border(const glm::dvec2 point, const double epsilon) const
{
    return epseq(point.x, min.x, epsilon) || epseq(point.x, max.x, epsilon) ||
        epseq(point.y, min.y, epsilon) || epseq(point.y, max.y, epsilon);
}

std::string BBox2D::to_string() const
{
    std::string out;
    constexpr int double_str_len = 16;
    out.reserve(4 * 2 + 3 + 4 * double_str_len + 1 /*to make it a even*/);
    out = "[("; //2
    out += std::to_string(min.x); //double
    out += ", "; //2
    out += std::to_string(min.y); //double
    out += "),("; //3
    out += std::to_string(max.x); //double
    out += ", "; //2
    out += std::to_string(max.y); //double
    out += "])"; //2
    return out;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

BBox3D::BBox3D() :
    min(std::numeric_limits<double>::infinity()),
    max(-std::numeric_limits<double>::infinity())
{
}

BBox3D::BBox3D(glm::vec3 a, glm::vec3 b) :
    BBox3D(glm::dvec3(a.x, a.y, a.z), glm::dvec3(b.x, b.y, b.z))
{
}

BBox3D::BBox3D(glm::dvec3 a, glm::dvec3 b) :
    min(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)),
    max(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z))
{
}

BBox3D::BBox3D(const Triangle& t) : BBox3D(t[0], t[1])
{
    add(t[2]);
}

void BBox3D::reset()
{
    min = glm::dvec3(std::numeric_limits<double>::infinity());
    max = glm::dvec3(-std::numeric_limits<double>::infinity());
}

void BBox3D::add(glm::dvec3 p)
{
    min.x = std::min(min.x, p.x);
    min.y = std::min(min.y, p.y);
    min.z = std::min(min.z, p.z);

    max.x = std::max(max.x, p.x);
    max.y = std::max(max.y, p.y);
    max.z = std::max(max.z, p.z);
}

void BBox3D::add(glm::vec3 p)
{
    add(glm::dvec3(p.x, p.y, p.z));
}

void BBox3D::add(const Triangle& t)
{
    for(int i = 0; i < 3; i++)
    {
        add(t[i]);
    }
}

void BBox3D::grow(const double delta)
{
    min.x -= delta;
    min.y -= delta;
    min.z -= delta;

    max.x += delta;
    max.y += delta;
    max.z += delta;
}

bool BBox3D::contains(const glm::dvec3 point, const double epsilon) const
{
    return (min.x - epsilon) <= point.x && (min.y - epsilon) <= point.y &&
        (min.z - epsilon) <= point.z &&

        (max.x + epsilon) >= point.x && (max.y + epsilon) >= point.y &&
        (max.z + epsilon) >= point.z;
}

std::string BBox3D::to_string() const
{
    std::string out;
    constexpr int double_str_len = 16;
    out.reserve(6 * 2 + 3 + 6 * double_str_len + 1 /*to make it a even*/);
    out = "[("; //2
    out += std::to_string(min.x); //double
    out += ", "; //2
    out += std::to_string(min.y); //double
    out += ", "; //2
    out += std::to_string(min.z); //double
    out += "),("; //3
    out += std::to_string(max.x); //double
    out += ", "; //2
    out += std::to_string(max.y); //double
    out += ", "; //2
    out += std::to_string(max.z); //double
    out += "])"; //2
    return out;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static glm::dvec3 abs_zero(const glm::dvec3& v)
{
    return glm::dvec3(v.x == -0.0 ? 0.0 : v.x, v.y == -0.0 ? 0.0 : v.y, v.z == -0.0 ? 0.0 : v.z);
}

static double squared_3D_distance(const glm::dvec3& p0, const glm::dvec3& p1)
{
    const double dx = p1.x - p0.x;
    const double dy = p1.y - p0.y;
    const double dz = p1.z - p0.z;
    return dx * dx + dy * dy + dz * dz;
}
static double squared_2D_distance(const glm::dvec2& p0, const glm::dvec2& p1)
{
    const double dx = p1.x - p0.x;
    const double dy = p1.y - p0.y;
    return dx * dx + dy * dy;
}

/**

 returns the intersection point of the 2.5D line segment p0-p1 (2D line with height in z) with the 2D line l_origin -> l_direction

 returns NaN if lines are (close to) parallel or intersection is outside the line segment p0-p1

 */
glm::dvec3 intersect_25D_linesegment_by_line(glm::dvec3 p0,
                                             glm::dvec3 p1,
                                             glm::dvec2 l_org,
                                             glm::dvec2 l_dir)
{
    //from https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection

    const double x_1 = p0.x;
    const double x_2 = p1.x;
    const double x_3 = l_org.x;
    const double x_4 = l_org.x + l_dir.x;

    const double y_1 = p0.y;
    const double y_2 = p1.y;
    const double y_3 = l_org.y;
    const double y_4 = l_org.y + l_dir.y;

    const double denom = (x_1 - x_2) * (y_3 - y_4) - (y_1 - y_2) * (x_3 - x_4);
    const double eps = 0.000000001;
    if(glm::abs(denom) < eps)
    {
        return glm::dvec3(NAN, NAN, NAN);
    }

    glm::dvec3 c;
    c.x = ((x_1 * y_2 - y_1 * x_2) * (x_3 - x_4) - (x_1 - x_2) * (x_3 * y_4 - y_3 * x_4)) / denom;
    c.y = ((x_1 * y_2 - y_1 * x_2) * (y_3 - y_4) - (y_1 - y_2) * (x_3 * y_4 - y_3 * x_4)) / denom;

    //interpolate z

    //z = mx + n
    const double d_p0p1 = glm::distance(p0.xy(), p1.xy());
    const double m = (p1.z - p0.z) / d_p0p1;
    const double n = p0.z;

    const double d_p0c = glm::distance(p0.xy(), c.xy());
    if(d_p0c < 0.0 - eps || d_p0c > (d_p0p1 + eps))
    {
        return glm::dvec3(NAN, NAN, NAN);
    }

    c.z = m * d_p0c + n;
    return abs_zero(c);
}

int sign_2D(const glm::dvec3& p, const glm::dvec2& l_org, const glm::dvec2& l_dir)
{
    //specialized versions using comparison
    if(l_dir.x == 0.0)
    {
        //left-right clipping
        const int direction_sign = l_dir.y > 0.0 ? -1 /* upwards */ : 1 /* downwards */;
        if(p.x < l_org.x)
        {
            return direction_sign;
        }
        else if(p.x > l_org.x)
        {
            return -direction_sign;
        }
        else
        {
            return 0;
        }
    }
    else if(l_dir.y == 0.0)
    {
        //top-bottom clipping
        const int direction_sign = l_dir.x > 0 ? -1 /* right-wards */ : 1 /* left-wards */;
        if(p.y < l_org.y)
        {
            return -direction_sign;
        }
        else if(p.y > l_org.y)
        {
            return direction_sign;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        //generic version using cross product
        const double d = (p.x - l_org.x) * (l_dir.y) - (p.y - l_org.y) * (l_dir.x);
        const double eps = 0.000000001;
        if(d < eps)
        {
            return -1;
        }
        else if(d > eps)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}

int compare_length(const glm::dvec3& a1,
                   const glm::dvec3& a2,
                   const glm::dvec3& b1,
                   const glm::dvec3& b2)
{
    const double da_sq = squared_3D_distance(a1, a2);
    const double db_sq = squared_3D_distance(b1, b2);

    if(da_sq < db_sq)
    {
        return -1;
    }
    else if(da_sq == db_sq)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

int compare_length(const glm::dvec2& a1,
                   const glm::dvec2& a2,
                   const glm::dvec2& b1,
                   const glm::dvec2& b2)
{
    const double da_sq = squared_2D_distance(a1, a2);
    const double db_sq = squared_2D_distance(b1, b2);

    if(da_sq < db_sq)
    {
        return -1;
    }
    else if(da_sq == db_sq)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

static bool has_NaNs(const Triangle& t)
{
#if 1
    int num_nans = 0;
    for(int i = 0; i < 3; i++)
    {
        for(int k = 0; k < 3; k++)
        {
            num_nans += std::isnan(t[i][k]);
        }
    }
    return num_nans != 0;
#else
    for(int i = 0; i < 3; i++)
    {
        if(std::isnan(t[i].x) || std::isnan(t[i].y) || std::isnan(t[i].z))
        {
            return true;
        }
    }
    return false;
#endif
}

static inline bool is_front_facing(const Triangle& t)
{
#if 0
    //const glm::dvec3 n = glm::triangleNormal(t[0], t[1], t[2]);
    const glm::dvec3 n = glm::cross(t[0] - t[1], t[0] - t[2]);
    return n.z >= 0;
#else
    const double t0_x = t[0].x;
    const double t0_y = t[0].y;
    const double n_z = (t0_x - t[1].x) * (t0_y - t[2].y) - (t0_x - t[2].x) * (t0_y - t[1].y);
    return n_z >= 0;
#endif
}

void make_front_facing(Triangle& t)
{
    if(!is_front_facing(t))
    {
        std::swap(t[0], t[1]);
    }
}

void clip_25D_triangle_by_line(std::vector<Triangle>& tv,
                               size_t triangle_idx,
                               glm::dvec2 l_org,
                               glm::dvec2 l_dir)
{
    Triangle& t = tv[triangle_idx];
    //ignore NAN triangles:
    if(has_NaNs(t))
    {
        return;
    }

    /*

     lp - left_points
     op - other_points

     winding order counter-clockwise = inside
                     +
                    /|
                   / |
                  /  |
                 /   |
                /    |
               /     |
              /      |
             /       |
            /        |
           /         |
   l1   s0/          |s1     l0
    x----*-----------*-------x
        /            |
       /             |
lp[0] +              |
       \             |
         \           |
           \         |
             \       |
               \     |
                 \   |
                   \ |
                     + lp[1]

    */

    //calculate the points which are right of the clip-line
    std::array<glm::dvec3, 3> left_points;
    std::array<glm::dvec3, 3> other_points;
    std::array<int, 3> other_signs;
    int num_left_points = 0;
    int num_other_points = 0;

    for(const auto& point : t)
    {
        const int d = sign_2D(point, l_org, l_dir);
        if(d < 0)
        {
            left_points[num_left_points] = point;
            num_left_points++;
        }
        else
        {
            other_points[num_other_points] = point;
            other_signs[num_other_points] = d;
            num_other_points++;
        }
    }

    if(num_left_points == 0)
    {
        //mark triangle for later removal (make it invalid)
        t[0] = glm::dvec3(NAN, NAN, NAN);
        //t[1] = glm::dvec3(NAN, NAN, NAN);
        //t[2] = glm::dvec3(NAN, NAN, NAN);
    }
    else if(num_left_points == 1)
    {
        assert(num_other_points == 2);
        //intersect adjacent edges with line
        //  use original point when the point is exactly on the clib-line
        const glm::dvec3 s0 = other_signs[0] == 0
            ? other_points[0]
            : intersect_25D_linesegment_by_line(left_points[0], other_points[0], l_org, l_dir);
        const glm::dvec3 s1 = other_signs[1] == 0
            ? other_points[1]
            : intersect_25D_linesegment_by_line(left_points[0], other_points[1], l_org, l_dir);

        //replace triangle t with new triangle
        t[0] = left_points[0];
        t[1] = s0;
        t[2] = s1;
        make_front_facing(t);
    }
    else if(num_left_points == 2)
    {
        assert(num_other_points == 1);
        if(other_signs[0] == 0)
        {
            //other point is exactly on clib-line, keep original triangle
            return;
        }

        const glm::dvec3 s0 =
            intersect_25D_linesegment_by_line(other_points[0], left_points[0], l_org, l_dir);
        const glm::dvec3 s1 =
            intersect_25D_linesegment_by_line(other_points[0], left_points[1], l_org, l_dir);

        //determine the shorter of the two possible new edges
        //const double d0 = glm::distance(s0, left_points[1]);
        //const double d1 = glm::distance(s1, left_points[0]);
        const int d0_d1_cmp = compare_length(s0, left_points[1], s1, left_points[0]);

        //replace triangle t with new triangle
        // and construct a new second new triangle
        // the new edge is selected to be the shorter of the two possible ones

        t[0] = d0_d1_cmp >= 0 ? s1 : s0;
        t[1] = left_points[0];
        t[2] = left_points[1];

        Triangle t_new = {{s1, s0, d0_d1_cmp >= 0 ? left_points[0] : left_points[1]}};

        make_front_facing(t);
        make_front_facing(t_new);
        tv.push_back(t_new);
    }

    return;
}

void clip_25D_triangles_to_01_quadrant(std::vector<Triangle>& tv)
{

    /*
       (0,1)     (1,1)
            +---+
            |   |       //winding order: counter-clock-wise = inside
            +---+
       (0,0)     (1,0)
     */

    size_t tv_size = tv.size();
    for(size_t i = 0; i < tv_size; i++)
    {
        //clip at bottom edge - right-wards line
        clip_25D_triangle_by_line(tv, i, {0, 0}, {1, 0});
    }

    tv_size = tv.size();
    for(size_t i = 0; i < tv_size; i++)
    {
        //clip at right edge - upwards line
        clip_25D_triangle_by_line(tv, i, {1, 0}, {0, 1});
    }

    tv_size = tv.size();
    for(size_t i = 0; i < tv_size; i++)
    {
        //clip at top edge - leftwards line
        clip_25D_triangle_by_line(tv, i, {1, 1}, {-1, 0});
    }

    tv_size = tv.size();
    for(size_t i = 0; i < tv_size; i++)
    {
        //clip at left edge - downwards line
        clip_25D_triangle_by_line(tv, i, {0, 1}, {0, -1});
    }

    //filter out NaN triangles
    tv.erase(std::remove_if(tv.begin(), tv.end(), [](const Triangle& t) { return has_NaNs(t); }),
             tv.end());
}

} //namespace tntn
