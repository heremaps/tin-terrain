#include "tntn/benchmark_workflow.h"

#include "tntn/Mesh.h"
#include "tntn/logging.h"
#include "tntn/File.h"
#include "tntn/SurfacePoints.h"
#include "tntn/Raster.h"
#include "tntn/RasterIO.h"
#include "tntn/FileFormat.h"
#include "tntn/MeshIO.h"
#include "tntn/Mesh2Raster.h"

#include "tntn/terra_meshing.h"
#include "tntn/simple_meshing.h"
#include "tntn/zemlya_meshing.h"

#include <memory>
#include <chrono>
#include <array>
#include <functional>
#include <cstdlib> //for getenv

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace tntn {

struct StatsRow
{
    bool is_ok = false;

    std::string input_file;
    std::string method_name;
    int input_num_points = -1;
    int input_width = -1;
    int input_height = -1;

    double param_max_error = NAN;
    double param_threshold = NAN;
    int param_step = -1;

    double meshing_time_seconds = NAN;

    double standard_dev_error = NAN;
    double mean_error = NAN;
    double max_error = NAN;

    int num_vertices = -1;
    int num_faces = -1;
};

class BenchmarkStatsCSVWriter
{
  public:
    BenchmarkStatsCSVWriter(const std::shared_ptr<FileLike>& file) : m_stats_file(file)
    {
        m_write_pos = file->size();
        m_header_line_writen = m_write_pos > 0;
    }

    bool write_row(const StatsRow& r)
    {
        const char header_line[] =
            "#input_file,method_name,"
            "input_num_points,input_width,input_height,"
            "param_max_error,param_threshold,param_step,"
            "meshing_time_seconds,mean_error,std_dev_error,max_error,"
            "num_vertices,num_faces\r\n";

        if(!m_stats_file || !m_stats_file->is_good())
        {
            TNTN_LOG_ERROR("unable to write statistics to file {}",
                           m_stats_file ? m_stats_file->name() : "null");
            return false;
        }
        if(!m_header_line_writen)
        {
            if(!write(header_line))
            {
                return false;
            }
            m_header_line_writen = true;
        }

        const std::string& row_str = format_row(r);
        TNTN_LOG_INFO("stats: {}", header_line);
        TNTN_LOG_INFO("stats: {}", row_str);

        if(!write(row_str))
        {
            return false;
        }
        return true;
    }

  private:
    bool write(const std::string& s)
    {
        const auto ok = m_stats_file->write(m_write_pos, s);
        if(ok)
        {
            m_stats_file->flush();
            m_write_pos += s.size();
        }
        return ok;
    }

    std::string format_row(const StatsRow& r) const
    {
        std::string out;
        out.reserve(512);

        out.append(r.input_file);
        out.append(",");
        out.append(r.method_name);
        out.append(",");
        out.append(std::to_string(r.input_num_points));
        out.append(",");
        out.append(std::to_string(r.input_width));
        out.append(",");
        out.append(std::to_string(r.input_height));
        out.append(",");
        out.append(std::to_string(r.param_max_error));
        out.append(",");
        out.append(std::to_string(r.param_threshold));
        out.append(",");
        out.append(std::to_string(r.param_step));
        out.append(",");
        out.append(std::to_string(r.meshing_time_seconds));
        out.append(",");
        out.append(std::to_string(r.mean_error));
        out.append(",");
        out.append(std::to_string(r.standard_dev_error));
        out.append(",");
        out.append(std::to_string(r.max_error));
        out.append(",");
        out.append(std::to_string(r.num_vertices));
        out.append(",");
        out.append(std::to_string(r.num_faces));
        out.append("\r\n");

        return out;
    }

    std::shared_ptr<FileLike> m_stats_file;
    FileLike::position_type m_write_pos = 0;
    bool m_header_line_writen = false;
};

static bool prepare_output_directory(const fs::path& output_dir, const bool resume)
{
    boost::system::error_code e;
    if(!fs::exists(output_dir))
    {
        //create output directory
        fs::create_directories(output_dir, e);
        if(e)
        {
            TNTN_LOG_ERROR("unable to create output directory {} with message: {}",
                           output_dir.string().c_str(),
                           e.message());
            return false;
        }
    }

    //make sure it is a directory
    if(!fs::is_directory(output_dir, e))
    {
        TNTN_LOG_ERROR("output directory {} is not a directory", output_dir.string().c_str());
        return false;
    }
    if(e)
    {
        TNTN_LOG_ERROR("filesystem error while checking output directory {} with message: {}",
                       output_dir.string().c_str(),
                       e.message());
        return false;
    }

    if(!resume)
    {
        //make sure it is empty
        if(!fs::is_empty(output_dir, e))
        {
            TNTN_LOG_ERROR("output directory {} is not empty", output_dir.string().c_str());
            return false;
        }
        if(e)
        {
            TNTN_LOG_ERROR("filesystem error while checking output directory {} with message: {}",
                           output_dir.string().c_str(),
                           e.message());
            return false;
        }
    }

    return true;
}

class SurfaceDescription
{
  private:
    //disallow copy and assign
    SurfaceDescription(const SurfaceDescription&) = delete;
    SurfaceDescription& operator=(const SurfaceDescription&) = delete;

  public:
    SurfaceDescription() = default;

    SurfaceDescription(std::unique_ptr<SurfacePoints> points) : m_points(std::move(points)) {}

    SurfaceDescription(std::unique_ptr<RasterDouble> raster) : m_raster(std::move(raster)) {}

    SurfaceDescription(std::unique_ptr<Raster<Vertex>> vxraster) : m_vxraster(std::move(vxraster))
    {
    }

    SurfaceDescription(SurfaceDescription&& other) noexcept { swap(other); }
    SurfaceDescription& operator=(SurfaceDescription&& other) noexcept
    {
        swap(other);
        return *this;
    }

    bool empty() const
    {
        return (!m_points && !m_raster && !m_vxraster) || (m_points && m_points->empty()) ||
            (m_raster && m_raster->empty()) || (m_vxraster && m_vxraster->empty());
    }

    void swap(SurfaceDescription& other) noexcept
    {
        std::swap(m_points, other.m_points);
        std::swap(m_raster, other.m_raster);
        std::swap(m_vxraster, other.m_vxraster);
    }

    size_t num_samples() const
    {
        if(m_points)
        {
            return m_points->size();
        }
        else if(m_raster)
        {
            return static_cast<size_t>(m_raster->get_width()) *
                static_cast<size_t>(m_raster->get_height());
        }
        else if(m_vxraster)
        {
            return static_cast<size_t>(m_vxraster->get_width()) *
                static_cast<size_t>(m_vxraster->get_height());
        }
        return 0;
    }

    const SurfacePoints* points() const { return m_points.get(); }
    const RasterDouble* raster() const { return m_raster.get(); }
    const Raster<Vertex>* vxraster() const { return m_vxraster.get(); }

    std::unique_ptr<SurfacePoints> grab_points()
    {
        if(!m_points && m_raster)
        {
            //convert raster to points
            m_points = std::make_unique<SurfacePoints>();
            m_points->load_from_raster(*m_raster);
        }

        return std::move(m_points);
    }

    std::unique_ptr<RasterDouble> grab_raster()
    {
        if(!m_raster && m_points)
        {

            // TODO:
            // this conversion loses the noDataValue and other properties!
            // needs fixing

            //convert points to raster
            m_raster = m_points->to_raster();
        }
        return std::move(m_raster);
    }

    std::unique_ptr<Raster<Vertex>> grab_vxraster()
    {
        if(!m_vxraster && m_points)
        {
            //convert points to vxraster
            m_vxraster = m_points->to_vxraster();
        }
        return std::move(m_vxraster);
    }

    SurfaceDescription clone() const
    {
        if(m_points)
        {
            return SurfaceDescription(std::make_unique<SurfacePoints>(m_points->clone()));
        }
        else if(m_raster)
        {
            return SurfaceDescription(std::make_unique<RasterDouble>(m_raster->clone()));
        }
        else if(m_vxraster)
        {
            return SurfaceDescription(std::make_unique<Raster<Vertex>>(m_vxraster->clone()));
        }
        else
        {
            return SurfaceDescription();
        }
    }

    void set_points(std::unique_ptr<SurfacePoints> points) { m_points = std::move(points); }

    void set_raster(std::unique_ptr<RasterDouble> raster) { m_raster = std::move(raster); }

    void set_vxraster(std::unique_ptr<Raster<Vertex>> vxraster)
    {
        m_vxraster = std::move(vxraster);
    }

  private:
    std::unique_ptr<SurfacePoints> m_points;
    std::unique_ptr<RasterDouble> m_raster;
    std::unique_ptr<Raster<Vertex>> m_vxraster;
};

static double to_seconds(const std::chrono::high_resolution_clock::duration& duration)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000000.0;
}
static double to_seconds(const std::chrono::high_resolution_clock::time_point& t_start,
                         const std::chrono::high_resolution_clock::time_point& t_end)
{
    return to_seconds(t_end - t_start);
}

class BenchmarkMeshingMethod
{
  public:
    virtual ~BenchmarkMeshingMethod() = default;
    virtual std::string name() const = 0;
    virtual int get_num_parametrizations() const = 0;
    virtual SurfaceDescription transform_to_preferred(SurfaceDescription&& old) const = 0;
    virtual std::string parametrization_subdir(int parameterization) const = 0;
    virtual std::unique_ptr<Mesh> generate_tin(int parametrization,
                                               const SurfaceDescription& sd) = 0;
    virtual StatsRow get_stats() const = 0;
};

class BenchmarkMeshingMethodRegular : public BenchmarkMeshingMethod
{
  public:
    BenchmarkMeshingMethodRegular() { m_stats_row.method_name = name(); }

    std::string name() const override { return "regular"; }

    int get_num_parametrizations() const override { return param_step.size(); }

    SurfaceDescription transform_to_preferred(SurfaceDescription&& old) const override
    {
        auto raster = old.grab_raster();
        return SurfaceDescription(std::move(raster));
    }

    std::string parametrization_subdir(int parametrization) const override
    {
        if(parametrization < 0 || parametrization >= param_step.size())
        {
            return name() + "_" + std::to_string(parametrization);
        }
        else
        {
            return name() + "_" + std::to_string(param_step[parametrization]);
        }
    }

    std::unique_ptr<Mesh> generate_tin(int parametrization, const SurfaceDescription& sd) override
    {

        if(parametrization < 0 && parametrization >= param_step.size())
        {
            TNTN_LOG_ERROR("invalid parametrization {}", parametrization);
            return std::make_unique<Mesh>();
        }

        const auto step = param_step[parametrization];

        m_stats_row.param_step = step;

        auto raster = sd.raster();

        if(!raster)
        {
            TNTN_LOG_ERROR("raster pointer is NULL");
            return std::make_unique<Mesh>();
        }

        auto t_start = std::chrono::high_resolution_clock::now();

        auto mesh = generate_tin_dense_quadwalk(*raster, step);

        auto t_end = std::chrono::high_resolution_clock::now();

        m_stats_row.meshing_time_seconds = to_seconds(t_start, t_end);
        m_stats_row.is_ok = mesh && !mesh->empty();

        return mesh;
    }

    StatsRow get_stats() const override { return m_stats_row; }

  private:
    const std::vector<int> param_step = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 20};

    StatsRow m_stats_row;
};

class BenchmarkMeshingMethodCurvature : public BenchmarkMeshingMethod
{
  public:
    BenchmarkMeshingMethodCurvature() { m_stats_row.method_name = name(); }

    std::string name() const override { return "curvature"; }

    int get_num_parametrizations() const override { return param_threshold.size(); }

    SurfaceDescription transform_to_preferred(SurfaceDescription&& old) const override
    {
        auto raster = old.grab_raster();
        return SurfaceDescription(std::move(raster));
    }

    std::string parametrization_subdir(int parametrization) const override
    {
        if(parametrization < 0 || parametrization >= param_threshold.size())
        {
            return name() + "_" + std::to_string(parametrization);
        }
        else
        {
            return name() + "_" + std::to_string(param_threshold[parametrization]);
        }
    }

    std::unique_ptr<Mesh> generate_tin(int parametrization, const SurfaceDescription& sd) override
    {

        if(parametrization < 0 && parametrization >= param_threshold.size())
        {
            TNTN_LOG_ERROR("invalid parametrization {}", parametrization);
            return std::make_unique<Mesh>();
        }

        const double threshold = param_threshold[parametrization];

        m_stats_row.param_threshold = threshold;

        auto raster = sd.raster();

        if(!raster)
        {
            TNTN_LOG_ERROR("raster pointer is NULL");
            return std::make_unique<Mesh>();
        }

        auto t_start = std::chrono::high_resolution_clock::now();

        auto mesh = generate_tin_curvature(*raster, threshold);

        auto t_end = std::chrono::high_resolution_clock::now();

        m_stats_row.meshing_time_seconds = to_seconds(t_start, t_end);
        m_stats_row.is_ok = mesh && !mesh->empty();

        return mesh;
    }

    StatsRow get_stats() const override { return m_stats_row; }

  private:
    const std::vector<double> param_threshold = {
        0.1, 0.2, 0.3, 0.5, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.5, 3.0};

    StatsRow m_stats_row;
};

class BenchmarkMeshingMethodTerraLike : public BenchmarkMeshingMethod
{
  public:
    int get_num_parametrizations() const override { return param_max_error.size(); }

    SurfaceDescription transform_to_preferred(SurfaceDescription&& old) const override
    {
        //Terra works on points
        auto points = old.grab_points();
        return SurfaceDescription(std::move(points));
    }

    std::string parametrization_subdir(int parametrization) const override
    {
        if(parametrization < 0 || parametrization >= param_max_error.size())
        {
            return name() + "_" + std::to_string(parametrization);
        }
        else
        {
            return name() + "_" + std::to_string(param_max_error[parametrization]);
        }
    }

    std::unique_ptr<Mesh> generate_tin(int parametrization, const SurfaceDescription& sd) override
    {
        m_stats_row.method_name = name();

        double max_error = -1;
        if(parametrization < 0 && parametrization >= param_max_error.size())
        {
            TNTN_LOG_ERROR("invalid parametrization {}", parametrization);
            return std::make_unique<Mesh>();
        }
        max_error = param_max_error[parametrization];

        m_stats_row.param_max_error = max_error;

        auto surface_points = sd.points();
        if(!surface_points)
        {
            TNTN_LOG_ERROR("surface points NULL");
            return std::make_unique<Mesh>();
        }
        auto t_start = std::chrono::high_resolution_clock::now();
        auto mesh = this->generate_tin_like_terra(*surface_points, max_error);
        auto t_end = std::chrono::high_resolution_clock::now();
        m_stats_row.meshing_time_seconds = to_seconds(t_start, t_end);

        m_stats_row.is_ok = mesh && !mesh->empty();
        return mesh;
    }

    StatsRow get_stats() const override { return m_stats_row; }

  protected:
    virtual std::unique_ptr<Mesh> generate_tin_like_terra(const SurfacePoints& sp,
                                                          const double max_error) const = 0;

  private:
    const std::vector<double> param_max_error = {
        0.1, 0.25, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.5, 2.0, 2.5, 3.0, 5.0, 8.0, 10.0};
    StatsRow m_stats_row;
};

class BenchmarkMeshingMethodTerra : public BenchmarkMeshingMethodTerraLike
{
  public:
    std::string name() const override { return "terra"; }

  protected:
    std::unique_ptr<Mesh> generate_tin_like_terra(const SurfacePoints& sp,
                                                  const double max_error) const override
    {
        return generate_tin_terra(sp, max_error);
    }
};

class BenchmarkMeshingMethodZemlya : public BenchmarkMeshingMethodTerraLike
{
  public:
    std::string name() const override { return "zemlya"; }

  protected:
    std::unique_ptr<Mesh> generate_tin_like_terra(const SurfacePoints& sp,
                                                  const double max_error) const override
    {
        return generate_tin_zemlya(sp, max_error);
    }
};

static bool prepare_parametrization_subdir(const fs::path& output_dir,
                                           const BenchmarkMeshingMethod& method,
                                           const int parametrization,
                                           const bool resume,
                                           bool& out_skip,
                                           fs::path& out_subdir,
                                           fs::path& out_is_done_file)
{
    std::string subdir = method.parametrization_subdir(parametrization);
    if(subdir.empty())
    {
        TNTN_LOG_ERROR("subdir for parametrization {} for method {} is empty",
                       parametrization,
                       method.name());
        return false;
    }

    auto parametrization_dir = output_dir / subdir;
    out_subdir = parametrization_dir;
    auto is_done_file = parametrization_dir / "benchmark_done";
    out_is_done_file = is_done_file;

    boost::system::error_code e;
    const bool is_done_file_exists = fs::exists(is_done_file, e);
    //if(e) {
    //    TNTN_LOG_ERROR("filesystem error, unable to check if is_done_file exists, message: {}", e.message());
    //    return false;
    //}
    if(resume && is_done_file_exists)
    {
        out_skip = true;
    }
    else
    {
        out_skip = false;
    }

    if(!out_skip)
    {
        const bool parametrization_dir_exists = fs::exists(parametrization_dir, e);
        //if(e) {
        //    TNTN_LOG_ERROR("filesystem error, unable to check if parametrization subdir exists, message: {}", e.message());
        //    return false;
        //}
        if(parametrization_dir_exists)
        {
            fs::remove_all(parametrization_dir, e);
            if(e)
            {
                TNTN_LOG_ERROR("unable to remove the directory {}, message: {}",
                               parametrization_dir.string(),
                               e.message());
                return false;
            }
        }
        fs::create_directories(parametrization_dir, e);
        if(e)
        {
            TNTN_LOG_ERROR("unable to create the directory {}, message: {}",
                           parametrization_dir.string(),
                           e.message());
        }
    }
    return true;
}

static bool write_mesh_as_obj_and_off(const fs::path& parametrization_subdir,
                                      const fs::path& input_file,
                                      const Mesh& m)
{
    fs::path obj_filename = std::string("meshed_") + input_file.filename().string();
    obj_filename.replace_extension(".obj");
    fs::path off_filename = std::string("meshed_") + input_file.filename().string();
    off_filename.replace_extension(".off");

    obj_filename = parametrization_subdir / obj_filename;
    off_filename = parametrization_subdir / off_filename;

    bool rc = true;
    File f;
    if(f.open(obj_filename.string().c_str(), File::OM_RWC))
    {
        rc = rc && write_mesh_as_obj(f, m);
        f.close();
    }

    if(f.open(off_filename.string().c_str(), File::OM_RWC))
    {
        rc = rc && write_mesh_as_off(f, m);
        f.close();
    }

    return rc;
}

static bool write_raster_as_asc_with_prefix(const fs::path& parametrization_subdir,
                                            const std::string& prefix,
                                            const fs::path& input_file,
                                            const RasterDouble& raster)
{
    fs::path raster_filename = prefix + input_file.filename().string();
    raster_filename.replace_extension(".asc");

    raster_filename = parametrization_subdir / raster_filename;

    File f;
    if(!f.open(raster_filename.string().c_str(), File::OM_RWC))
    {
        return false;
    }
    if(!write_raster_to_asc(f, raster))
    {
        TNTN_LOG_ERROR("error raster to file {}", raster_filename.string());
        return false;
    }
    return f.close();
}

static std::string get_user_home_dir()
{
    const char* home = std::getenv("HOME");
    if(home)
    {
        return std::string(home);
    }
    else
    {
        return std::string();
    }
}

static void strip_home_dir(std::string& path, const std::string& home_dir)
{
    if(home_dir.empty())
    {
        return;
    }
    if(path.find(home_dir) == 0)
    {
        path.replace(0, home_dir.size(), "~");
    }
}

typedef std::function<void(const StatsRow&)> write_stats_row_callback;

static bool run_all_dem2tin_method_benchmarks_on_single_file(
    const fs::path& output_dir,
    const fs::path& input_file,
    const bool resume,
    const bool no_data,
    const std::vector<std::string>& skip_methods,
    const std::vector<std::string>& select_methods,
    const std::vector<int>& skip_params,
    const std::vector<int>& select_params,
    write_stats_row_callback write_stats_row)
{
    std::vector<std::unique_ptr<BenchmarkMeshingMethod>> available_methods;

    available_methods.push_back(std::make_unique<BenchmarkMeshingMethodRegular>());
    available_methods.push_back(std::make_unique<BenchmarkMeshingMethodTerra>());
    available_methods.push_back(std::make_unique<BenchmarkMeshingMethodZemlya>());

#if defined(TNTN_USE_ADDONS) && TNTN_USE_ADDONS
    available_methods.push_back(std::make_unique<BenchmarkMeshingMethodCurvature>());
#endif

    // const auto original_surface = load_input_raster_or_points(input_file);

    auto raster = std::make_unique<RasterDouble>();
    if(!load_raster_file(input_file.string(), *raster))
    {
        TNTN_LOG_ERROR("Can not load raster input file, aborting");
        return false;
    }

    auto original_surface = SurfaceDescription(std::move(raster));

    const std::string user_home = get_user_home_dir();

    if(original_surface.empty())
    {
        TNTN_LOG_ERROR("input empty, input file was: {}", input_file.string());
        return false;
    }

    for(auto& method : available_methods)
    {
        if(!select_methods.empty() &&
           std::find(select_methods.begin(), select_methods.end(), method->name()) ==
               select_methods.end())
        {
            TNTN_LOG_INFO("skipping meshing method {} (not selected by parameter)",
                          method->name());
            continue;
        }
        if(std::find(skip_methods.begin(), skip_methods.end(), method->name()) !=
           skip_methods.end())
        {
            TNTN_LOG_INFO("skipping meshing method {} as requested by parameter", method->name());
            continue;
        }

        auto surface = method->transform_to_preferred(original_surface.clone());

        const int parametrizations = method->get_num_parametrizations();

        for(int i = 0; i < parametrizations; i++)
        {
            bool skip = false;
            fs::path parametrization_subdir;
            fs::path is_done_file;

            if(!select_params.empty() &&
               std::find(select_params.begin(), select_params.end(), i + 1) ==
                   select_params.end())
            {
                TNTN_LOG_INFO(
                    "skipping meshing method {} with parameter set {} of {} (not selected by parameter)",
                    method->name(),
                    i + 1,
                    parametrizations);
                continue;
            }

            if(!skip_params.empty() &&
               std::find(skip_params.begin(), skip_params.end(), i + 1) != skip_params.end())
            {
                TNTN_LOG_INFO(
                    "skipping meshing method {} with parameter set {} of {} as requested by parameter",
                    method->name(),
                    i + 1,
                    parametrizations);
                continue;
            }

            if(!prepare_parametrization_subdir(
                   output_dir, *method, i, resume, skip, parametrization_subdir, is_done_file))
            {
                TNTN_LOG_ERROR(
                    "error preparing subdir {} for method {} with parameter set {} of {}",
                    parametrization_subdir.string(),
                    method->name(),
                    i + 1,
                    parametrizations);
                return false;
            }

            if(skip)
            {
                TNTN_LOG_INFO(
                    "skipping meshing method {} with parameter set {} of {}... already done",
                    method->name(),
                    i + 1,
                    parametrizations);
                continue;
            }
            else
            {
                TNTN_LOG_INFO("running meshing method {} with parameter set {} of {}, aka {}...",
                              method->name(),
                              i + 1,
                              parametrizations,
                              method->parametrization_subdir(i));
            }

            auto mesh = method->generate_tin(i, surface);
            if(!mesh)
            {
                TNTN_LOG_WARN(
                    "empty mesh after running meshing method {} with parameter set {} of {}, aka {}...",
                    method->name(),
                    i + 1,
                    parametrizations);
                continue;
            }
            auto stats_row = method->get_stats();
            stats_row.input_file = input_file.string();
            stats_row.input_num_points = original_surface.num_samples();
            stats_row.num_faces = mesh->poly_count();
            stats_row.num_vertices = mesh->vertices().distance();

            if(!no_data)
            {
                TNTN_LOG_INFO(
                    "writing out resulting mesh as .obj and .off files ({} vertices, {} faces)... ",
                    mesh->vertices().distance(),
                    mesh->poly_count());
                write_mesh_as_obj_and_off(parametrization_subdir, input_file, *mesh);
            }

            //borrow the original raster and run error statistics
            auto original_raster = surface.grab_raster();
            stats_row.input_height = original_raster->get_height();
            stats_row.input_width = original_raster->get_width();

            Mesh2Raster rasteriser;
            TNTN_LOG_INFO("rasterising resulting mesh to evaluate error...");
            auto raster_from_mesh = rasteriser.rasterise(
                *mesh, original_raster->get_width(), original_raster->get_height());
            if(raster_from_mesh.empty())
            {
                TNTN_LOG_ERROR("rasterised mesh empty, unable to calculate errors");
                return false;
            }

            if(!no_data)
            {
                TNTN_LOG_INFO("writing rasterised mesh as .asc raster ({}x{}px)...",
                              raster_from_mesh.get_width(),
                              raster_from_mesh.get_height());
                write_raster_as_asc_with_prefix(
                    parametrization_subdir, "rasterized_mesh_", input_file, raster_from_mesh);
            }

            //TNTN_LOG_INFO("writing original raster as .asc raster...");
            //write_raster_as_asc_with_prefix(parametrization_subdir, "original_raster_", input_file, *original_raster);

            // rms error is the same as standard deviation in case the mean is zero
            // this, although a good assumption, is by no means certain
            // so this function also returns mean so we can double check this

            double max_error = NAN;
            double std_error = NAN;
            double mean_error = NAN;

            RasterDouble error_map_raster = Mesh2Raster::measureError(
                *original_raster, raster_from_mesh, mean_error, std_error, max_error);
            if(error_map_raster.empty())
            {
                TNTN_LOG_ERROR("error raster is empty, measuring error failed");
                return false;
            }

            error_map_raster.set_pos_x(original_raster->get_pos_x());
            error_map_raster.set_pos_y(original_raster->get_pos_y());
            error_map_raster.set_cell_size(original_raster->get_cell_size());

            stats_row.mean_error = mean_error;
            stats_row.standard_dev_error = std_error;
            stats_row.max_error = max_error;

            //push original raster back into surface
            surface.set_raster(std::move(original_raster));

            if(!no_data)
            {
                TNTN_LOG_INFO("writing error map as .asc raster ({}x{}px)...",
                              error_map_raster.get_width(),
                              error_map_raster.get_height());
                write_raster_as_asc_with_prefix(
                    parametrization_subdir, "error_raster_", input_file, error_map_raster);
            }

            strip_home_dir(stats_row.input_file, user_home);

            write_stats_row(stats_row);

            //create is_done_file to signal to later resume runs
            File f;
            f.open(is_done_file.string().c_str(), File::OM_RWC);
        }
    }
    return true;
}

static fs::path prepare_subdir_based_on_input_file(const fs::path& output_dir,
                                                   const std::string& input_file)
{
    fs::path input_file_p(input_file);
    fs::path input_file_name = input_file_p.filename();
    if(input_file_name.empty())
    {
        TNTN_LOG_ERROR("unable to get filename component from input_file {}", input_file);
        return fs::path();
    }

    auto output_subdir = output_dir / input_file_name;
    boost::system::error_code e;
    fs::create_directories(output_subdir, e);
    if(e)
    {
        TNTN_LOG_ERROR("error creating output subdirectory {} with message: {}",
                       output_subdir.string(),
                       e.message());
        return fs::path();
    }

    return output_subdir;
}

bool run_dem2tin_method_benchmarks(const std::string& output_dir,
                                   const std::vector<std::string>& input_files,
                                   const bool resume,
                                   const bool no_data,
                                   const std::vector<std::string>& skip_methods,
                                   const std::vector<std::string>& select_methods,
                                   const std::vector<int>& skip_params,
                                   const std::vector<int>& select_params)
{
    fs::path output_dir_p(output_dir);
    if(output_dir_p.empty())
    {
        TNTN_LOG_ERROR("output directory is empty");
        return false;
    }
    output_dir_p = fs::absolute(output_dir_p);
    TNTN_LOG_DEBUG("resolved output dir to absolute path: {}", output_dir_p.string());

    if(no_data)
    {
        TNTN_LOG_INFO(
            "benchmark data output is disabled, only .csv and .done files will be created");
    }

    //make sure that output directory is an empty directory or create it
    if(!prepare_output_directory(output_dir_p, resume))
    {
        return false;
    }

    auto csv_file = std::make_shared<File>();
    const auto benchmark_csv_filename = output_dir_p / "tin_terrain_benchmarks.csv";
    if(resume && fs::exists(benchmark_csv_filename))
    {
        if(!csv_file->open(benchmark_csv_filename.string().c_str(), File::OM_RW))
        {
            TNTN_LOG_ERROR("unable to open CSV output file {} for resume",
                           benchmark_csv_filename.string());
            return false;
        }
    }
    else
    {
        if(!csv_file->open(benchmark_csv_filename.string().c_str(), File::OM_RWC))
        {
            TNTN_LOG_ERROR("unable to create CSV output file {} for writing",
                           benchmark_csv_filename.string());
            return false;
        }
    }
    BenchmarkStatsCSVWriter csv_writer(csv_file);

    //foreach input file:
    //  read input
    //  for each method:
    //    create mesh
    //    rasterize mesh
    //    compare to input

    bool had_error = false;
    for(const auto& input_file : input_files)
    {
        auto subdir = prepare_subdir_based_on_input_file(output_dir_p, input_file);
        if(subdir.empty())
        {
            had_error = true;
            continue;
        }

        const bool ok = run_all_dem2tin_method_benchmarks_on_single_file(
            subdir,
            input_file,
            resume,
            no_data,
            skip_methods,
            select_methods,
            skip_params,
            select_params,
            [&had_error, &csv_writer](const StatsRow& row) {
                if(!row.is_ok)
                {
                    had_error = true;
                }
                else
                {
                    csv_writer.write_row(row);
                }
            });
        had_error = had_error || !ok;
    }

    if(!csv_file->close())
    {
        TNTN_LOG_ERROR("unable to close csv output file {}", csv_file->name());
        return false;
    }

    return !had_error;
}

} //namespace tntn
