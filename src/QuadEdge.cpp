#include "tntn/QuadEdge.h"
#include "tntn/DelaunayTriangle.h"
#include "tntn/tntn_assert.h"

namespace tntn {
namespace terra {

#if 0
QuadEdge::QuadEdge(qe_ptr prev)
{
    qprev = prev;
    prev->qnext = this;
}

QuadEdge::QuadEdge()
{
    QuadEdge *e0 = this;
    QuadEdge *e1 = new QuadEdge(e0);
    QuadEdge *e2 = new QuadEdge(e1);
    QuadEdge *e3 = new QuadEdge(e2);
    
    qprev = e3;
    e3->qnext = e0;
    
    e0->next = e0;
    e1->next = e3;
    e2->next = e2;
    e3->next = e1;
}
#endif

//init with dual edge qprev aka invRot
void QuadEdge::init(qe_ptr self_ptr, qe_ptr qprev)
{
    TNTN_ASSERT(self_ptr.get() == this);
    this->self_ptr = self_ptr;

    this->qprev = qprev;
    qprev->qnext = self_ptr;
}

/*

 creates 3 more QuadEdges

 this=e0
 e1,e2,e3


 they are linked together like this:

          |        ^ e0
          |  _---->|
 e1       | /qprev |
 <--------+--------+----------
     ^    |        |
      \___|        |
    qprev |        |___
          |        |   \ qprev
          |        |    V
 ---------+--------+--------->
          | qprev/ |         e3
          |<-___/  |
       e2 V        |
 */
void QuadEdge::init(qe_ptr self_ptr)
{
    TNTN_ASSERT(self_ptr.get() == this);
    this->self_ptr = self_ptr;

    qe_ptr e0 = self_ptr;

    qe_ptr e1 = self_ptr.pool()->spawn();
    qe_ptr e2 = self_ptr.pool()->spawn();
    qe_ptr e3 = self_ptr.pool()->spawn();

    e1->init(e1, e0);
    e2->init(e2, e1);
    e3->init(e3, e2);

    e0->qprev = e3;
    e3->qnext = e0;

    e0->next = e0;
    e1->next = e3;
    e2->next = e2;
    e3->next = e1;
}

#if 0
QuadEdge::~QuadEdge()
{
    if (qnext) {
        QuadEdge *e1 = qnext;
        QuadEdge *e2 = qnext->qnext;
        QuadEdge *e3 = qprev;
        
        e1->qnext = NULL;
        e2->qnext = NULL;
        e3->qnext = NULL;
        
        delete e1;
        delete e2;
        delete e3;
    }
}
#endif

void QuadEdge::recycle_next()
{
    if(qnext)
    {
        qe_ptr e1 = qnext;
        qe_ptr e2 = qnext->qnext;
        qe_ptr e3 = qprev;

        e1->qnext.clear();
        e2->qnext.clear();
        e3->qnext.clear();

        //TODO this a recursive graph traversal on the stack... make this iterative
        e1->recycle_next();
        e1.recycle();
        e2->recycle_next();
        e2.recycle();
        e3->recycle_next();
        e3.recycle();
    }
}

void splice(qe_ptr a, qe_ptr b)
{
    qe_ptr alpha = a->Onext()->Rot();
    qe_ptr beta = b->Onext()->Rot();

    qe_ptr t1 = b->Onext();
    qe_ptr t2 = a->Onext();
    qe_ptr t3 = beta->Onext();
    qe_ptr t4 = alpha->Onext();

    a->next = t1;
    b->next = t2;
    alpha->next = t3;
    beta->next = t4;
}

} //namespace terra
} //namespace tntn
