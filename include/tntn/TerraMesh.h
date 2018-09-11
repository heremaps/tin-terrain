#pragma once

#include "tntn/geometrix.h"
#include "Raster.h"
#include "tntn/DelaunayMesh.h"
#include "tntn/MeshIO.h"
#include "tntn/TerraUtils.h"

#include <memory>

namespace tntn {
namespace terra {

class TerraMesh : public TerraBaseMesh
{
  private:
    Raster<char> m_used;
    Raster<int> m_token;

    CandidateList m_candidates;
    double m_max_error;
    int m_counter;

    void scan_triangle_line(const Plane& plane,
                            int y,
                            double x1,
                            double x2,
                            Candidate& candidate,
                            const double no_data_value);

  public:
    void greedy_insert(double max_error);
    void scan_triangle(dt_ptr t) override;
    std::unique_ptr<Mesh> convert_to_mesh();
};

} //namespace terra
} //namespace tntn
