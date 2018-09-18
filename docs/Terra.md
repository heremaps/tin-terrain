# Terra

The "Terra" meshing algorithm implemented in the TIN Terrain tool is based on [Michael Garland's 1995 research paper](https://mgarland.org/papers/scape.pdf) on greedy insertion algorithms and his [open-source implementation](https://mgarland.org/software/terra.html) (released to public domain).

Paper: Garland, Michael, and Paul S. Heckbert. "Fast polygonal approximation of terrains and height fields." (1995).

## Algorithm

The basic algorithm of greedy insertion starts with a simple triangulation of the original raster. For example, you can use the four corner points of the original raster to form two triangles as the initial mesh.

Then on each pass, find the raster point with the highest error in the current mesh, and insert it as a new vertex. The mesh is then updated using incremental Delaunay triangulation.

The insertion process is greedy, meaning that once a point is inserted, it's never updated or deleted. It's also sequential, as only one point is inserted at a time.

The error metric is defined as the absolute difference between the height value of the original raster point and the interpolated height value at the same point of the current mesh approximation. This simple error metric definition turns out to work better than more sophisticated ones.

The incremental Delaunay triangulation works as follows:
1. To insert a new vertex A, first locate its containing triangle.
2. Add "spoke" edges from vertex A to the vertices of this containing triangle.
3. All perimeter edges of the containing polygon need to be validated since they may not conform to Delaunay requirements any more. All invalid edges must be swapped with the other diagonal of the quadrilateral containing them. Continue this process until no invalid edges are left.

Since it is costly to scan the whole raster to find the highest error point, we can cache the error calculations by storing the highest error point (called a "candidate point") of each triangle in the current mesh approximation. When a triangle is changed, we update the candidate point of this triangle. This way we can avoid recalculating the highest error points of all triangles on each insertion.

## Implementation

Below we describe in detail how the "Terra" meshing algorithm is implemented in the TIN Terrain tool.

### Quad-Edge Data Structure

The Quad-Edge data structure is used to efficiently describe and walk the mesh. This data structure is designed to find the vertices/edges/faces immediately adjacent to a given vertex/edge/face in constant time.

In the TIN Terrain tool, the quad-edge data structure is implemented in "QuadEdge.h/cpp" and "DelaunayTriangle.h/cpp".

![7AD175B29AB688F685EBE45F6D46FAE3.jpg](https://raw.githubusercontent.com/heremaps/tin-terrain/algorithm-docs.ylian/docs/resources/7AD175B29AB688F685EBE45F6D46FAE3.jpg)

(Illustration from <http://www.cs.cmu.edu/afs/andrew/scs/cs/15-463/2001/pub/src/a2/quadedge.html>)

Each QuadEdge object internally stores pointers to three more edges: its inverse (`e->Sym`), dual edge pointing to the left (`e->Rot`), and dual edge pointing to the right (`e->InvRot`), as shown in the illustration above.

```c
class QuadEdge
{
    qe_ptr qnext; //aka Rot
    qe_ptr qprev; //aka invRot
    qe_ptr self_ptr;
    qe_ptr next; //aka Onext
    
    //next edge around origin, with same origin
    qe_ptr Onext() const noexcept { return next; }

    //edge pointing opposite to this
    qe_ptr Sym() const { return qnext->qnext; }

    //dual edge pointing to the left of this
    qe_ptr Rot() const noexcept { return qnext; }

    //dual edge pointing to the right of this
    qe_ptr invRot() const noexcept { return qprev; }
}
```

![4D859D9DDCB9690EFB04E6CCC75FABDC.jpg](https://raw.githubusercontent.com/heremaps/tin-terrain/algorithm-docs.ylian/docs/resources/4D859D9DDCB9690EFB04E6CCC75FABDC.jpg)

(Illustration from <http://www.cs.cmu.edu/afs/andrew/scs/cs/15-463/2001/pub/src/a2/quadedge.html>)

With these pointers, a QuadEdge object can quickly find the adjacent vertices/edges/faces. For example, `e->Rot()->Onext()->Rot()` gives the previous edge around the same origin.

Below are convenience methods defined in the QuadEdge class for quick access to adjacent edges:

```c
class QuadEdge
{
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
};
```

For a deeper understanding of the Quad-Edge data structure, please read this handout:

<http://graphics.stanford.edu/courses/cs348a-17-winter/ReaderNotes/handout31.pdf>

### Incremental Delaunay

The incremental Delaunay triangulation is implemented in "DelaunayMesh.h/cpp". The key method in this class is `DelaunayMesh::insert()`, which inserts a new vertex into the mesh. Inserting a new vertex requires creating spokes from the new vertex to the vertices of the enclosing triangle, as well as updating the affected edges so that they still satisfy Delaunay requirements.

```c
void DelaunayMesh::insert(const Point2D x, dt_ptr tri)
{
    qe_ptr e = tri ? locate(x, tri->getAnchor()) : locate(x);

    if((x == e->Org()) || (x == e->Dest()))
    {
        // point is already in the mesh, skip it
        return;
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
```

Every time a triangle is updated during re-triangulation, we scan the triangle again to update the candidate point with the greatest error, as discussed in the next section.

### Greedy Insertion

The greedy insertion algorithm is implemented in "TerraMesh.h/cpp".

The algorithm starts with an initial mesh configuration: two triangles formed by four corner points. If any corner point is missing from the original raster, we fill the point with its closest average value.

```c
// Ensure the four corners are not NO_VALUE, otherwise the algorithm can't proceed.
this->repair_point(0, 0);
this->repair_point(0, h - 1);
this->repair_point(w - 1, h - 1);
this->repair_point(w - 1, 0);

// Initialize the mesh to two triangles with the height field grid corners as vertices
this->init_mesh(glm::dvec2(0, 0), glm::dvec2(0, h - 1), glm::dvec2(w - 1, h - 1), glm::dvec2(w - 1, 0));
```

Then we scan all the triangles and push the points with highest errors in each triangle into a priority queue. The candidates points are stored in a priority queue, because we want to retrieve the point with the greatest error as quickly as possible. We iterate this process until no point with an error higher than a given `max-error` tolerance is found.

Note that an inserted point is immediately marked as used, so it won't be scanned again in the future. An inserted point is never updated or removed.

```c
// Scan all the triangles and push all candidates into a stack
dt_ptr t = m_first_face;
while(t)
{
    scan_triangle(t);
    t = t->getLink();
}

// Iterate until the error threshold is met
while(!m_candidates.empty())
{
    Candidate candidate = m_candidates.grab_greatest();

    if(candidate.importance < m_max_error) continue;

    // Skip if the candidate is not the latest
    if(m_token.value(candidate.y, candidate.x) != candidate.token) continue;

    m_used.value(candidate.y, candidate.x) = 1;

    this->insert(glm::dvec2(candidate.x, candidate.y), candidate.triangle);
}
```

After the iteration process is over, we can export the mesh to an OBJ file using the `TerraMesh::convert_to_mesh()` method.
