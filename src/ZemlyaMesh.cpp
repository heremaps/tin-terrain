#include "tntn/ZemlyaMesh.h"
#include "tntn/TerraUtils.h"
#include "tntn/logging.h"
#include "tntn/SurfacePoints.h"
#include "tntn/DelaunayTriangle.h"
#include "tntn/raster_tools.h"

#include <iostream>
#include <fstream>
#include <array>
#include <unordered_map>
#include <cmath>

namespace tntn {
namespace zemlya {

static double average_of(
    const double d1, const double d2, const double d3, const double d4, double no_data_value)
{
    int count = 0;
    double sum = 0.0;

    for(double d : {d1, d2, d3, d4})
    {
        if(terra::is_no_data(d, no_data_value))
        {
            continue;
        }
        count++;
        sum += d;
    }

    if(count > 0)
    {
        return sum / count;
    }
    else
    {
        return NAN;
    }
}

void ZemlyaMesh::greedy_insert(double max_error)
{
    m_max_error = max_error;
    m_counter = 0;
    int w = m_raster->get_width();
    int h = m_raster->get_height();
    m_max_level = static_cast<int>(ceil(log2(w > h ? w : h)));

    TNTN_LOG_INFO("starting greedy insertion with raster width: {}, height: {}", w, h);

    // Create another raster to store average values
    m_sample.allocate(w, h);
    m_sample.set_all(NAN);

    const double no_data_value = m_raster->get_no_data_value();

    for(int level = m_max_level - 1; level >= 1; level--)
    {
        int step = m_max_level - level;
        for(int y = 0; y < h; y += pow(2, step))
        {
            for(int x = 0; x < w; x += pow(2, step))
            {
                if(step == 1)
                {
                    double v1 = y < h && x < w ? m_raster->value(y, x) : NAN;
                    double v2 = y < h && x + 1 < w ? m_raster->value(y, x + 1) : NAN;
                    double v3 = y + 1 < h && x < w ? m_raster->value(y + 1, x) : NAN;
                    double v4 = y + 1 < h && x + 1 < w ? m_raster->value(y + 1, x + 1) : NAN;

                    if(y + 1 < h && x + 1 < w)
                    {
                        m_sample.value(y + 1, x + 1) = average_of(v1, v2, v3, v4, no_data_value);
                    }
                }
                else
                {
                    int co = pow(2, step - 1); // offset
                    int d = pow(2, step - 2); // delta

                    double v1 = y + co - d < h && x + co - d < w
                        ? m_sample.value(y + co - d, x + co - d)
                        : NAN;
                    double v2 = y + co - d < h && x + co + d < w
                        ? m_sample.value(y + co - d, x + co + d)
                        : NAN;
                    double v3 = y + co + d < h && x + co - d < w
                        ? m_sample.value(y + co + d, x + co - d)
                        : NAN;
                    double v4 = y + co + d < h && x + co + d < w
                        ? m_sample.value(y + co + d, x + co + d)
                        : NAN;

                    if(y + co < h && x + co < w)
                    {
                        m_sample.value(y + co, x + co) =
                            average_of(v1, v2, v3, v4, no_data_value);
                    }
                }
            }
        }
    }

    // Ensure the four corners are not NAN, otherwise the algorithm can't proceed.
    this->repair_point(0, 0);
    this->repair_point(0, h - 1);
    this->repair_point(w - 1, h - 1);
    this->repair_point(w - 1, 0);

    // Initialize m_result
    m_result.allocate(w, h);
    m_result.set_all(NAN);

    m_result.value(0, 0) = m_raster->value(0, 0);
    m_result.value(h - 1, 0) = m_raster->value(h - 1, 0);
    m_result.value(h - 1, w - 1) = m_raster->value(h - 1, w - 1);
    m_result.value(0, w - 1) = m_raster->value(0, w - 1);

    // Initialize m_insert
    m_insert.allocate(w, h);
    m_insert.set_all(NAN);

    // Initialize m_used
    m_used.allocate(w, h);

    // Initialize m_token
    m_token.allocate(w, h);
    m_token.set_all(0);

    // Initialize the mesh to two triangles with the height field grid corners as vertices
    TNTN_LOG_INFO("initialize the mesh with four corner points");
    this->init_mesh(
        glm::dvec2(0, 0), glm::dvec2(0, h - 1), glm::dvec2(w - 1, h - 1), glm::dvec2(w - 1, 0));

    // Iterate over the levels
    for(int level = 1; level <= m_max_level; level++)
    {
        m_current_level = level;
        TNTN_LOG_INFO("starting level {}", level);

        // Clear m_used
        m_used.set_all(0);

        // Use points from the original raster starting from level 5 to compensate for the half-pixel offset of average values.
        if(level >= 5 && level <= m_max_level - 1)
        {
            int step = m_max_level - level;

            // Update points from previous levels
            for(int y = 0; y < h; y++)
            {
                for(int x = 0; x < w; x++)
                {
                    double& z = m_insert.value(y, x);
                    if(terra::is_no_data(z, no_data_value))
                    {
                        continue;
                    }
                    z = m_raster->value(y, x);
                }
            }

            // Add new points from this level
            for(int y = 0; y < h; y += pow(2, step))
            {
                for(int x = 0; x < w; x += pow(2, step))
                {
                    int co = pow(2, step - 1); // current step's offset
                    if(y + co < h && x + co < w)
                    {
                        m_insert.value(y + co, x + co) = m_raster->value(y + co, x + co);
                    }
                }
            }
        }
        else if(level < m_max_level)
        {
            int step = m_max_level - level;

            // Update points from previous levels by shrinking their commanding areas
            // This is crucial for transitioning from global averages to local values.
            if(step >= 3)
            {
                double d = pow(2, step - 3); // delta

                for(int y = 0; y < h; y++)
                {
                    for(int x = 0; x < w; x++)
                    {
                        double& z = m_insert.value(y, x);
                        if(terra::is_no_data(z, no_data_value))
                        {
                            continue;
                        }

                        const double v1 =
                            y - d < h && x - d < w ? m_sample.value(y - d, x - d) : NAN;
                        const double v2 =
                            y - d < h && x + d < w ? m_sample.value(y - d, x + d) : NAN;
                        const double v3 =
                            y + d < h && x - d < w ? m_sample.value(y + d, x - d) : NAN;
                        const double v4 =
                            y + d < h && x + d < w ? m_sample.value(y + d, x + d) : NAN;
                        const double avg = average_of(v1, v2, v3, v4, no_data_value);
                        if(terra::is_no_data(avg, no_data_value))
                        {
                            continue;
                        }
                        z = avg;
                    }
                }
            }

            // Add new points from this level
            for(int y = 0; y < h; y += pow(2, step))
            {
                for(int x = 0; x < w; x += pow(2, step))
                {
                    int co = pow(2, step - 1); // current step's offset
                    if(y + co < h && x + co < w)
                    {
                        m_insert.value(y + co, x + co) = m_sample.value(y + co, x + co);
                    }
                }
            }
        }

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
            terra::Candidate candidate = m_candidates.grab_greatest();

            if(candidate.importance < m_max_error) continue;

            // Skip if the candidate is not the latest
            if(m_token.value(candidate.y, candidate.x) != candidate.token) continue;

            m_result.value(candidate.y, candidate.x) = candidate.z;
            m_used.value(candidate.y, candidate.x) = 1;

            //TNTN_LOG_DEBUG("inserting point: ({}, {}, {})", candidate.x, candidate.y, candidate.z);
            this->insert(glm::dvec2(candidate.x, candidate.y), candidate.triangle);
        }
    }

    TNTN_LOG_INFO("finished greedy insertion");
}

void ZemlyaMesh::scan_triangle_line(const Plane& plane,
                                    int y,
                                    double x1,
                                    double x2,
                                    Candidate& candidate,
                                    const double no_data_value)
{
    const int startx = static_cast<int>(ceil(fmin(x1, x2)));
    const int endx = static_cast<int>(floor(fmax(x1, x2)));

    if(startx > endx) return;

    double z0 = plane.eval(startx, y);
    const double dz = plane.a;

    for(int x = startx; x <= endx; x++)
    {
        if(!m_used.value(y, x))
        {
            //attention - use m_raster/m_insert depending on level
            const double z =
                m_current_level == m_max_level ? m_raster->value(y, x) : m_insert.value(y, x);
            if(!terra::is_no_data(z, no_data_value))
            {
                const double diff = fabs(z - z0);
                //TNTN_LOG_DEBUG("candidate consider: ({}, {}, {}), diff: {}", x, y, z, diff);
                candidate.consider(x, y, z, diff);
            }
        }
        z0 += dz;
    }
}

void ZemlyaMesh::scan_triangle(dt_ptr t)
{
    Plane z_plane;
    terra::compute_plane(z_plane, t, m_result);

    std::array<Point2D, 3> by_y = {{
        t->point1(),
        t->point2(),
        t->point3(),
    }};
    terra::order_triangle_points(by_y);
    const double v0_x = by_y[0].x;
    const double v0_y = by_y[0].y;
    const double v1_x = by_y[1].x;
    const double v1_y = by_y[1].y;
    const double v2_x = by_y[2].x;
    const double v2_y = by_y[2].y;

    terra::Candidate candidate = {0, 0, 0.0, -DBL_MAX, m_counter++, t};

    const double dx2 = (v2_x - v0_x) / (v2_y - v0_y);
    const double no_data_value = m_raster->get_no_data_value();

    if(v1_y != v0_y)
    {
        const double dx1 = (v1_x - v0_x) / (v1_y - v0_y);

        double x1 = v0_x;
        double x2 = v0_x;

        const int starty = v0_y;
        const int endy = v1_y;

        for(int y = starty; y < endy; y++)
        {
            scan_triangle_line(z_plane, y, x1, x2, candidate, no_data_value);
            x1 += dx1;
            x2 += dx2;
        }
    }

    if(v2_y != v1_y)
    {
        const double dx1 = (v2_x - v1_x) / (v2_y - v1_y);

        double x1 = v1_x;
        double x2 = v0_x;

        const int starty = v1_y;
        const int endy = v2_y;

        for(int y = starty; y <= endy; y++)
        {
            scan_triangle_line(z_plane, y, x1, x2, candidate, no_data_value);
            x1 += dx1;
            x2 += dx2;
        }
    }

    // We have now found the appropriate candidate point
    m_token.value(candidate.y, candidate.x) = candidate.token;

    // Push the candidate into the stack
    m_candidates.push_back(candidate);
}

std::unique_ptr<Mesh> ZemlyaMesh::convert_to_mesh()
{
    // Find all the vertices
    int w = m_raster->get_width();
    int h = m_raster->get_height();

    std::vector<Vertex> mvertices;

    Raster<int> vertex_id;
    vertex_id.allocate(w, h);
    vertex_id.set_all(0);

    int index = 0;
    const double no_data_value = m_raster->get_no_data_value();
    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            const double z = m_result.value(y, x);
            if(!terra::is_no_data(z, no_data_value))
            {
                Vertex v = Vertex({m_raster->col2x(x), m_raster->row2y(y), z});
                mvertices.push_back(v);
                vertex_id.value(y, x) = index;
                index++;
            }
        }
    }

    // Find all the faces
    std::vector<Face> mfaces;
    dt_ptr t = m_first_face;
    while(t)
    {
        Face f;

        glm::dvec2 p1 = t->point1();
        glm::dvec2 p2 = t->point2();
        glm::dvec2 p3 = t->point3();

        if(!terra::ccw(p1, p2, p3))
        {
            f[0] = vertex_id.value((int)p1.y, (int)p1.x);
            f[1] = vertex_id.value((int)p2.y, (int)p2.x);
            f[2] = vertex_id.value((int)p3.y, (int)p3.x);
        }
        else
        {
            f[0] = vertex_id.value((int)p3.y, (int)p3.x);
            f[1] = vertex_id.value((int)p2.y, (int)p2.x);
            f[2] = vertex_id.value((int)p1.y, (int)p1.x);
        }

        mfaces.push_back(f);

        t = t->getLink();
    }

    // now initialise our mesh class with this
    auto mesh = std::make_unique<Mesh>();
    mesh->from_decomposed(std::move(mvertices), std::move(mfaces));
    return mesh;
}

} //namespace zemlya
} //namespace tntn
