#pragma once

#include "tntn/geometrix.h"
#include "tntn/ObjPool.h"

#define EPS 1e-6

namespace tntn {
namespace terra {

typedef glm::dvec2 Point2D;

// triArea returns TWICE the area of the oriented triangle ABC.
// The area is positive when ABC is oriented counterclockwise.
inline double triArea(const Point2D& a, const Point2D& b, const Point2D& c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

inline bool ccw(const Point2D& a, const Point2D& b, const Point2D& c)
{
    return triArea(a, b, c) > 0;
}

inline bool rightOf(const Point2D& x, const Point2D& org, const Point2D& dest)
{
    return ccw(x, dest, org);
}

inline bool leftOf(const Point2D& x, const Point2D& org, const Point2D& dest)
{
    return ccw(x, org, dest);
}

// Returns True if the point d is inside the circle defined by the
// points a, b, c. See Guibas and Stolfi (1985) p.107.
inline bool inCircle(const Point2D& a, const Point2D& b, const Point2D& c, const Point2D& d)
{
    return (a[0] * a[0] + a[1] * a[1]) * triArea(b, c, d) -
        (b[0] * b[0] + b[1] * b[1]) * triArea(a, c, d) +
        (c[0] * c[0] + c[1] * c[1]) * triArea(a, b, d) -
        (d[0] * d[0] + d[1] * d[1]) * triArea(a, b, c) >
        EPS;
}

class Line
{
  private:
    double a;
    double b;
    double c;

  public:
    Line(const glm::dvec2& p, const glm::dvec2& q)
    {
        const glm::dvec2 t = q - p;
        double l = t.length();

        a = t.y / l;
        b = -t.x / l;
        c = -(a * p.x + b * p.y);
    }

    inline double eval(const glm::dvec2& p) const noexcept { return (a * p.x + b * p.y + c); }
};

class Plane
{
  public:
    double a;
    double b;
    double c;

    Plane() = default;
    Plane(const glm::dvec3& p, const glm::dvec3& q, const glm::dvec3& r) { init(p, q, r); }

    inline void init(const glm::dvec3& p, const glm::dvec3& q, const glm::dvec3& r) noexcept;

    double eval(double x, double y) const noexcept { return a * x + b * y + c; }
    double eval(int x, int y) const noexcept { return a * x + b * y + c; }
};

// find the plane z=ax+by+c passing through three points p,q,r
inline void Plane::init(const glm::dvec3& p, const glm::dvec3& q, const glm::dvec3& r) noexcept
{
    // We explicitly declare these (rather than putting them in a
    // Vector) so that they can be allocated into registers.
    const double ux = q.x - p.x;
    const double uy = q.y - p.y;
    const double uz = q.z - p.z;

    const double vx = r.x - p.x;
    const double vy = r.y - p.y;
    const double vz = r.z - p.z;

    const double den = ux * vy - uy * vx;

    const double _a = (uz * vy - uy * vz) / den;
    const double _b = (ux * vz - uz * vx) / den;

    a = _a;
    b = _b;
    c = p.z - _a * p.x - _b * p.y;
}

class DelaunayTriangle;
typedef pool_ptr<DelaunayTriangle> dt_ptr;

class QuadEdge;
typedef pool_ptr<QuadEdge> qe_ptr;

//see http://www.cs.cmu.edu/afs/andrew/scs/cs/15-463/2001/pub/src/a2/quadedge.html
/*

      \_                 _/
        \_             _/
          \_    |    _/
            \_  |  _/
              \_*_/
                |e->Dest
                |
                ^
       *        |e          *
    e->Left     |         e->Right
                |e->Org
               _*_
             _/ | \_
           _/   |   \_
         _/     |     \_
       _/               \_
      /                   \




            next=ccw
      \_                 _/
        \_             _/
          \_    |    _/
    <Lnext  \_  |  _/   <Dnext
              \_|_/
                | Dest
                |        +--    <---+
                ^        | next=ccw |
                |e       +----------+
                |
                |
               _|_Org
             _/ | \_
    <Onext _/   |   \_ <Rnext
         _/     |     \_
       _/               \_
      /                   \





            prev=cw
      \_                 _/
        \_             _/
          \_    |    _/
    >Dprev  \_  |  _/  >Rprev
              \_|_/
                | Dest
                |
                ^        +--->   --+
                |e       | prev=cw |
                |        +---------+
                |
               _|_Org
             _/ | \_
    >Lprev _/   |   \_ >Oprev
         _/     |     \_
       _/               \_
      /                   \



                *
                |
                v e->Sym
    e->InvRot   |
    *----->-----|-------<-----*
                |       e->Rot
              e ^
                |
                *


 */

class QuadEdge
{
  private:
    qe_ptr qnext; //aka Rot
    qe_ptr qprev; //aka invRot
    qe_ptr self_ptr;

    QuadEdge(qe_ptr prev);

  protected:
    Point2D data;
    qe_ptr next; //aka Onext
    dt_ptr lface;

  public:
    QuadEdge() = default;

    void init(qe_ptr self_ptr);
    void init(qe_ptr self_ptr, qe_ptr qprev);
    void recycle_next();

    //"next" means next in a counterclockwise (ccw) sense around a neighboring face or vertex
    //"prev" means next in a clockwise (cw) sense around a neighboring face or vertex

    // Primitive methods
    //next edge around origin, with same origin
    qe_ptr Onext() const noexcept { return next; }

    //edge pointing opposite to this
    qe_ptr Sym() const { return qnext->qnext; }

    //dual edge pointing to the left of this
    qe_ptr Rot() const noexcept { return qnext; }
    //dual edge pointing to the right of this
    qe_ptr invRot() const noexcept { return qprev; }

    // Synthesized methods
    qe_ptr Oprev() const { return Rot()->Onext()->Rot(); }
    qe_ptr Dnext() const { return Sym()->Onext()->Sym(); }
    qe_ptr Dprev() const { return invRot()->Onext()->invRot(); }

    //next edge around left face, with same left face
    qe_ptr Lnext() const { return invRot()->Onext()->Rot(); }
    //prev edge around left face, with same left face
    qe_ptr Lprev() const { return Onext()->Sym(); }

    //next edge around right face, with same right face
    qe_ptr Rnext() const { return Rot()->Onext()->invRot(); }
    //prev edge around right face, with same right face
    qe_ptr Rprev() const { return Sym()->Onext(); }

    //origin point
    Point2D Org() const noexcept { return data; }
    //destination point
    Point2D Dest() const { return Sym()->data; }

    //face on the left
    dt_ptr Lface() const noexcept { return lface; }
    void set_Lface(dt_ptr t) noexcept { lface = t; }

    void set_end_points(const Point2D org, const Point2D dest)
    {
        data = org;
        Sym()->data = dest;
    }

    // The fundamental topological operator
    friend void splice(qe_ptr a, qe_ptr b);
};

inline bool rightOf(const Point2D& x, qe_ptr e)
{
    return rightOf(x, e->Org(), e->Dest());
}

inline bool leftOf(const Point2D& x, qe_ptr e)
{
    return leftOf(x, e->Org(), e->Dest());
}

} //namespace terra
} //namespace tntn
