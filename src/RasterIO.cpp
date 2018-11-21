
#include "tntn/RasterIO.h"
#include "fmt/format.h"
#include "tntn/logging.h"
#include "tntn/println.h"
#include "tntn/util.h"
#include "tntn/raster_tools.h"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <iomanip>

#include <ogr_spatialref.h>
#include <gdal_priv.h>

#include "tntn/gdal_init.h"

namespace tntn {

std::string getFirstNonZero(std::vector<std::string> tokens, int start)
{
    for(int i = start; i < tokens.size(); i++)
    {
        if(tokens[i].size() > 0) return tokens[i];
    }

    return "";
}

// reads from file in ESRI ASCII Raster format
// http://resources.esri.com/help/9.3/arcgisdesktop/com/gp_toolref/spatial_analyst_tools/esri_ascii_raster_format.htm
RasterDouble load_raster_from_asc(const std::string& filename)
{
    TNTN_LOG_INFO("load raster...");

    RasterDouble raster;
    std::ifstream input(filename);

    int count = 0;
    int r = 0;
    int c = 0;

    int width = 0;
    int height = 0;

    int numPoints = 0;

    std::vector<std::string> tokens;
    for(std::string line; getline(input, line);)
    {
        tokenize(line, tokens);

        if(tokens.size() > 1)
        {
            if(count == 0)
            {
                width = std::stoi(getFirstNonZero(tokens, 1));
            }
            else if(count == 1)
            {
                height = std::stoi(getFirstNonZero(tokens, 1));
                raster.allocate(width, height);
            }
            else if(count == 2)
            {
                raster.set_pos_x(std::stod(getFirstNonZero(tokens, 1)));
            }
            else if(count == 3)
            {
                raster.set_pos_y(std::stod(getFirstNonZero(tokens, 1)));
            }
            else if(count == 4)
            {
                raster.set_cell_size(std::stod(getFirstNonZero(tokens, 1)));
            }
            else if(count == 5)
            {
                raster.set_no_data_value(std::stod(getFirstNonZero(tokens, 1)));
            }
            else
            {
#if true

                // read data
                if(tokens.size() != raster.get_width())
                {
                    TNTN_LOG_ERROR("can't read data");
                    return raster;
                }
                else
                {
                    c = 0;
                    for(auto p : tokens)
                    {
                        try
                        {
                            raster.value(r, c) = std::stod(p);
                        }
                        catch(const char* msg)
                        {
                            std::cout << msg << std::endl;
                            raster.value(r, c) = raster.get_no_data_value();
                        }

                        numPoints++;
                        c++;
                    }

                    if((r % (raster.get_height() / 10)) == 0)
                    {
                        float p = r / (float)raster.get_height();
                        TNTN_LOG_INFO("{} %", p * 100.0);
                    }

                    r++;
                }
#else
                std::stringstream stream(input);
                while(true)
                {
                    double v;
                    stream >> v;
                    if(!stream) break;
                }
#endif
            }
        }

        count++;
    }

    TNTN_LOG_INFO("done");

    return raster;
}

// writes to file in ESRI ASCII Raster format
// http://resources.esri.com/help/9.3/arcgisdesktop/com/gp_toolref/spatial_analyst_tools/esri_ascii_raster_format.htm
bool write_raster_to_asc(const std::string& filename, const RasterDouble& raster)
{
    File f;
    if(!f.open(filename.c_str(), File::OM_RWCF))
    {
        TNTN_LOG_ERROR("unable to open raster file {} for writing", filename);
        return false;
    }
    return write_raster_to_asc(f, raster);
}

bool write_raster_to_asc(FileLike& file, const RasterDouble& raster)
{
    /*
     NCOLS xxx
     NROWS xxx
     XLLCENTER xxx | XLLCORNER xxx
     YLLCENTER xxx | YLLCORNER xxx
     CELLSIZE xxx
     NODATA_VALUE xxx
     row 1
     row 2
     ...
     row n
    */
    if(!file.is_good())
    {
        return false;
    }
    TNTN_LOG_INFO("write raster...");

    fmt::memory_buffer line_buffer;
    line_buffer.reserve(4096);
    FileLike::position_type write_position = 0;

    //file << "NCOLS "        << raster.getWidth()    << std::endl;
    fmt::format_to(line_buffer, "NCOLS {}\n", raster.get_width());
    //file << "NROWS "        << raster.getHeight()   << std::endl;
    fmt::format_to(line_buffer, "NROWS {}\n", raster.get_height());
    //file << "XLLCORNER "    << std::setprecision(9) << raster.getXPos()        << std::endl;
    fmt::format_to(line_buffer, "XLLCORNER {:.9f}\n", raster.get_pos_x());
    //file << "YLLCORNER "    << std::setprecision(9) << raster.getYPos()        << std::endl;
    fmt::format_to(line_buffer, "YLLCORNER {:.9f}\n", raster.get_pos_y());
    //file << "CELLSIZE "     << std::setprecision(9) << raster.getCellsize()    << std::endl;
    fmt::format_to(line_buffer, "CELLSIZE {:.9f}\n", raster.get_cell_size());
    //file << "NODATA_VALUE " << std::setprecision(9) << raster.getNoDataValue() << std::endl;
    fmt::format_to(line_buffer, "NODATA_VALUE {:.9f}\n", raster.get_no_data_value());

    if(!file.write(write_position, line_buffer.data(), line_buffer.size())) return false;
    write_position += line_buffer.size();
    line_buffer.resize(0);

    if(!file.is_good())
    {
        return false;
    }

    for(int r = 0; r < raster.get_height(); r++)
    {
        for(int c = 0; c < raster.get_width(); c++)
        {
            fmt::format_to(line_buffer, "{} ", raster.value(r, c));
        }

        fmt::format_to(line_buffer, "\n");

        if(!file.write(write_position, line_buffer.data(), line_buffer.size())) return false;
        write_position += line_buffer.size();
        line_buffer.resize(0);

        if((r % (1 + raster.get_height() / 10)) == 0)
        {
            float p = r / (float)raster.get_height();
            TNTN_LOG_INFO("{} %", p * 100.0);
        }
    }

    if(!file.write(write_position, line_buffer.data(), line_buffer.size())) return false;
    line_buffer.resize(0);

    TNTN_LOG_INFO("done");

    return true;
}

// --------------------------------------------------------------------------------
// Import raster from GDAL

typedef std::unique_ptr<GDALDataset, void (*)(GDALDataset*)> GDALDataset_ptr;

struct TransformationMatrix
{
    union
    {
        double matrix[6];
        struct
        {
            double origin_x;
            double scale_x;
            double padding_0;
            double origin_y;
            double padding_1;
            double scale_y;
        };
    };
};

static void GDALClose_wrapper(GDALDataset* p)
{
    if(p != nullptr)
    {
        GDALClose(p);
    }
}

static bool get_transformation_matrix(GDALDataset* dataset, TransformationMatrix& gt)
{
    if(dataset->GetGeoTransform(gt.matrix) != CE_None)
    {
        TNTN_LOG_ERROR("Input raster is missing geotransformation matrix");
        return false;
    }

    if(fabs(gt.scale_x) != fabs(gt.scale_y))
    {
        TNTN_LOG_ERROR("Can not process rasters with non square pixels ({}x{})",
                       fabs(gt.scale_x),
                       fabs(gt.scale_y));
        return false;
    }

    return true;
}

static bool is_valid_projection(GDALDataset* dataset)
{
    // returned pointer should not be altered, freed or expected to last for long.
    const char* projection_wkt = dataset->GetProjectionRef();

    if(projection_wkt == NULL)
    {
        TNTN_LOG_ERROR("Input raster file does not provide spatial reference information");
        return false;
    }

    OGRSpatialReference raster_srs;

#if GDAL_VERSION_MAJOR == 2 && GDAL_VERSION_MINOR < 3
    raster_srs.importFromWkt(const_cast<char**>(&projection_wkt));
#else
    raster_srs.importFromWkt(projection_wkt);
#endif

    int matches_number;

    OGRSpatialReference web_mercator;
    web_mercator.importFromEPSG(3857);

    bool matched = false;

#if GDAL_VERSION_MAJOR == 2 && GDAL_VERSION_MINOR < 3
    OGRErr match_projection_error = raster_srs.AutoIdentifyEPSG();

    if(match_projection_error != 0)
    {
        TNTN_LOG_ERROR("Can not match projection to EPSG:3857");
        return false;
    }

    if(web_mercator.IsSame(&raster_srs))
    {
        matched = true;
    }
#else
    // Matched projections must be freed with OSRFreeSRSArray()
    OGRSpatialReferenceH* matched_projections =
            raster_srs.FindMatches(NULL, &matches_number, NULL);

    if(matched_projections == 0)
    {
        TNTN_LOG_ERROR("Can not match projection to EPSG:3857");
        return false;
    }

    for(int i = 0; i < matches_number; i++)
    {
        if(web_mercator.IsSame(static_cast<OGRSpatialReference*>(matched_projections[i])))
        {
            matched = true;
            break;
        }
    }

    if(matched_projections)
    {
        OSRFreeSRSArray(matched_projections);
    }
#endif

    if(!matched)
    {
        TNTN_LOG_ERROR("Can not match projection to EPSG:3857");
    }

    return matched;
}

bool load_raster_file(const std::string& file_name,
                      RasterDouble& target_raster,
                      bool validate_projection)
{
    initialize_gdal_once();

    TNTN_LOG_INFO("Opening raster file {} with GDAL...", file_name);

    GDALDataset_ptr dataset(static_cast<GDALDataset*>(GDALOpen(file_name.c_str(), GA_ReadOnly)),
                            &GDALClose_wrapper);

    if(dataset == nullptr)
    {
        TNTN_LOG_ERROR("Can't open input raster {}: ", file_name);
        return false;
    }

    TransformationMatrix gt;

    if(!get_transformation_matrix(dataset.get(), gt))
    {
        return false;
    }

    if(validate_projection && !is_valid_projection(dataset.get()))
    {
        println("input raster file must be in EPSG:3857 (Web Mercator) format");
        println("you can reproject raster terrain using GDAL");
        println("as follows: 'gdalwarp -t_srs EPSG:3857 input.tif output.tif'");

        return false;
    }

    int bands_count = dataset->GetRasterCount();

    if(bands_count == 0)
    {
        TNTN_LOG_ERROR("Can't process a raster file witout raster bands");
        return false;
    }
    else if(bands_count > 1)
    {
        TNTN_LOG_WARN("File {} has {} raster bands, processing with a raster band #1",
                      file_name,
                      bands_count);
    }

    // TODO: Perhaps make raster band number a parameter
    GDALRasterBand* raster_band = dataset->GetRasterBand(1);

    int raster_width = raster_band->GetXSize();
    int raster_height = raster_band->GetYSize();

    target_raster.set_cell_size(fabs(gt.scale_x));
    target_raster.allocate(raster_width, raster_height);
    target_raster.set_no_data_value(raster_band->GetNoDataValue());

    TNTN_LOG_INFO("reading raster data...");
    if(raster_band->RasterIO(GF_Read,
                             0,
                             0,
                             raster_width,
                             raster_height,
                             target_raster.get_ptr(),
                             raster_width,
                             raster_height,
                             GDT_Float64,
                             0,
                             0) != CE_None)
    {
        TNTN_LOG_ERROR("Can not read raster data");
        return false;
    }

    double x1 = gt.origin_x;
    double y1 = gt.origin_y;
    double x2 = gt.origin_x + raster_width * gt.scale_x;
    double y2 = gt.origin_y + raster_height * gt.scale_y;

    // Ensure raster's origin is exactly at the lower left corner
    target_raster.set_pos_x(std::min(x1, x2));
    target_raster.set_pos_y(std::min(y1, y2));

    if(gt.scale_x < 0)
    {
        raster_tools::flip_data_x(target_raster);
    }

    if(gt.scale_y > 0)
    {
        raster_tools::flip_data_y(target_raster);
    }

    return true;
}

} // namespace tntn
