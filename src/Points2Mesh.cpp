
#include "tntn/Points2Mesh.h"
#include "tntn/logging.h"

#include "delaunator_cpp/Delaunator.h"

using namespace std;
namespace tntn {

bool generate_delaunay_faces(const std::vector<Vertex>& vlist, std::vector<Face>& faces)
{
    std::vector<double> points;
    points.reserve(vlist.size());

    for(int i = 0; i < vlist.size(); i++)
    {
        points.push_back(vlist[i].x);
        points.push_back(vlist[i].y);
    }

    delaunator_cpp::Delaunator dn;

    if(dn.triangulate(points))
    {
        faces.reserve(dn.triangles.size() / 3);
        for(int i = 0; i < dn.triangles.size() / 3; i++)
        {
            Face f;
            f[0] = dn.triangles[3 * i];
            f[1] = dn.triangles[3 * i + 1];
            f[2] = dn.triangles[3 * i + 2];
            faces.push_back(f);
        }
        return true;
    }

    return false;
}

std::vector<int> check_duplicates(const std::vector<Vertex>& vlist, double precision)
{
    int dcount = 0;

    std::vector<int> duplicates;

    double p2 = precision * precision;
    for(int i = 0; i < vlist.size(); i++)
    {
        for(int j = i; j < vlist.size(); j++)
        {
            if(i != j)
            {
                double dx = vlist[i].x - vlist[j].x;
                double dy = vlist[i].y - vlist[j].y;
                double dd = dx * dx + dy * dy;

                if(dd < p2)
                {
                    duplicates.push_back(i);
                    dcount++;
                }
            }
        }
    }

    return duplicates;
}

void remove_duplicates(std::vector<Vertex>& vlist, const std::vector<int> del)
{
    for(int i : del)
    {
        vlist[i] = vlist.back();
        vlist.pop_back();
    }
}

std::unique_ptr<Mesh> generate_delaunay_mesh(std::vector<Vertex>&& vlist)
{
    std::vector<Face> faces;
    generate_delaunay_faces(vlist, faces);
    std::unique_ptr<Mesh> pMesh = std::make_unique<Mesh>();
    pMesh->from_decomposed(std::move(vlist), std::move(faces));
    return pMesh;
}

} // namespace tntn
