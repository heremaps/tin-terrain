#pragma once

#include "tntn/QuadEdge.h"
#include "tntn/ObjPool.h"
#include "tntn/tntn_assert.h"

namespace tntn {
namespace terra {

class DelaunayTriangle;
typedef pool_ptr<DelaunayTriangle> dt_ptr;

class DelaunayTriangle
{
    qe_ptr anchor;
    dt_ptr next_face;
    dt_ptr self_ptr;

  public:
    DelaunayTriangle() {}

    void init(dt_ptr self_ptr, qe_ptr e)
    {
        TNTN_ASSERT(self_ptr.get() == this);
        this->self_ptr = self_ptr;
        reshape(e);
    }

    dt_ptr linkTo(dt_ptr t)
    {
        next_face = t;
        return self_ptr;
    }

    dt_ptr getLink() { return next_face; }

    qe_ptr getAnchor() { return anchor; }

    void dontAnchor(qe_ptr e);

    void reshape(qe_ptr e);

    Point2D point1() const { return anchor->Org(); }
    Point2D point2() const { return anchor->Dest(); }
    Point2D point3() const { return anchor->Lprev()->Org(); }
};

} //namespace terra
} //namespace tntn
