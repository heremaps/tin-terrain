#pragma once

#include "Raster.h"
#include "tntn/File.h"
#include <string>

namespace tntn {
//template<typename T> Raster<T> load_raster_from_asc(const std::string &filename);
//template<typename T> void write_raster_to_asc(const std::string &filename,Raster<T>);

RasterDouble load_raster_from_asc(const std::string& filename);

bool write_raster_to_asc(const std::string& filename, const RasterDouble& raster);
bool write_raster_to_asc(FileLike& f, const RasterDouble& raster);

bool load_raster_file(const std::string& filename,
                      RasterDouble& raster,
                      bool validate_projection = true);
} // namespace tntn
