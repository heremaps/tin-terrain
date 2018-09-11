#include "tntn/DelaunayTriangle.h"

namespace tntn {
namespace terra {

void DelaunayTriangle::dontAnchor(qe_ptr e)
{
    if(anchor == e)
    {
        anchor = e->Lnext();
    }
}

void DelaunayTriangle::reshape(qe_ptr e)
{
    anchor = e;
    e->set_Lface(self_ptr);
    e->Lnext()->set_Lface(self_ptr);
    e->Lprev()->set_Lface(self_ptr);
}

} //namespace terra
} //namespace tntn
