#include "tntn/DelaunayMesh.h"
#include "tntn/DelaunayTriangle.h"

namespace tntn {
namespace terra {

dt_ptr DelaunayMesh::make_face(qe_ptr e)
{
    dt_ptr t = m_triangles->spawn();
    t->init(t, e);

    m_first_face = t->linkTo(m_first_face);
    return t;
}

void DelaunayMesh::init_mesh(const Point2D a, const Point2D b, const Point2D c, const Point2D d)
{
    qe_ptr ea = m_edges->spawn();
    ea->init(ea);
    ea->set_end_points(a, b);

    qe_ptr eb = m_edges->spawn();
    eb->init(eb);
    splice(ea->Sym(), eb);
    eb->set_end_points(b, c);

    qe_ptr ec = m_edges->spawn();
    ec->init(ec);
    splice(eb->Sym(), ec);
    ec->set_end_points(c, d);

    qe_ptr ed = m_edges->spawn();
    ed->init(ed);
    splice(ec->Sym(), ed);
    ed->set_end_points(d, a);
    splice(ed->Sym(), ea);

    qe_ptr diag = m_edges->spawn();
    diag->init(diag);
    splice(ed->Sym(), diag);
    splice(eb->Sym(), diag->Sym());
    diag->set_end_points(a, c);

    m_starting_edge = ea;

    m_first_face.clear();

    make_face(ea->Sym());
    make_face(ec->Sym());
}

void DelaunayMesh::delete_edge(qe_ptr e)
{
    splice(e, e->Oprev());
    splice(e->Sym(), e->Sym()->Oprev());
    e->recycle_next();
    e.recycle();
}

qe_ptr DelaunayMesh::connect(qe_ptr a, qe_ptr b)
{
    qe_ptr e = m_edges->spawn();
    e->init(e);

    splice(e, a->Lnext());
    splice(e->Sym(), b);
    e->set_end_points(a->Dest(), b->Org());
    return e;
}

void DelaunayMesh::swap(qe_ptr e)
{
    dt_ptr f1 = e->Lface();
    dt_ptr f2 = e->Sym()->Lface();

    qe_ptr a = e->Oprev();
    qe_ptr b = e->Sym()->Oprev();

    splice(e, a);
    splice(e->Sym(), b);
    splice(e, a->Lnext());
    splice(e->Sym(), b->Lnext());
    e->set_end_points(a->Dest(), b->Dest());

    f1->reshape(e);
    f2->reshape(e->Sym());
}

//
// Helper functions
//

bool DelaunayMesh::ccw_boundary(qe_ptr e)
{
    return !rightOf(e->Oprev()->Dest(), e);
}

bool DelaunayMesh::on_edge(const Point2D x, qe_ptr e)
{
    double t1, t2, t3;

    t1 = (x - e->Org()).length();
    t2 = (x - e->Dest()).length();

    if(t1 < EPS || t2 < EPS) return true;

    t3 = (e->Org() - e->Dest()).length();

    if(t1 > t3 || t2 > t3) return false;

    Line line(e->Org(), e->Dest());
    return (fabs(line.eval(x)) < EPS);
}

// Tests whether e is an interior edge
bool DelaunayMesh::is_interior(qe_ptr e)
{
    return (e->Lnext()->Lnext()->Lnext() == e && e->Rnext()->Rnext()->Rnext() == e);
}

bool DelaunayMesh::should_swap(const Point2D x, qe_ptr e)
{
    qe_ptr t = e->Oprev();
    return inCircle(e->Org(), t->Dest(), e->Dest(), x);
}

void DelaunayMesh::scan_triangle(dt_ptr t)
{
    // noop
}

qe_ptr DelaunayMesh::locate(const Point2D x, qe_ptr start)
{
    qe_ptr e = start;
    double t = triArea(x, e->Dest(), e->Org());

    if(t > 0)
    { // x is to the right of edge e
        t = -t;
        e = e->Sym();
    }

    while(true)
    {
        qe_ptr eo = e->Onext();
        qe_ptr ed = e->Dprev();

        double to = triArea(x, eo->Dest(), eo->Org());
        double td = triArea(x, ed->Dest(), ed->Org());

        if(td > 0)
        { // x is below ed
            if(to > 0 || (to == 0 && t == 0))
            {
                // x is interior, or origin endpoint
                m_starting_edge = e;
                return e;
            }
            else
            {
                // x is below ed, below eo
                t = to;
                e = eo;
            }
        }
        else
        { // x is on or above ed
            if(to > 0)
            {
                // x is above eo
                if(td == 0 && t == 0)
                {
                    // x is destination endpoint
                    m_starting_edge = e;
                    return e;
                }
                else
                {
                    // x is on or above ed and above eo
                    t = td;
                    e = ed;
                }
            }
            else
            {
                // x is on or below eo
                if(t == 0 && !leftOf(eo->Dest(), e))
                {
                    // x on e but DelaunayMesh is to right
                    e = e->Sym();
                }
                else if((next_random_number() & 1) == 0)
                {
                    // x is on or above ed and on or below eo; step randomly
                    t = to;
                    e = eo;
                }
                else
                {
                    t = td;
                    e = ed;
                }
            }
        }
    }
}

qe_ptr DelaunayMesh::spoke(const Point2D x, qe_ptr e)
{
    dt_ptr new_faces[4];
    int facedex = 0;

    qe_ptr boundary_edge;

    dt_ptr lface = e->Lface();
    lface->dontAnchor(e);
    new_faces[facedex++] = lface;

    if(on_edge(x, e))
    {
        if(ccw_boundary(e))
        {
            // e lies on the boundary
            // Defer deletion until after new edges are added.
            boundary_edge = e;
        }
        else
        {
            dt_ptr sym_lface = e->Sym()->Lface();
            new_faces[facedex++] = sym_lface;
            sym_lface->dontAnchor(e->Sym());

            e = e->Oprev();
            delete_edge(e->Onext());
        }
    }
    else
    {
        // x lies within the Lface of e
    }

    qe_ptr base = m_edges->spawn();
    base->init(base);

    base->set_end_points(e->Org(), x);

    splice(base, e);

    m_starting_edge = base;
    do
    {
        base = connect(e, base->Sym());
        e = base->Oprev();
    } while(e->Lnext() != m_starting_edge);

    if(boundary_edge) delete_edge(boundary_edge);

    // Update all the faces in our new spoked polygon.
    // If point x on perimeter, then don't add an exterior face.

    base = boundary_edge ? m_starting_edge->Rprev() : m_starting_edge->Sym();

    do
    {
        if(facedex)
        {
            new_faces[--facedex]->reshape(base);
        }
        else
        {
            make_face(base);
        }

        base = base->Onext();
    } while(base != m_starting_edge->Sym());

    return m_starting_edge;
}

// s is a spoke pointing OUT from x
void DelaunayMesh::optimize(const Point2D x, qe_ptr s)
{
    qe_ptr start_spoke = s;
    qe_ptr spoke = s;

    do
    {
        qe_ptr e = spoke->Lnext();
        if(is_interior(e) && should_swap(x, e))
        {
            swap(e);
        }
        else
        {
            spoke = spoke->Onext();
            if(spoke == start_spoke) break;
        }
    } while(true);

    // Now, update all the triangles
    spoke = start_spoke;

    do
    {
        qe_ptr e = spoke->Lnext();
        dt_ptr t = e->Lface();

        if(t) this->scan_triangle(t);

        spoke = spoke->Onext();
    } while(spoke != start_spoke);
}

void DelaunayMesh::insert(const Point2D x, dt_ptr tri)
{
    qe_ptr e = tri ? locate(x, tri->getAnchor()) : locate(x);

    if((x == e->Org()) || (x == e->Dest()))
    {
        // point is already in the mesh, so update the triangles x is in
        optimize(x, e);
    }
    else
    {
        qe_ptr start_spoke = spoke(x, e);
        if(start_spoke)
        {
            optimize(x, start_spoke->Sym());
        }
    }
}

} //namespace terra
} //namespace tntn
