#include "tntn/gdal_init.h"
#include "tntn/logging.h"
#include <mutex> //for call_once

#include <gdal.h>

namespace tntn {

std::once_flag g_gdal_initialized_once_flag;

void initialize_gdal_once()
{
    std::call_once(g_gdal_initialized_once_flag, []() {
        TNTN_LOG_DEBUG("calling GDALAllRegister...");
        GDALAllRegister();
    });
}

} //namespace tntn
