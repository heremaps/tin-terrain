cmake_minimum_required(VERSION 3.0)

project(tin-terrain)

option(TNTN_TEST "include test targets in the buildsystem" OFF)
option(TNTN_DOWNLOAD_DEPS "download dependencies during cmake configure" ON)
#option(TNTN_USE_ADDONS "" OFF)

set(CMAKE_CXX_STANDARD 14)

if(MSVC)
    if(MSVC_VERSION LESS 1910)
        message(FATAL_ERROR "Visual Studio 2017 version 15.0 or higher is required")
    endif()
endif()

# get the current working branch
execute_process(
    COMMAND git describe --all --dirty
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE TNTN_GIT_DESCRIPTION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# last commit hash
execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE TNTN_GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(TIMESTAMP TNTN_TIMESTAMP "%Y%m%dT%H%MZ" UTC)


find_package(GDAL REQUIRED)
if(NOT GDAL_FOUND)
    message(FATAL_ERROR "GDAL not found, cannot proceed")
endif()

if(GDAL_VERSION)
    set(SYSTEM_GDAL_VERSION ${GDAL_VERSION})
else()
    if(NOT GDAL_CONFIG)
        message(FATAL_ERROR "gdal-config command not found (not in PATH?), cannot proceed")
    endif()

    execute_process(
      COMMAND ${GDAL_CONFIG} --version
      OUTPUT_VARIABLE SYSTEM_GDAL_VERSION
    )
endif()

if(SYSTEM_GDAL_VERSION VERSION_LESS "2.2")
    message(FATAL_ERROR "GDAL version \"${SYSTEM_GDAL_VERSION}\" is too old, at least 2.2 is required")
endif()

# Boost
set(Boost_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS
    program_options
    filesystem
    system
)

include("download-deps.cmake")
find_path(TNTN_LIBGLM_SOURCE_DIR NAMES "glm/glm.hpp" HINTS "${CMAKE_SOURCE_DIR}/3rdparty/glm-0.9.9.0/")
find_path(TNTN_LIBFMT_SOURCE_DIR NAMES "include/fmt/format.h" HINTS "${CMAKE_SOURCE_DIR}/3rdparty/fmt-5.1.0/")
find_path(TNTN_CATCH2_SOURCE_DIR NAMES "catch.hpp" HINTS "${CMAKE_SOURCE_DIR}/3rdparty/Catch2-2.3.0/")

if(NOT TNTN_LIBGLM_SOURCE_DIR)
    message(SEND_ERROR "GLM math library source dir not found, please download and set TNTN_LIBGLM_SOURCE_DIR")
endif()
if(NOT TNTN_LIBFMT_SOURCE_DIR)
    message(SEND_ERROR "libfmt string formatting library source dir not found, please download and set TNTN_LIBFMT_SOURCE_DIR")
endif()

add_subdirectory("${TNTN_LIBFMT_SOURCE_DIR}")


set(TNTN_SOURCE_FILES

    src/dem2tintiles_workflow.cpp
    include/tntn/dem2tintiles_workflow.h
    
    src/TileMaker.cpp
    include/tntn/TileMaker.h
    
    src/MercatorProjection.cpp
    include/tntn/MercatorProjection.h
    
    include/tntn/MeshMode.h
    include/tntn/Mesh.h
    src/Mesh.cpp

    include/tntn/QuadEdge.h
    src/QuadEdge.cpp

    include/tntn/DelaunayMesh.h
    src/DelaunayMesh.cpp

    include/tntn/DelaunayTriangle.h
    src/DelaunayTriangle.cpp

    include/tntn/TerraMesh.h
    src/TerraMesh.cpp

    include/tntn/TerraUtils.h
    src/TerraUtils.cpp
    
    include/tntn/terra_meshing.h
    src/terra_meshing.cpp

    include/tntn/zemlya_meshing.h
    src/zemlya_meshing.cpp

    include/tntn/MeshIO.h
    src/MeshIO.cpp
    
    include/tntn/geometrix.h
    src/geometrix.cpp
    
    include/tntn/SurfacePoints.h
    src/SurfacePoints.cpp
    
    include/tntn/ZoomRange.h
    
    include/tntn/util.h
    src/util.cpp
    
    include/tntn/logging.h
    src/logging.cpp

    include/tntn/OFFReader.h
    src/OFFReader.cpp

    include/tntn/Raster.h

    include/tntn/RasterIO.h
    src/RasterIO.cpp    

    include/tntn/Mesh2Raster.h
    src/Mesh2Raster.cpp

    include/tntn/SuperTriangle.h
    src/SuperTriangle.cpp

    include/tntn/raster_tools.h
    src/raster_tools.cpp

    include/tntn/FileFormat.h
    
    include/tntn/tntn_assert.h
    
    include/tntn/File.h
    src/File.cpp

    include/tntn/QuantizedMeshIO.h
    src/QuantizedMeshIO.cpp

    include/tntn/MeshWriter.h
    src/MeshWriter.cpp

    include/tntn/ObjPool.h
    
    include/tntn/benchmark_workflow.h
    src/benchmark_workflow.cpp

    include/tntn/simple_meshing.h
    src/simple_meshing.cpp

    include/tntn/Points2Mesh.h
    src/Points2Mesh.cpp
    
    include/tntn/endianness.h
    include/tntn/BinaryIO.h    
    src/BinaryIO.cpp
    
    include/tntn/version_info.h
    src/version_info.cpp.in
    "${CMAKE_CURRENT_BINARY_DIR}/version_info.cpp"

    include/tntn/RasterOverviews.h
    src/RasterOverviews.cpp
    
    include/tntn/gdal_init.h
    src/gdal_init.cpp

    include/delaunator_cpp/Delaunator.h
    src/Delaunator.cpp    
    
    include/tntn/println.h
    src/println.cpp

    include/tntn/ZemlyaMesh.h
    src/ZemlyaMesh.cpp
)

if(TNTN_USE_ADDONS)
    list(APPEND TNTN_SOURCE_FILES
        include/tntn/Raster2Mesh.h
        src/Raster2Mesh.cpp
    )
endif()

configure_file(src/version_info.cpp.in version_info.cpp)


add_library(tntn STATIC
    ${TNTN_SOURCE_FILES}
)

target_compile_definitions(tntn PUBLIC $<$<CONFIG:Debug>:TNTN_DEBUG>)

target_compile_definitions(tntn PUBLIC GLM_FORCE_SWIZZLE GLM_ENABLE_EXPERIMENTAL)

if(TNTN_DOWNLOAD_ADDONS)
    target_compile_definitions(tntn PUBLIC TNTN_USE_ADDONS=1)
endif()

target_include_directories(tntn
    PUBLIC
    ${TNTN_LIBGLM_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
    ${Boost_INCLUDE_DIRS}    
    
    PRIVATE
    ${GDAL_INCLUDE_DIR}
)
if(TNTN_USE_ADDONS)
    target_include_directories(tntn
        PUBLIC
        3rdparty/addons/include/
    )
endif()

target_link_libraries(tntn
    PUBLIC
    ${Boost_LIBRARIES}    
    fmt
    
    PRIVATE
    ${GDAL_LIBRARY}
)

add_executable(tin-terrain
    src/cmd.cpp
)
target_include_directories(tin-terrain
    PRIVATE
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries(tin-terrain
    PRIVATE
    ${Boost_LIBRARIES}
    tntn
)

if(TNTN_TEST)
    add_subdirectory(test)
endif()
