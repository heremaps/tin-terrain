#pragma once

#include <string>
#include <vector>

namespace tntn {

bool run_dem2tin_method_benchmarks(const std::string& output_dir,
                                   const std::vector<std::string>& input_files,
                                   const bool resume,
                                   const bool no_data,
                                   const std::vector<std::string>& skip_methods,
                                   const std::vector<std::string>& select_methods,
                                   const std::vector<int>& skip_params,
                                   const std::vector<int>& select_params);

} //namespace tntn
