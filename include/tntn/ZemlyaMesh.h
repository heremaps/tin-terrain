#pragma once

#include "tntn/geometrix.h"
#include "tntn/Raster.h"
#include "tntn/DelaunayMesh.h"
#include "tntn/MeshIO.h"
#include "tntn/TerraUtils.h"

#include <memory>

namespace tntn {
namespace zemlya {

using dt_ptr = terra::dt_ptr;
using qe_ptr = terra::qe_ptr;
using Plane = terra::Plane;
using Point2D = terra::Point2D;
using CandidateList = terra::CandidateList;
using Candidate = terra::Candidate;

class ZemlyaMesh : public terra::TerraBaseMesh
{
  private:
    Raster<double> m_sample;
    Raster<double> m_insert;
    Raster<double> m_result;
    Raster<char> m_used;
    Raster<int> m_token;

    CandidateList m_candidates;
    double m_max_error = 0;
    int m_counter = 0;
    int m_current_level = 0;
    int m_max_level = 0;

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

} //namespace zemlya
} //namespace tntn
