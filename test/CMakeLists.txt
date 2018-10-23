find_package(Boost REQUIRED COMPONENTS
    filesystem
    system
)

find_file(TNTN_TEST_CRATERLAKE_DEMS_10M_DEM
    "dems_10m.dem"
    PATHS "${CMAKE_SOURCE_DIR}/3rdparty/craterlake"
)
find_file(TNTN_TEST_UKLIDAR_TOTTENHAM
    "tq3389_DSM_50CM.asc"
    PATHS "${CMAKE_SOURCE_DIR}/3rdparty/uk-lidar-composite"    
)

add_executable(tntn-tests

    src/main.cpp
    
    #common
    src/test_common.cpp
    src/test_common.h

    #tests
    src/Mesh_tests.cpp
    src/geometrix_tests.cpp
    src/SurfacePoints_tests.cpp
    src/SimpleRange_tests.cpp
    src/Raster_tests.cpp
    src/SuperTriangle_tests.cpp
    src/terra_meshing_tests.cpp
    src/File_tests.cpp
    src/OFFReader_tests.cpp
    src/ObjPool_tests.cpp
    src/Delaunay_tests.cpp
    src/util_tests.cpp
    src/QuantizedMeshIO_tests.cpp
    src/simple_meshing_tests.cpp
    src/println_tests.cpp
    src/raster_tools_tests.cpp
	src/RasterIO_tests.cpp
    src/RasterOverviews_tests.cpp

	#data
    src/vertex_points.cpp
    src/vertex_points.h
    src/triangle_indices.cpp
    src/triangle_indices.h    
)

set (CMAKE_CXX_FLAGS "--std=c++11")

get_filename_component(TNTN_FIXTURES_LOCATION "fixtures" ABSOLUTE)

target_link_libraries(tntn-tests
    PRIVATE
    tntn
    ${Boost_LIBRARIES}
)

if(NOT TNTN_CATCH2_SOURCE_DIR)
    message(SEND_ERROR "Catch2 testing framework source dir not found, please download and set TNTN_CATCH2_SOURCE_DIR")
endif()

if(TNTN_TEST_CRATERLAKE_DEMS_10M_DEM)
    target_compile_definitions(tntn-tests
        PRIVATE
        TNTN_TEST_CRATERLAKE_DEMS_10M_DEM="${TNTN_TEST_CRATERLAKE_DEMS_10M_DEM}"
    )
endif()

if(TNTN_TEST_UKLIDAR_TOTTENHAM)
    target_compile_definitions(tntn-tests
        PRIVATE
        TNTN_TEST_UKLIDAR_TOTTENHAM="${TNTN_TEST_UKLIDAR_TOTTENHAM}"
    )
endif()

target_compile_definitions(tntn-tests
    PRIVATE
    TNTN_FIXTURES_PATH="${TNTN_FIXTURES_LOCATION}"
)

target_include_directories(tntn-tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/
    ${Boost_INCLUDE_DIRS}
    ${TNTN_CATCH2_SOURCE_DIR}
)
