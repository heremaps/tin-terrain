
#include "tntn/SuperTriangle.h"

namespace tntn {

SuperTriangle::SuperTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3)
{
    m_t[0] = v1;
    m_t[1] = v2;
    m_t[2] = v3;

    init();
}

SuperTriangle::SuperTriangle(const Triangle& tr)
{
    m_t = tr;

    init();
}

BBox2D SuperTriangle::getBB()
{
    return m_bb;
}

Triangle SuperTriangle::getTriangle()
{
    return m_t;
}

bool SuperTriangle::interpolate(const double x, const double y, double& z)
{
    auto& v1 = m_t[0];
    auto& v2 = m_t[1];
    auto& v3 = m_t[2];

    // https://codeplea.com/triangular-interpolation
    const double w1 = ((v2.y - v3.y) * (x - v3.x) + (v3.x - v2.x) * (y - v3.y)) / m_wdem;
    const double w2 = ((v3.y - v1.y) * (x - v3.x) + (v1.x - v3.x) * (y - v3.y)) / m_wdem;
    const double w3 = 1.0 - w1 - w2;

    z = (double)(v1.z * w1 + v2.z * w2 + v3.z * w3);

    if(0 <= w1 && w1 <= 1 && 0 <= w2 && w2 <= 1 && 0 <= w3 && w3 <= 1)
        return true;
    else
        return false;
}

void SuperTriangle::rasterise(RasterDouble& raster)
{
    int w = raster.get_width();
    int h = raster.get_height();

    int rs = (int)(m_bb.min.y + 0.5);
    int re = (int)(m_bb.max.y + 1.5);

    int cs = (int)(m_bb.min.x + 0.5);
    int ce = (int)(m_bb.max.x + 1.5);

    // bounds check
    rs = rs < 0 ? 0 : rs > h ? h : rs;
    re = re < 0 ? 0 : re > h ? h : re;

    cs = cs < 0 ? 0 : cs > w ? w : cs;
    ce = ce < 0 ? 0 : ce > w ? w : ce;

#if false
        double ndv = raster.getNoDataValue();

        double minz = m_v1.z < m_v2.z ? m_v1.z : m_v2.z;
        minz = minz < m_v3.z ? minz : m_v3.z;
        
        double maxz = m_v1.z > m_v2.z ? m_v1.z : m_v2.z;
        maxz = maxz > m_v3.z ? maxz : m_v3.z;
#endif

    for(int r = rs; r < re; r++)
    {
        double* pH = raster.get_ptr(h - r - 1);

        for(int c = cs; c < ce; c++)
        {
            double ht;
            if(interpolate(c + 0.5, r + 0.5, ht))
            {
#if false
                    if(pH[c] != ndv)
                    {
                        if(std::abs(pH[c] - ht) > 0.0001)
                        {
                            TNTN_LOG_WARN("overwriting pixel with different value - before: {} after: {}",pH[c],ht);
                        }
                    }
                    
                    if(ht > (maxz + 0.0001) || ht < (minz-0.0001))
                    {
                        TNTN_LOG_WARN("interpolate out of range");
                    }
#endif

                pH[c] = ht;
            }
        }
    }
}

void SuperTriangle::init()
{
    auto& v1 = m_t[0];
    auto& v2 = m_t[1];
    auto& v3 = m_t[2];

    m_wdem = (v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y);

    findBounds();
}

void SuperTriangle::findBounds()
{
    for(int i = 0; i < 3; i++)
        m_bb.add(m_t[i]);
}
} // namespace tntn
