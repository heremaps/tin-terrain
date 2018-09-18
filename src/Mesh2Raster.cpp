#include "tntn/Mesh2Raster.h"
#include "tntn/SuperTriangle.h"
#include "tntn/geometrix.h"
#include "tntn/raster_tools.h"

namespace tntn {
/**
     renders a single triangle to a raster
     by interpolating the vertex z-position inside the triangle
     traverses all pixels inside the triangle bounding box
     @param raster - image to render to
     @param tri - triangle with coordinates scaled to pixel coordinates
        of raster (i.e. colum and rows with lower left coordinate system in keeping with raster format)
     */
void Mesh2Raster::rasterise_triangle(RasterDouble& raster, SuperTriangle& tri)
{
    bool visited = false;

    int w = raster.get_width();
    int h = raster.get_height();

    BBox2D bb = tri.getBB();

    // row start and end index
    int rs = (int)(bb.min.y);
    int re = (int)(bb.max.y + 1.5);

    // column start and end index
    int cs = (int)(bb.min.x);
    int ce = (int)(bb.max.x + 1.5);

    // conform to raster bounds
#if true
    rs = rs < 0 ? 0 : rs > h ? h : rs;
    re = re < 0 ? 0 : re > h ? h : re;

    cs = cs < 0 ? 0 : cs > w ? w : cs;
    ce = ce < 0 ? 0 : ce > w ? w : ce;
#endif

    // cycle through raster
    for(int r = rs; r < re; r++)
    {
        //double* pH = raster.getPtr(h-r-1);
        double* pH = raster.get_ptr(r);

        for(int c = cs; c < ce; c++)
        {
            double terrain_height = 0;

            //if(tri.interpolate(raster.col2x(c),raster.rowLL2y(r),terrain_height))
            if(tri.interpolate(c, r, terrain_height))
            {
                visited = true;
#if false
                    if(pH[c] != ndv)
                    {
                        if(std::abs(pH[c] - terrain_height) > 0.0001)
                        {
                            TNTN_LOG_WARN("overwriting pixel with different value - before: {} after: {}",pH[c],ht);
                        }
                    }
                    
                    if( terrain_height> (maxz + 0.0001) ||  terrain_height < (minz-0.0001))
                    {
                        TNTN_LOG_WARN("interpolate out of range");
                    }
#endif
                pH[c] = terrain_height;
            }
        }
    }

    if(!visited)
    {
        TNTN_LOG_WARN("triangle NOT rendered X min: {} max: {} Y min: {} max: {}",
                      tri.getBB().min.x,
                      tri.getBB().max.x,
                      tri.getBB().min.y,
                      tri.getBB().max.y);
        TNTN_LOG_WARN("rs: {} re: {}", rs, re);
        TNTN_LOG_WARN("cs: {} ce: {}", cs, ce);

        auto t = tri.getTriangle();

        for(int i = 0; i < 3; i++)
        {
            std::vector<Vertex> plist;
            for(int j = 0; j < 3; j++)
            {
                if(i != j)
                {
                    plist.push_back(t[j]);
                }
            }

            if(plist.size() == 2)
            {

                double dx = plist[0].x - plist[1].x;
                double dy = plist[0].y - plist[1].y;

                double dd = dx * dx + dy * dy;

                double d = sqrt(dd);

                TNTN_LOG_DEBUG("edge: {} length: {}", i, d);
            }
        }
    }
    /*else
        {
            TNTN_LOG_WARN("triangle rendered X min: {} max: {} Y min: {} max: {}",tri.getBB().min.x,tri.getBB().max.x,tri.getBB().min.y,tri.getBB().max.y);
        }*/
}

/**
     renders a mesh to a raster
     uses the bounding box of vertices to define rendering area
     this is then scaled to out_width and height (height determined using mesh aspect ratio)
     note: for benchmarking it's important that the source raster and the
     raster returned by this function have the correct scaling and offset
     it's therefore necessary to know the source raster dimensions
     and it's important that the mesh algorithm sets vertices at the edges
     of the raster otherwise the re-rasterisation will be scaled.
     
     we assume that vertex positions where derived from raster positions (row, col)
     by adding cellSize/2. i.e
     x = (col+0.5) * cellSize + xpos and y = (height - row - 1 + 0.5) * cellsize + ypos
     this is the accepted way to derive coordianates
     
     in this case the x coordinate in a mesh derived for an image of original size w is in the range:
     min.x = x_start = (0+0.5) * cellSize + xpos
     max.x = x_end   = (w-1+0.5) * cellSize + xpos
     mesh_w = max.x - min.x   = (w-1+0.5) * cellSize - (0+0.5) * cellSize
     mesh_w = (w-1) * cellSize and mesh_h = (h-1) * cellSize
     this means we have two unknows - w and cellsize!!

     we further assume there is a vertex at min x/y and max x/y of the origian raster otherwise scaling cannot be guranteed

    @param out_width width of output raster - we assume this is the same as the original raster size. if not the orignal can be specified..
    @param original_width - width of original rast
    @return rasterised mesh
    */
RasterDouble Mesh2Raster::rasterise(
    Mesh& mesh, int out_width, int out_height, int original_width, int original_height)
{
    m_bb = findBoundingBox(mesh);

    double mesh_w = m_bb.max.x - m_bb.min.x;
    double mesh_h = m_bb.max.y - m_bb.min.y;

    RasterDouble raster;
    if(mesh_w <= 0 || mesh_h <= 0)
    {
        TNTN_LOG_ERROR("mesh dimensions zero");
        return raster;
    }

    if(original_width == -1) original_width = out_width;
    if(original_height == -1) original_height = out_width;

    double cellSize_original = mesh_w / (double)(original_width - 1);
    double cellSize = (mesh_w + cellSize_original) / (double)(out_width);

    // width is user supplied (in pixels)
    double raster_w = out_width;

    // derive height from mesh bb to preserve aspect ratio
    double raster_h = out_height;

    // oversample factor set here
    int oversample = 1;

    TNTN_LOG_DEBUG("oversample {}", oversample);

    // size of raster with oversampling
    double os_w = oversample * raster_w;
    double os_h = oversample * raster_h;

    // round to nearest integer for raster
    int w = (int)(os_w + 0.5);
    int h = (int)(os_h + 0.5);

    TNTN_LOG_DEBUG("raster w: {}", w);
    TNTN_LOG_DEBUG("raster h: {}", h);

    // allocate
    raster.allocate(w, h);

    // set parameters
    raster.set_no_data_value(-99999);
    raster.set_all(raster.get_no_data_value());

    TNTN_LOG_DEBUG("cell size w: {}", cellSize);

    raster.set_cell_size(cellSize);

    raster.set_pos_x(m_bb.min.x - cellSize_original * 0.5);
    raster.set_pos_y(m_bb.min.y - cellSize_original * 0.5);

    TNTN_LOG_DEBUG("x-pos:: {}", raster.get_pos_x());
    TNTN_LOG_DEBUG("y-pos: {}", raster.get_pos_y());

    mesh.generate_triangles();
    auto trange = mesh.triangles();

    int tcount = 0;
    for(auto ptr = trange.begin; ptr != trange.end; ptr++)
    {
        SuperTriangle super_triangle(scaleTriangle(*ptr, raster));
        //SuperTriangle super_triangle(*ptr);
        rasterise_triangle(raster, super_triangle);

        if((tcount % (1 + trange.distance() / 10)) == 0)
        {
            float p = tcount / (float)trange.distance();
            TNTN_LOG_INFO("{} %", p * 100.0);
        }
        tcount++;
    }
    
#ifdef TNTN_DEBUG
    // count empty pixels
    // i.e. where rendere hasn't been able to add height
    int countempty = 0;
    double ndv = raster.get_no_data_value();
    for(int r = 2; r < raster.get_height() - 2; r++)
    {
        double* pH = raster.get_ptr(r);

        for(int c = 2; c < raster.get_width() - 2; c++)
        {
            if(pH[c] == ndv)
            {
                countempty++;
            }
        }
    }

    if(countempty > 0)
    {
        TNTN_LOG_WARN("{} empty pixel ", countempty);
    }
#endif

    if(oversample != 1)
    {
        raster = raster_tools::integer_downsample_mean(raster, oversample);
    }

    return raster;
}

// return value is the root of the mean squared error
// errorMap is the difference for that pixels position
// maxError is the maximum abs difference between any two pixel values
double Mesh2Raster::findRMSError(const RasterDouble& r1,
                                 const RasterDouble& r2,
                                 RasterDouble& errorMap,
                                 double& maxError)
{
    int w = r1.get_width();
    int h = r1.get_height();

    if(h != r2.get_height())
    {
        return 0;
    }

    if(w != r2.get_width())
    {
        return 0;
    }

    long double sum = 0;
    long int count = 0;

    maxError = -std::numeric_limits<double>::max();

    errorMap.allocate(w, h);
    errorMap.set_no_data_value(-99999);
    errorMap.set_all(errorMap.get_no_data_value());

    double r1ndv = r1.get_no_data_value();
    double r2ndv = r2.get_no_data_value();

    TNTN_LOG_DEBUG("no data value for r1 : {}", r1ndv);
    TNTN_LOG_DEBUG("no data value for r2 : {}", r2ndv);

    int count_r1_nd = 0;
    int count_r2_nd = 0;
    int count_er_nd = 0;

    // ignore 2 pixel boundary around raster
    // in each direction
    for(int r = 2; r < h - 2; r++)
    {
        double* pE = errorMap.get_ptr(r);
        double* pH1 = r1.get_ptr(r);
        double* pH2 = r2.get_ptr(r);

        for(int c = 2; c < w - 2; c++)
        {
            if(pH1[c] == r1ndv) count_r1_nd++;
            if(pH2[c] == r2ndv) count_r2_nd++;

            // only perform comparison for non-empty pixels
            if((pH1[c] != r1ndv) && (pH2[c] != r2ndv))
            {
                double d = std::abs(pH1[c] - pH2[c]);

                double dd = d * d;

                // store abs error map
                pE[c] = d;

                // max error
                if(d > maxError)
                {
                    maxError = d;
                }

                // rms error
                sum += dd;
                count++;
            }
            else
            {
                count_er_nd++;
                pE[c] = 0;
                //pE[c] = errorMap.getNoDataValue();
            }
        }
    }

    TNTN_LOG_DEBUG("findRMS - no data in");
    TNTN_LOG_DEBUG("r1 : {}", count_r1_nd);
    TNTN_LOG_DEBUG("r2 : {}", count_r2_nd);
    TNTN_LOG_DEBUG("error map : {}", count_er_nd);

    // compute standard deviation
    if(count > 0)
    {
        sum = sum / (long double)count;
        sum = sqrt(sum);
    }

    return sum;
}

RasterDouble Mesh2Raster::measureError(const RasterDouble& r1,
                                       const RasterDouble& r2,
                                       double& out_mean,
                                       double& out_std,
                                       double& out_max_abs_error)
{
    int w = r1.get_width();
    int h = r1.get_height();

    // cant be negative
    double max_abs_error = 0;
    double std = 0;
    double mean = 0;

    RasterDouble errorMap;

    if(h != r2.get_height() || w != r2.get_width() || r1.empty() || r2.empty())
    {
        return errorMap;
    }

    errorMap.allocate(w, h);
    errorMap.set_no_data_value(-99999);
    errorMap.set_all(errorMap.get_no_data_value());

    double r1ndv = r1.get_no_data_value();
    double r2ndv = r2.get_no_data_value();

    TNTN_LOG_DEBUG("no data value for r1 : {}", r1ndv);
    TNTN_LOG_DEBUG("no data value for r2 : {}", r2ndv);

    int count_r1_nd = 0;
    int count_r2_nd = 0;
    int count_er_nd = 0;

    /*
         using single pass to compute variance and mean
         
         use method that preserves numerical accuracy:
         http://jonisalonen.com/2013/deriving-welfords-method-for-computing-variance/
         
         variance(samples):
         
         M := 0
         S := 0
         
         for k from 1 to N:
             x := samples[k]
             oldM := M
             M := M + (x-M)/k
             S := S + (x-M)*(x-oldM)
         return S/(N-1)
        */

    long double m_sum = 0;
    long double s_sum = 0;

    long double sum = 0;
    long int count = 0;

    // ignore 2 pixel boundary around raster
    for(int r = 2; r < h - 2; r++)
    {
        double* pE = errorMap.get_ptr(r); // output error map
        double* pH1 = r1.get_ptr(r); // raster 1
        double* pH2 = r2.get_ptr(r); // raster 2

        for(int c = 2; c < w - 2; c++)
        {
            // for debug reasons count empty pixels
            if(pH1[c] == r1ndv) count_r1_nd++;
            if(pH2[c] == r2ndv) count_r2_nd++;

            // only perform comparison for
            // non-empty pixels in both rasters
            if((pH1[c] != r1ndv) && (pH2[c] != r2ndv))
            {
                // the error we want to measure
                long double d = pH1[c] - pH2[c];

                //oldM := M
                long double old_m = m_sum;

                //M := M + (x-M)/k
                m_sum = m_sum + (d - m_sum) / (long double)(count + 1);

                //S := S + (x-M)*(x-oldM)
                s_sum = s_sum + (d - m_sum) * (d - old_m);

                // computing mean
                sum += d;

                // for max error calculation
                double d_abs = std::abs(d);

                // max error
                if(d_abs > max_abs_error)
                {
                    max_abs_error = d_abs;
                }

                // ...and error map
                pE[c] = d_abs;

                // increment count
                // as not all pixels will be included
                count++;
            }
            else
            {
                // count ocurrance
                count_er_nd++;

                // unknown error value at this point
                // so make no data value
                pE[c] = errorMap.get_no_data_value();
            }
        }
    }

    TNTN_LOG_DEBUG("measureError - no data in");
    TNTN_LOG_DEBUG("r1 : {}", count_r1_nd);
    TNTN_LOG_DEBUG("r2 : {}", count_r2_nd);
    TNTN_LOG_DEBUG("error map : {}", count_er_nd);

    // compute standard deviation
    if(count > 0)
    {
        double variance = (double)(s_sum / (long double)(count));

        // mean and variance
        std = sqrt(variance);
        mean = sum / (long double)count;
    }

    // only set output in success return path so caller can keep NaNs
    out_std = std;
    out_mean = mean;
    out_max_abs_error = max_abs_error;
    return errorMap;
}

BBox2D Mesh2Raster::findBoundingBox(Mesh& mesh)
{
    BBox2D bb;
    auto vrange = mesh.vertices();
    for(auto ptr = vrange.begin; ptr != vrange.end; ptr++)
    {
        bb.add(*ptr);
    }

    return bb;
}

Triangle Mesh2Raster::scaleTriangle(const Triangle& t, float w, float h)
{
    Triangle nt;
    for(int i = 0; i < 3; i++)
    {
        nt[i] = scaleVertex(t[i], w, h);
    }
    return nt;
}

Triangle Mesh2Raster::scaleTriangle(const Triangle& t, RasterDouble& raster)
{
    Triangle nt;
    for(int i = 0; i < 3; i++)
    {
        nt[i] = scaleVertex(t[i], raster);
    }
    return nt;
}

Vertex Mesh2Raster::scaleVertex(const Vertex& v, float w, float h)
{
    Vertex vs;

    vs.x = (w - 1) * (double)(v.x - m_bb.min.x) / (double)(m_bb.max.x - m_bb.min.x);
    vs.y = (h - 1) * (double)(v.y - m_bb.min.y) / (double)(m_bb.max.y - m_bb.min.y);

    //vs.x = w * (double)(v.x - m_bb.min.x) / (double)(m_bb.max.x - m_bb.min.x);
    //vs.y = h * (double)(v.y - m_bb.min.y) / (double)(m_bb.max.y - m_bb.min.y);

    vs.z = v.z;
    vs.z = v.z;

    return vs;
}

Vertex Mesh2Raster::scaleVertex(const Vertex& v, RasterDouble& raster)
{
    Vertex vs;
    vs.x = raster.x2col(v.x);
    vs.y = raster.y2row(v.y);
    vs.z = v.z;

    return vs;
}
} // namespace tntn
