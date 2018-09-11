# TIN Terrain
TIN Terrain is a tool for converting heightmaps presented in GeoTIFF format into tiled optimised mesh
(Triangulated Irregular Network) with different levels of details.

Note: This is experimental code, expect changes.


## Features

 * takes GeoTIFF with heightmap as an input
 * transforms heightmap into a TIN mesh with a given max-error parameter and
    outputs into `.obj` format
 * transforms heightmap into tiled TIN mesh for a given zoom range and
    outputs tiled pyramid into
    [quantized-mesh-1.0](https://github.com/AnalyticalGraphicsInc/quantized-mesh)
    terrain format


### Building with Docker
The provided Dockerfile builds the TIN Terrain executable on Ubuntu Linux.

To build the container execute:

```
./build-docker.sh
```

To run TIN Terrain from Docker, type e.g. 

    docker run -v [local data directory]:/data:cached --name tin-terrain --rm -i -t tin-terrain tin-terrain ...
	
where `[local data directory]` is the folder that contains your DEM files and will receive the output files. This will be mapped to `/data` in the Docker instance, so you should use:

    docker run -v [local data directory]:/data:cached --name tin-terrain --rm -i -t tin-terrain tin-terrain dem2tin --input /data/... -output /data/... 
	
Alternatively, use 
    
	docker run -v [local data directory]:/data:cached --name tin-terrain --rm -i -t tin-terrain bash 
	
to open an interactive shell which lets you execute `tin-terrain` and access `/data`. Any files not stored in `/data` will be lost when closing the session.
	


## Dependencies and License

Copyright (C) 2018 HERE Europe B.V.
Unless otherwise noted in [LICENSE](LICENSE) files for specific files or directories,
the LICENSE in the root applies to all content in this repository.

TIN Terrain downloads some sourcecode of 3rd party libraries during the cmake configure phase.

* libfmt (BSD-2-Clause) https://github.com/fmtlib/fmt
* glm (MIT) https://github.com/g-truc/glm
* Catch2 (BSL-1.0) https://github.com/catchorg/Catch2

You can disable this behaviour by passing the `-DTNTN_DOWNLOAD_DEPS=OFF` option to cmake when generating the project/makefiles.
If you do so, you have to download dependencies yourself and also pass the `TNTN_LIBGLM_SOURCE_DIR` and `TNTN_LIBFMT_SOURCE_DIR` variables to cmake
as well as `TNTN_CATCH2_SOURCE_DIR` if you want to run the tests.

See [download-deps.cmake](download-deps.cmake) for detailed version info

# Prerequisites

Building and running TIN Terrain requires the packages below to be installed on your system:

* The C++ standard library from the compiler (tested with `clang` `9.1.0` and `gcc` `7.3.0`)
* Boost (BSL-1.0) `program_options`, `filesystem`, `system` (tested with version `1.67`)
* GDAL (MIT) `gdal`, `gdal_priv`, `cpl_conv` (tested with version `2.3`)

### Building on Mac OS
1. Install dependencies, preferably with homebrew:
    ```
    brew install boost
    brew install cmake
    brew install gdal
    ```
2. Create an XCode project
    ```
    mkdir build-cmake-xcode
    cd build-cmake-xcode
    cmake -GXcode path/to/sourcecode/
    open tin-terrain.xcodeproj
    ```
3. Build the TIN Terrain target.
    The resulting binaries will be in the Debug/Release subdirectory.
    To run the tests, build and run the tntn-tests target.

### Building on Linux
1. Install dependencies, e.g. on Ubuntu:
    ```
    apt-get install build-essential cmake libboost-all-dev libgdal-dev
    ```
2. Create Makefile
    ```
    mkdir build-cmake-release
    cd build-cmake-release
    cmake -DCMAKE_BUILD_TYPE=Release path/to/sourcecode/
    ```
3. Build the `tin-terrain` target
    ```
    VERBOSE=1 make tin-terrain
    ```
    The resulting binary should then be ready.
    To run the tests, build and run the tntn-tests target.




### License

Copyright (C) 2018 HERE Europe B.V.

See the [LICENSE](LICENSE) file in the root of this project for license details.
