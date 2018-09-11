#pragma once

#include <algorithm>

namespace here {
namespace tntn {

struct ZoomRange
{
    int min_zoom = 0;
    int max_zoom = 0;

    void normalize()
    {
        if(min_zoom > max_zoom)
        {
            std::swap(min_zoom, max_zoom);
        }
    }
};

} //namespace tntn
} //namespace here
