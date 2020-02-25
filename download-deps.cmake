
function(file_touch FILENAME)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E touch "${FILENAME}"
    )
endfunction()

function(file_mkdir DIRNAME)
    file(MAKE_DIRECTORY "${DIRNAME}")
endfunction()


function(string_ends_with STR ENDING RESULT_VARIABLE_NAME)
    string(LENGTH "${STR}" STR_LEN)
    string(LENGTH "${ENDING}" ENDING_LEN)
    if(STR_LEN LESS ENDING_LEN)
        set(${RESULT_VARIABLE_NAME} FALSE PARENT_SCOPE)
        return()
    endif()

    math(EXPR BEGIN "${STR_LEN}-${ENDING_LEN}")
    string(SUBSTRING "${STR}" ${BEGIN} ${ENDING_LEN} STR_ENDING)
    if(STR_ENDING STREQUAL ENDING)
        set(${RESULT_VARIABLE_NAME} TRUE PARENT_SCOPE)
        return()
    else()
        set(${RESULT_VARIABLE_NAME} FALSE PARENT_SCOPE)
        return()
    endif()

endfunction()

function(string_rot13 INPUT_STR RESULT_VARIABLE_NAME)
    set(ALPHABET   "a;b;c;d;e;f;g;h;i;j;k;l;m;n;o;p;q;r;s;t;u;v;w;x;y;z;A;B;C;D;E;F;G;H;I;J;K;L;M;N;O;P;Q;R;S;T;U;V;W;X;Y;Z")
    set(TRANSLATED "n;o;p;q;r;s;t;u;v;w;x;y;z;a;b;c;d;e;f;g;h;i;j;k;l;m;N;O;P;Q;R;S;T;U;V;W;X;Y;Z;A;B;C;D;E;F;G;H;I;J;K;L;M")
    set(OUT "")
    string(LENGTH "${INPUT_STR}" INPUT_STR_LEN)
    foreach(INDEX RANGE 1 ${INPUT_STR_LEN})
        math(EXPR INDEX "${INDEX}-1")
        string(SUBSTRING "${INPUT_STR}" ${INDEX} 1 INPUT_CHAR)
        list(FIND ALPHABET "${INPUT_CHAR}" ALPHABET_POSITION)
        if(ALPHABET_POSITION EQUAL -1)
            set(OUTPUT_CHAR "${INPUT_CHAR}")
        else()
            list(GET TRANSLATED ${ALPHABET_POSITION} OUTPUT_CHAR)
        endif()
        set(OUT "${OUT}${OUTPUT_CHAR}")
    endforeach()
    set(${RESULT_VARIABLE_NAME} "${OUT}" PARENT_SCOPE)
endfunction(string_rot13)

function(download_verify_unpack URL DOWNLOAD_FILENAME SHA512HASH)
    if((DEFINED TNTN_DOWNLOAD_DEPS) AND (NOT TNTN_DOWNLOAD_DEPS))
        message(STATUS "dependency download disabled, not doing anything")
        return()
    endif()

    string(SUBSTRING "${SHA512HASH}" 0 32 SHORTHASH)
    set(FINISHED_TOUCHFILENAME "${DOWNLOAD_FILENAME}.${SHORTHASH}.finished")

    if(EXISTS "${FINISHED_TOUCHFILENAME}")
        message(STATUS "file ${DOWNLOAD_FILENAME} already downloaded and unpacked, not doing anything")
        return()
    endif()

    get_filename_component(DOWNLOAD_FILENAME_DIR "${DOWNLOAD_FILENAME}" DIRECTORY)
    get_filename_component(DOWNLOAD_FILENAME_NAME "${DOWNLOAD_FILENAME}" NAME)

    if(NOT EXISTS "${DOWNLOAD_FILENAME_DIR}")
        file_mkdir("${DOWNLOAD_FILENAME_DIR}")
    endif()

    if(NOT EXISTS "${DOWNLOAD_FILENAME}")
        message(STATUS "downloading ${URL} to ${DOWNLOAD_FILENAME}...")
        file(DOWNLOAD "${URL}" "${DOWNLOAD_FILENAME}"
            TLS_VERIFY ON
            SHOW_PROGRESS
            STATUS DOWNLOAD_STATUS
        )
        list(GET DOWNLOAD_STATUS 0 DOWNLOAD_STATUS_CODE)
        list(GET DOWNLOAD_STATUS 1 DOWNLOAD_STATUS_MSG)
        if(NOT DOWNLOAD_STATUS_CODE EQUAL 0)
            file(REMOVE "${DOWNLOAD_FILENAME}")
            message(FATAL_ERROR "error downloading, code is ${DOWNLOAD_STATUS_CODE} with msg: ${DOWNLOAD_STATUS_MSG}")
        endif()
    else()
        message(STATUS "${DOWNLOAD_FILENAME} already downloaded")
    endif()

    if(NOT "${SHA512HASH}" STREQUAL "NOHASHCHECK")
        message(STATUS "checking hashes of downloaded file...")
        file(SHA512 "${DOWNLOAD_FILENAME}" DOWNLOAD_FILE_SHA512)
        if(NOT "${DOWNLOAD_FILE_SHA512}" STREQUAL "${SHA512HASH}")
            message(FATAL_ERROR "the sha512 hash of downloaded file ${DOWNLOAD_FILENAME} doesn't match the expected hash")
            return()
        endif()
    endif()


    string_ends_with("${DOWNLOAD_FILENAME}" ".tar.gz" FILENAME_IS_TAR_GZ)
    string_ends_with("${DOWNLOAD_FILENAME}" ".tar.gz" FILENAME_IS_TGZ)
    string_ends_with("${DOWNLOAD_FILENAME}" ".zip" FILENAME_IS_ZIP)

    if(FILENAME_IS_TAR_GZ OR FILENAME_IS_TGZ OR FILENAME_IS_ZIP)
        message(STATUS "unpacking downloaded file in ${DOWNLOAD_FILENAME_DIR}...")
        execute_process(
            WORKING_DIRECTORY "${DOWNLOAD_FILENAME_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E tar -xzf "${DOWNLOAD_FILENAME_NAME}"
            RESULT_VARIABLE UNPACK_ERROR_CODE
        )

        if(NOT UNPACK_ERROR_CODE EQUAL 0)
            message(STATUS "error unpacking ${DOWNLOAD_FILENAME}, errno = ${UNPACK_ERROR_CODE}")
            return()
        endif()
    endif()


    file_touch("${FINISHED_TOUCHFILENAME}")
endfunction()


download_verify_unpack(
    "https://github.com/fmtlib/fmt/archive/5.1.0.tar.gz"
    "${CMAKE_SOURCE_DIR}/3rdparty/libfmt-5.1.0.tar.gz"
    "b759a718353254fa8cd981e483bf01a45af0fc76901216404ace5e47f5d3edf43d42422184e5413c221e49832322fdf60d1860e8ec87349c674511064b31e5d6"
)

download_verify_unpack(
    "https://github.com/g-truc/glm/archive/0.9.9.0.tar.gz"
    "${CMAKE_SOURCE_DIR}/3rdparty/libglm-0.9.9.0.tar.gz"
    "b7a6996cb98bc334130c33a339275b50ea28d8dfce300f3d14ac52edf0b5c38bf216d318f411e898edef428876c0b2d1f6a6e951f160f31425fe0852ad150421"
)

download_verify_unpack(
    "https://github.com/catchorg/Catch2/releases/download/v2.3.0/catch.hpp"
    "${CMAKE_SOURCE_DIR}/3rdparty/Catch2-2.3.0/catch.hpp"
    "62716213504195f0482f2aeb1fbdbc20dd96b5d7d3024bc68eaf19a3ba218d3a359163d17d5b6dd93ecc6c2a68573d9fdf9fae596f5f44bb6bab4b0598ad2790"
)

if(TNTN_TEST)
    #see also http://oe.oregonexplorer.info/craterlake/
    download_verify_unpack(
        "http://oe.oregonexplorer.info/craterlake/products/dem/dems_10m.zip"
        "${CMAKE_SOURCE_DIR}/3rdparty/craterlake/dems_10m.zip"
        "c677ac64dd3e443edebf0cdbb8e8785eb2d1dee864d1011b00bd0ef1745c72e4305f81d6630db240a31205d3b7ebed2dd0d52f468c0222013d6d79312a830ae0"
    )

    #see also https://data.gov.uk/dataset/lidar-composite-dsm-50cm1
    #data is in EPSG:27700 aka OSGB 1936 / British National Grid
    #look at tile tq3389_DSM_50CM.asc - Tottenham for an interesting city DSM
    download_verify_unpack(
        "https://environment.data.gov.uk/UserDownloads/interactive/dcbcae4faa49490186edaa3b991025f549808/LIDARCOMP/LIDAR-DSM-50CM-TQ38nw.zip"
        "${CMAKE_SOURCE_DIR}/3rdparty/uk-lidar-composite/LIDAR-DSM-50CM-TQ38nw.zip"
        "f0c8f1cfbffba35122f1be6ba77df15a1f0c666cf4d49b44410bc9ea782cde46c1c24d9e0936aaf477d6a717cfcea64ebc8421155664929cb828e9806748c16b"
    )
endif()

#https://github.com/boostorg/filesystem/archive/boost-1.68.0.tar.gz
#https://github.com/boostorg/program_options/archive/boost-1.68.0.tar.gz
#https://github.com/boostorg/system/archive/boost-1.68.0.tar.gz
