#include "tntn/dem2tintiles_workflow.h"
#include "tntn/MercatorProjection.h"
#include "tntn/terra_meshing.h"
#include "tntn/simple_meshing.h"
#include "tntn/zemlya_meshing.h"
#include "tntn/TileMaker.h"
#include "tntn/logging.h"

#include <vector>
#include <boost/filesystem.hpp>

namespace tntn {

namespace fs = boost::filesystem;

std::vector<Partition> create_partitions_for_zoom_level(const RasterDouble& dem, int zoom)
{
    MercatorProjection projection;
    std::vector<Partition> partitions;

    double resolution = dem.get_cell_size();
    const auto points_bbox = dem.get_bounding_box();

    // const auto points_bbox = points.bounding_box();

    auto tile_xy = projection.MetersToTileXY({points_bbox.min.x, points_bbox.min.y}, zoom);
    const int tminx = tile_xy.x;
    const int tminy = tile_xy.y;

    tile_xy = projection.MetersToTileXY({points_bbox.max.x, points_bbox.max.y}, zoom);
    const int tmaxx = tile_xy.x;
    const int tmaxy = tile_xy.y;

    const int tile_size = projection.tileSize();
    const double tile_size_in_meters = projection.tileSizeInMeters(zoom);
    int nt = static_cast<int>(resolution * 800 / tile_size_in_meters);
    if(nt == 0) nt = 1;

    for(int tx = tminx; tx <= tmaxx; tx += nt)
    {
        for(int ty = tminy; ty <= tmaxy; ty += nt)
        {
            auto bbox_min = projection.PixelsToMeters({tx * tile_size, ty * tile_size}, zoom);
            auto bbox_max =
                projection.PixelsToMeters({(tx + nt) * tile_size, (ty + nt) * tile_size}, zoom);

            BoundingBox bbox = {bbox_min,
                                {std::min(bbox_max.x, points_bbox.max.x),
                                 std::min(bbox_max.y, points_bbox.max.y)}};

            double buffer = resolution * 100; // in meters
            BoundingBox bboxWithBuffer = {{bbox.min.x - buffer, bbox.min.y - buffer},
                                          {bbox.max.x + buffer, bbox.max.y + buffer}};

            Partition part = {bboxWithBuffer,
                              {tx, ty},
                              {(bbox_max.x > points_bbox.max.x ? tmaxx : tx + nt - 1),
                               (bbox_max.y > points_bbox.max.y ? tmaxy : ty + nt - 1)}};

            partitions.push_back(part);
        }
    }
    return partitions;
}

bool create_tiles_for_zoom_level(const RasterDouble& dem,
                                 const std::vector<Partition>& partitions,
                                 int zoom,
                                 const std::string& output_basedir,
                                 const double method_parameter,
                                 const std::string& meshing_method,
                                 MeshWriter& mesh_writer)
{
    for(const auto& part : partitions)
    {
        const auto bbox = part.bbox;
        TNTN_LOG_DEBUG("current tile bbox (world coordinates) [({},{}),({},{})]",
                       bbox.min.x,
                       bbox.min.y,
                       bbox.max.x,
                       bbox.max.y);

        int x1 = dem.x2col(bbox.min.x);
        int y1 = dem.y2row(bbox.min.y);

        int x2 = dem.x2col(bbox.max.x);
        int y2 = dem.y2row(bbox.max.y);

        TNTN_LOG_DEBUG("current tile raster crop box: [({},{}),({},{})]", x1, y1, x2, y2);

        if(x2 < x1)
        {
            std::swap(x1, x2);
        }

        if(y2 < y1)
        {
            std::swap(y1, y2);
        }

        auto raster_tile = std::make_unique<RasterDouble>();
        dem.crop(x1, y1, x2 - x1, y2 - y1, *raster_tile);

        std::unique_ptr<Mesh> mesh;

        if(meshing_method == "terra")
        {
            mesh = generate_tin_terra(std::move(raster_tile), method_parameter);
        }
        else if(meshing_method == "zemlya")
        {
            mesh = generate_tin_zemlya(std::move(raster_tile), method_parameter);
        }
#if defined(TNTN_USE_ADDONS) && TNTN_USE_ADDONS
        else if(meshing_method == "curvature")
        {
            mesh = generate_tin_curvature(*raster_tile, method_parameter);
        }
#endif
        else if(meshing_method == "dense")
        {
            mesh = generate_tin_dense_quadwalk(*raster_tile, (int)method_parameter);
        }
        else
        {
            TNTN_LOG_ERROR("Unknown meshing method {}, aborting", meshing_method);
            return false;
        }

        // Cut the TIN into tiles
        TileMaker tm;
        tm.loadMesh(std::move(mesh));

        fs::create_directory(fs::path(output_basedir));
        fs::create_directory(fs::path(output_basedir) / std::to_string(zoom));

        for(int tx = part.tmin.x; tx <= part.tmax.x; tx++)
        {
            for(int ty = part.tmin.y; ty <= part.tmax.y; ty++)
            {
                TNTN_LOG_INFO("Creating tile: {},{}", tx, ty);

                auto tile_dir =
                    fs::path(output_basedir) / std::to_string(zoom) / std::to_string(tx);
                fs::create_directory(tile_dir);

                auto file_path =
                    tile_dir / (std::to_string(ty) + "." + mesh_writer.file_extension());

                if(!tm.dumpTile(tx, ty, zoom, file_path.string().c_str(), mesh_writer))
                {
                    TNTN_LOG_ERROR("error dumping tile z:{} x:{} y:{}", zoom, tx, ty);
                    return false;
                }
            }
        }
    }
    return true;
}

} //namespace tntn
