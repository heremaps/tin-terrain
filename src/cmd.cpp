#include "tntn/dem2tintiles_workflow.h"
#include "tntn/terra_meshing.h"
#include "tntn/zemlya_meshing.h"
#include "tntn/MeshIO.h"
#include "tntn/Mesh.h"
#include "tntn/RasterIO.h"
#include "tntn/Raster.h"
#include "tntn/logging.h"
#include "tntn/Mesh2Raster.h"
#include "tntn/FileFormat.h"
#include "tntn/QuantizedMeshIO.h"
#include "tntn/benchmark_workflow.h"
#include "tntn/simple_meshing.h"
#include "tntn/version_info.h"
#include "tntn/RasterOverviews.h"
#include "tntn/println.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <string>

namespace po = boost::program_options;

namespace tntn {

//forward decl for use in preset commands
int tin_terrain_commandline_action(std::vector<std::string> args);

typedef int (*subcommand_function_t)(bool need_help,
                                     const po::variables_map&,
                                     const std::vector<std::string>&);
struct subcommand_t
{
    const char* name;
    subcommand_function_t handler;
    const char* description;
};

static int subcommand_dem2tintiles(bool need_help,
                                   const po::variables_map& global_varmap,
                                   const std::vector<std::string>& unrecognized)
{
    po::options_description subdesc("dem2tintiles options");
    // clang-format off
    subdesc.add_options()
        ("input", po::value<std::string>(), "input filename")
        ("output-dir", po::value<std::string>()->default_value("./output"))
        ("max-zoom", po::value<int>()->default_value(-1), "maximum zoom level to generate tiles for. will guesstimate from resolution if not provided.")
        ("min-zoom", po::value<int>()->default_value(-1), "minimum zoom level to generate tiles for will guesstimate from resolution if not provided.")
        ("max-error", po::value<double>(), "max error parameter when using terra or zemlya method")
        ("step", po::value<int>()->default_value(1), "grid spacing in pixels when using dense method")
        ("output-format", po::value<std::string>()->default_value("terrain"), "output tiles in terrain (quantized mesh) or obj")
#if defined(TNTN_USE_ADDONS) && TNTN_USE_ADDONS
        ("method", po::value<std::string>()->default_value("terra"), "meshing algorithm. one of: terra, zemlya, curvature or dense")
        ("threshold", po::value<double>(), "threshold when using curvature method");
#else
		("method", po::value<std::string>()->default_value("terra"), "meshing algorithm. one of: terra, zemlya or dense");
#endif
    // clang-format on

    auto parsed = po::command_line_parser(unrecognized).options(subdesc).run();

    po::variables_map local_varmap;
    po::store(parsed, local_varmap);
    po::notify(local_varmap);

    if(need_help)
    {
        println("usage:");
        println("  tin-terrain dem2tintiles [OPTION]...");
        println();
        println(subdesc);
        return 0;
    }

    const int max_zoom = local_varmap["max-zoom"].as<int>();
    if(max_zoom != -1 && (max_zoom < 0 || max_zoom > 21))
    {
        throw po::error("--max-zoom must be in range [0,21]");
    }

    const int min_zoom = local_varmap["min-zoom"].as<int>();
    if(min_zoom != -1 && (min_zoom < 0 || min_zoom > 21))
    {
        throw po::error("--min-zoom must be in range [0,21]");
    }

    if(max_zoom != -1 && max_zoom < min_zoom)
    {
        throw po::error("max zoom is less than min zoom");
    }

    if(!local_varmap.count("input"))
    {
        throw po::error("no --input option given");
    }
    const std::string input_file = local_varmap["input"].as<std::string>();

    const std::string output_basedir = local_varmap["output-dir"].as<std::string>();

    if(!boost::filesystem::is_regular_file(input_file))
    {
        throw std::runtime_error(std::string("input file ") + input_file + " does not exist");
    }

    bool max_error_given = false;
    double max_error = 0;

    if(local_varmap.count("max-error"))
    {
        max_error_given = true;
        max_error = local_varmap["max-error"].as<double>();
    }

    if(max_error < 0.0)
    {
        throw po::error("max-error must be positive");
    }

    std::unique_ptr<MeshWriter> w;

    if(local_varmap["output-format"].as<std::string>() == "obj")
    {
        w.reset(new ObjMeshWriter());
    }
    else if(local_varmap["output-format"].as<std::string>() == "terrain")
    {
        w.reset(new QuantizedMeshWriter());
    }
    else
    {
        throw po::error(std::string("unknow ouput-format: ") +
                        local_varmap["output-format"].as<std::string>());
    }

    const std::string meshing_method = local_varmap["method"].as<std::string>();

    auto input_raster = std::make_unique<RasterDouble>();

    if(!load_raster_file(input_file.c_str(), *input_raster))
    {
        return false;
    }

    if("zemlya" == meshing_method || "terra" == meshing_method)
    {
        max_error = input_raster->get_cell_size();
        if(local_varmap.count("max-error"))
        {
            max_error_given = true;
            max_error = local_varmap["max-error"].as<double>();
        }

        if(max_error < 0.0)
        {
            throw po::error("max-error must be positive");
        }
    }
#if defined(TNTN_USE_ADDONS) && TNTN_USE_ADDONS
    else if("curvature")
    {
        double threshold = 1;
        if(local_varmap.count("threshold"))
        {
            threshold = local_varmap["threshold"].as<double>();
        }
    }
#endif
    else
    {
        throw po::error(std::string("unknown method ") + meshing_method);
    }

    RasterOverviews overviews(std::move(input_raster), min_zoom, max_zoom);

    RasterOverview overview;

    while(overviews.next(overview))
    {
        if(!max_error_given)
        {
            max_error = overview.resolution;
        }

        const int zoom_level = overview.zoom_level;

        int overview_width = overview.raster->get_width();
        int overview_height = overview.raster->get_height();

        if(overview_height < 1 || overview_width < 1)
        {
            TNTN_LOG_WARN("raster on zoom level {} empty, cannot create tiles, proceeding...",
                          zoom_level);
            continue;
        }

        TNTN_LOG_INFO("Processing zoom level {}, raster size {}x{}",
                      zoom_level,
                      overview_width,
                      overview_height);

        const auto& partitions = create_partitions_for_zoom_level(*overview.raster, zoom_level);

        if(partitions.empty())
        {
            continue;
        }

        if(!create_tiles_for_zoom_level(*overview.raster,
                                        partitions,
                                        zoom_level,
                                        output_basedir,
                                        max_error,
                                        meshing_method,
                                        *w))
        {
            TNTN_LOG_ERROR("error creating files for zoom level {}", zoom_level);
            return -2;
        }
    }

    return 0;
}

static void remove_leading_dot(std::string& s)
{
    if(s.empty()) return;
    if(s[0] == '.') s.erase(s.begin());
}

static FileFormat validate_output_format_for_mesh(const std::string& output_filename,
                                                  std::string output_format_str)
{
    auto file_ext = boost::filesystem::extension(output_filename);
    remove_leading_dot(file_ext);
    if(output_format_str == "auto")
    {
        output_format_str = file_ext;
    }
    auto f = FileFormat::from_string(output_format_str);
    if(f != FileFormat::NONE && f.to_string() != file_ext)
    {
        TNTN_LOG_WARN("output file extension \"{}\" doesn't match output file format {}",
                      file_ext,
                      output_format_str);
    }

    const std::vector<FileFormat> valid_formats = {
        FileFormat::OBJ,
        FileFormat::OFF,
        FileFormat::TERRAIN,
        FileFormat::JSON,
        FileFormat::GEOJSON,
    };

    if(std::find(valid_formats.begin(), valid_formats.end(), f) == valid_formats.end())
    {
        throw po::error(std::string("invalid/unsupported output format ") + output_format_str);
    }
    return f;
}

static int subcommand_dem2tin(bool need_help,
                              const po::variables_map& global_varmap,
                              const std::vector<std::string>& unrecognized)
{
    TNTN_LOG_INFO("subcommand_dem2tin");
    po::options_description subdesc("dem2tin options");

#if defined(TNTN_USE_ADDONS) && TNTN_USE_ADDONS
    std::cout << "works";
#endif

    // clang-format off
    subdesc.add_options()
        ("input", po::value<std::string>(), "input filename")
        ("input-format",po::value<std::string>()->default_value("auto"), "input file format, can be any of: auto, asc, xyz, tiff")
        ("output", po::value<std::string>(), "output filename")
        ("output-format", po::value<std::string>()->default_value("auto"), "output file format, can be any of: auto, obj, off, terrain (quantized mesh), json/geojson")
        ("max-error", po::value<double>(), "max error parameter when using terra or zemlya method")
        ("step", po::value<int>(), "grid spacing in pixels when using dense method")
#if defined(TNTN_USE_ADDONS) && TNTN_USE_ADDONS
        ("threshold", po::value<double>(), "threshold when using curvature method")
        ("method", po::value<std::string>()->default_value("terra"), "meshing method, valid values are: dense, terra, zemlya, curvature");
#else
        ("method", po::value<std::string>()->default_value("terra"), "meshing method, valid values are: dense, terra, zemlya");
#endif
    // clang-format on

    auto parsed = po::command_line_parser(unrecognized).options(subdesc).run();

    po::variables_map local_varmap;
    po::store(parsed, local_varmap);
    po::notify(local_varmap);

    if(need_help)
    {
        println("usage: ");
        println("  tin-terrain dem2tin [OPTIONS]...");
        println();
        println(subdesc);

        println("methods:");
        println(
            "  terra     - implements a delaunay based triangulation with point selection using a fast greedy insert mechnism");
        println(
            "    reference: Garland, Michael, and Paul S. Heckbert. \"Fast polygonal approximation of terrains and height fields.\" (1995).");
        println("    paper: https://mgarland.org/papers/scape.pdf");
        println("    and http://mgarland.org/archive/cmu/scape/terra.html");
        println("  zemlya    - hierarchical greedy insertion");
        println(
            "    reference: Zheng, Xianwei, et al. \"A VIRTUAL GLOBE-BASED MULTI-RESOLUTION TIN SURFACE MODELING AND VISUALIZETION METHOD.\" International Archives of the Photogrammetry, Remote Sensing & Spatial Information Sciences 41 (2016).");
        println(
            "    paper: https://www.int-arch-photogramm-remote-sens-spatial-inf-sci.net/XLI-B2/459/2016/isprs-archives-XLI-B2-459-2016.pdf");
        println(
            "  dense     - generates a simple mesh grid from the raster input by placing one vertex per pixel");
#if defined(TNTN_USE_ADDONS) && TNTN_USE_ADDONS
        println("  curvature - sets points when curvature integral is larger than threshold");
#endif
        return 0;
    }

    if(!local_varmap.count("input"))
    {
        throw po::error("no --input option given");
    }

    const std::string input_file = local_varmap["input"].as<std::string>();

    if(!local_varmap.count("output"))
    {
        throw po::error("no --output option given");
    }

    const std::string output_file = local_varmap["output"].as<std::string>();
    const std::string output_file_format_str = local_varmap["output-format"].as<std::string>();
    const FileFormat output_file_format =
        validate_output_format_for_mesh(output_file, output_file_format_str);

    const std::string method = local_varmap["method"].as<std::string>();

    auto raster = std::make_unique<RasterDouble>();

    // Import raster file without projection validation
    if(!load_raster_file(input_file, *raster, false))
    {
        TNTN_LOG_ERROR("Unable to load input file, aborting");
        return false;
    }

    TNTN_LOG_INFO("done");

    std::unique_ptr<Mesh> mesh;

    const auto t_start = std::chrono::high_resolution_clock::now();

    if(method == "terra" || method == "zemlya")
    {
        double max_error = raster->get_cell_size();
        if(local_varmap.count("max-error"))
        {
            max_error = local_varmap["max-error"].as<double>();
        }

        if("terra" == method)
        {
            TNTN_LOG_INFO("performing terra meshing...");
            mesh = generate_tin_terra(std::move(raster), max_error);
        }
        else if("zemlya" == method)
        {
            TNTN_LOG_INFO("performing zemlya meshing...");
            mesh = generate_tin_zemlya(std::move(raster), max_error);
        }
    }
    else if(method == "dense")
    {
        int step = 1;
        if(local_varmap.count("step"))
        {
            step = local_varmap["step"].as<int>();
        }

        TNTN_LOG_INFO("generating dense mesh grid ...");
        mesh = generate_tin_dense_quadwalk(*raster, step);
    }
#if defined(TNTN_USE_ADDONS) && TNTN_USE_ADDONS
    else if(method == "curvature")
    {
        if(!local_varmap.count("threshold"))
        {
            throw po::error("no --threshold given, required for this method");
        }

        double gradient_threshold = local_varmap["threshold"].as<double>();

        if(gradient_threshold <= 0.0)
        {
            throw po::error("threshold must be larger than zero");
        }

        TNTN_LOG_INFO("performing curverture integral meshing...");
        mesh = generate_tin_curvature(*raster, gradient_threshold);
    }
#endif
    else
    {
        throw po::error(std::string("unknown method ") + method);
    }

    TNTN_LOG_INFO("done");

    const auto t_end = std::chrono::high_resolution_clock::now();

    // save mesh output

    if(!mesh || mesh->empty())
    {
        TNTN_LOG_ERROR("mesh empty or null, meshing failed");
        return -1;
    }

    //in case the meshing step didn't produce the required output

    if(output_file_format.optimal_mesh_mode() == MeshMode::decomposed)
    {
        mesh->generate_decomposed();
    }
    else if(output_file_format.optimal_mesh_mode() == MeshMode::triangles)
    {
        mesh->generate_triangles();
    }

    {
        const auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();
        const auto duration_seconds = duration / 1000000.0;
        TNTN_LOG_INFO("time for meshing step: {} sec = {} minutes",
                      duration_seconds,
                      (duration / (1000000.0 * 60.0)));
        TNTN_LOG_INFO("number of triangles: {}", mesh->poly_count());
        TNTN_LOG_INFO("number of vertices: {}", mesh->vertices().distance());
    }

    TNTN_LOG_INFO("writing mesh... ({})", output_file);
    if(!write_mesh_to_file(output_file.c_str(), *mesh, output_file_format))
    {
        TNTN_LOG_ERROR("error writing output file");
        return -1;
    }
    TNTN_LOG_INFO("done");

    return 0;
}

static int subcommand_benchmark(bool need_help,
                                const po::variables_map& global_varmap,
                                const std::vector<std::string>& unrecognized)
{
    po::options_description subdesc("benchmark options");
    // clang-format off
    subdesc.add_options()
        ("output-dir,o", po::value<std::string>()->required(), "output directory that must be empty (or not exist)")
        ("input,i", po::value<std::vector<std::string>>()->multitoken()->composing()->required(), "input raster filename(s)")
        ("resume", "resume interrupted benchmark runs in the same output directory, assumes the same set of input filenames")
        ("skip-method", po::value<std::vector<std::string>>()->composing(), "skip a certain method, can be given multiple times")
        ("select-method", po::value<std::vector<std::string>>()->composing(), "select a certain method for execution, can be given multiple times")
        ("skip-param", po::value<std::vector<int>>()->composing(), "skip a certain parameter set, can be given multiple times")
        ("select-param", po::value<std::vector<int>>()->composing(), "select the parameter set to run, can be given multiple times")
        ("no-data", "disable writing benchmark results to disk")
    ;
    // clang-format on

    po::positional_options_description local_pos_desc;
    local_pos_desc.add("output-dir", 1).add("input", -1);

    auto parsed =
        po::command_line_parser(unrecognized).options(subdesc).positional(local_pos_desc).run();

    if(need_help)
    {
        println("usage:");
        println("  tin-terrain benchmark [OPTION]... <OUTPUT_DIR> <INPUT_FILE>...");
        println();
        println(subdesc);
        return 0;
    }

    po::variables_map local_varmap;
    po::store(parsed, local_varmap);
    po::notify(local_varmap);

    const auto input = local_varmap["input"].as<std::vector<std::string>>();
    const auto output_dir = local_varmap["output-dir"].as<std::string>();
    const bool resume = local_varmap.count("resume") > 0;
    const bool no_data = local_varmap.count("no-data") > 0;

    std::vector<std::string> skip_methods;
    if(local_varmap.count("skip-method") > 0)
    {
        skip_methods = local_varmap["skip-method"].as<std::vector<std::string>>();
    }

    std::vector<std::string> select_methods;
    if(local_varmap.count("select-method") > 0)
    {
        select_methods = local_varmap["select-method"].as<std::vector<std::string>>();
    }

    std::vector<int> skip_params;
    if(local_varmap.count("skip-param") > 0)
    {
        skip_params = local_varmap["skip-param"].as<std::vector<int>>();
    }

    std::vector<int> select_params;
    if(local_varmap.count("select-param") > 0)
    {
        select_params = local_varmap["select-param"].as<std::vector<int>>();
    }

    std::string input_list;
    {
        for(const auto& s : input)
        {
            input_list.append(s);
            input_list.append(", ");
        }
        input_list.erase(input_list.end() - 2, input_list.end());
    }
    TNTN_LOG_DEBUG("input: {}", input_list);
    TNTN_LOG_DEBUG("output_dir: {}", output_dir);

    if(!run_dem2tin_method_benchmarks(output_dir,
                                      input,
                                      resume,
                                      no_data,
                                      skip_methods,
                                      select_methods,
                                      skip_params,
                                      select_params))
    {
        TNTN_LOG_ERROR("benchmarking failed");
        return -1;
    }

    return 0;
}

static int subcommand_version(bool need_help,
                              const po::variables_map& global_varmap,
                              const std::vector<std::string>& unrecognized)
{
    if(need_help)
    {
        println("usage:");
        println("  tin-terrain version");
        println();
        println("prints machine readable version information to stdout");
        return 0;
    }
    println("name: tin-terrain");
    println("copyright_notice: Copyright (C) 2018 HERE Europe B.V.");
    println("git_description: {}", get_git_description());
    println("git_hash: {}", get_git_hash());
    println("build_timestamp: {}", get_timestamp());
    return 0;
}

static const subcommand_t subcommands_list[] = {
    {"dem2tin", subcommand_dem2tin, "convert a DEM into a mesh/tin"},
    {"dem2tintiles", subcommand_dem2tintiles, "convert a DEM into mesh/tin tiles"},
    {"benchmark",
     subcommand_benchmark,
     "run all available meshing methods on a given set of input files and produce statistics (performance, error rate)"},
    {"version", subcommand_version, "print version information"},
};

struct VerbosityCounter
{
    int count = 0;
    std::vector<std::string> unmatched;
    friend std::ostream& operator<<(std::ostream&& os, const VerbosityCounter& c);
};
std::ostream& operator<<(std::ostream& os, const VerbosityCounter& c)
{
    os << c.count;
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<VerbosityCounter>& v)
{
    int total_count = 0;
    for(const auto& vc : v)
    {
        total_count += vc.count;
    }
    os << total_count;
    return os;
}

void validate(boost::any& v,
              const std::vector<std::string>& values,
              VerbosityCounter* target_type,
              int)
{
    int num_v = 1;
    bool all_v = true;

    for(const std::string& s : values)
    {
        for(const char ss : s)
        {
            if(ss == 'v')
            {
                num_v++;
            }
            else
            {
                all_v = false;
                break;
            }
        }
    }

    if(all_v)
    {
        v = boost::any(VerbosityCounter{num_v});
    }
    else
    {
        VerbosityCounter vc;
        vc.count = num_v;
        vc.unmatched = values;
        v = boost::any(vc);
        //throw po::validation_error(po::validation_error::invalid_option_value);
    }
}

int tin_terrain_commandline_action(std::vector<std::string> args)
{
    po::options_description global_options{"Global Options"};
    std::vector<VerbosityCounter> implicit_verbosity_counter{VerbosityCounter{1}};
    // clang-format off
    global_options.add_options()
        ("help,h", "print this help message")
        ("log", po::value<std::string>()->default_value("stdout"), "diagnostics output/log target, can be stdout, stderr, or none")
        ("verbose,v", po::value<std::vector<VerbosityCounter>>()
            ->implicit_value(implicit_verbosity_counter)
            ->composing(),
            "be more verbose")
        ("subcommand", po::value<std::string>(), "command to execute")
        ("subargs", po::value<std::vector<std::string>>(), "arguments for command")
    ;
    // clang-format on

    po::positional_options_description global_pos_desc;
    global_pos_desc.add("subcommand", 1).add("subargs", -1);

    po::variables_map global_varmap;

    try
    {
        auto parser = po::command_line_parser(args)
                          .options(global_options)
                          .positional(global_pos_desc)
                          .allow_unregistered();
        auto parsed = parser.run();

        po::store(parsed, global_varmap);
        po::notify(global_varmap);

        std::vector<std::string> unrecognized =
            po::collect_unrecognized(parsed.options, po::include_positional);
        std::vector<std::string> verbosity_unmatched;

        const std::string log_stream = global_varmap["log"].as<std::string>();
        if(log_stream == "stdout")
        {
            log_set_global_logstream(LogStream::STDOUT);
        }
        else if(log_stream == "stderr")
        {
            log_set_global_logstream(LogStream::STDERR);
        }
        else if(log_stream == "none")
        {
            log_set_global_logstream(LogStream::NONE);
        }
        else
        {
            throw po::error(std::string("invalid valid log stream ") + log_stream);
        }

        if(global_varmap.count("verbose"))
        {
            //std::cout << "verbose_count = " << global_varmap.count("verbose") << std::endl;
            const auto verbosity_counters =
                global_varmap["verbose"].as<std::vector<VerbosityCounter>>();
            for(const auto& vc : verbosity_counters)
            {
                //std::cout << "verbosity_counter.count = " << vc.count << std::endl;
                for(const auto& s : vc.unmatched)
                {
                    verbosity_unmatched.push_back(s);
                }
                for(int i = 0; i < vc.count; i++)
                {
                    log_decrease_global_level();
                }
            }
        }

        std::string subcommand_name;
        if(global_varmap.count("subcommand"))
        {
            subcommand_name = global_varmap["subcommand"].as<std::string>();
        }

        if(!verbosity_unmatched.empty())
        {
            subcommand_name = verbosity_unmatched[0];
            verbosity_unmatched.erase(verbosity_unmatched.begin());
        }

        for(const auto& vu : verbosity_unmatched)
        {
            unrecognized.insert(unrecognized.begin(), vu);
        }

        const bool need_help = global_varmap.count("help");
        if(subcommand_name.empty())
        {
            println("usage:");
            println("  tin-terrain [OPTION]... <subcommand> ...");
            println();
            println(global_options);
            println("available subcommands:");
            for(const auto& subcommand : subcommands_list)
            {
                println("  {} - {}", subcommand.name, subcommand.description);
            }
            return 0;
        }

        for(const auto& subcommand : subcommands_list)
        {
            if(subcommand_name == subcommand.name)
            {
                if(!unrecognized.empty())
                {
                    if(unrecognized[0] == subcommand_name)
                    {
                        unrecognized.erase(unrecognized.begin());
                    }
                }
                return subcommand.handler(need_help, global_varmap, unrecognized);
            }
        }

        TNTN_LOG_FATAL("unrecognized subcommand: {}", subcommand_name);
        return -1;
    }
    catch(const po::error_with_option_name& e)
    {
        std::string option_name = e.get_option_name();
        std::string error_msg = e.what();
        TNTN_LOG_FATAL(
            "program options error regarding {}, message: {}", e.get_option_name(), e.what());
        return -1;
    }
    catch(const po::error& e)
    {
        TNTN_LOG_FATAL("program options error: {}", e.what());
        return -1;
    }
    catch(const std::bad_alloc& ba)
    {
        TNTN_LOG_FATAL(
            "tin-terrain failed to allocate enough memory. please consider using smaller dataset (std::bad_alloc)");
        return -1;
    }
    catch(const std::exception& e)
    {
        TNTN_LOG_FATAL("exception: {}", e.what());
        return -1;
    }
}

} //namespace tntn

int main(int argc, const char* argv[])
{
    std::vector<std::string> args;
    for(int i = 1; i < argc; i++)
    {
        args.push_back(argv[i]);
    }
    return tntn::tin_terrain_commandline_action(std::move(args));
}
