cmake_minimum_required(VERSION 3.0.0)
project(futures-pipeline-gtkmm VERSION 0.1.0)

set (CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-Wall -Wunused-variable -Wunused-parameter")

include_directories(CMAKE_CURRENT_SOURCE_DIR)
include_directories(../../camera)
include_directories(../../camera/include/linux)
include_directories(../../utils)
include_directories(../../utils/boost_1_74_0)
include_directories(../../utils/queue)
include_directories(../../utils/rapidjson_004e8e6/include)
include_directories(../../utils/rapidjson_004e8e6/include/rapidjson)

# TODO implements target_sources() for distributed source, https://crascit.com/2016/01/31/enhanced-source-file-handling-with-target_sources/
# sources
add_executable(futures-pipeline-gtkmm
    main.cpp
    image-scaled.cpp
    render-ui.cpp
    worker.cpp
    ../../camera/camera.cpp
    ../../camera/camera-capabilities.cpp
    ../../utils/sync-log.cpp
    ../../utils/exec-shell.cpp
)

# pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(futures-pipeline-gtkmm PRIVATE Threads::Threads)

# gtkmm
find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0)
include_directories(${GTKMM_INCLUDE_DIRS})
target_link_libraries(futures-pipeline-gtkmm PRIVATE ${GTKMM_LIBRARIES})

# cmake -D CMAKE_C_COMPILER=gcc-7 -D CMAKE_CXX_COMPILER=g++-7 ..
# cmake -D CMAKE_C_COMPILER=clang-6.0 -D CMAKE_CXX_COMPILER=clang++-6.0 ..