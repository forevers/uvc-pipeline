cmake_minimum_required(VERSION 3.0.0)
project(futures-pipeline VERSION 0.1.0)

#include(CTest)
#enable_testing()

#target_compile_features(futures-pipeline PRIVATE cxx_std_17)
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-Wall -Wunused-variable -Wunused-parameter")

include_directories(camera)
include_directories(camera/include/linux)
include_directories(utils)
include_directories(utils/boost_1_74_0)
include_directories(utils/queue)
include_directories(utils/rapidjson_004e8e6/include)
include_directories(utils/rapidjson_004e8e6/include/rapidjson)

#clang++ -g -O0 -Wunused-variable -Wunused-parameter -std=c++17 
#-I$(pwd)/camera/include/linux -I./camera -I./utils -I./utils/queue -I./utils/boost_1_74_0 
#-I$(pwd)/utils/rapidjson_004e8e6/include 
#-I./utils/rapidjson_004e8e6/include/rapidjson 
#-pthread -lpthread main.cpp ./camera/camera.cpp ./utils/sync-log.cpp ./utils/exec-shell.cpp

# sources
add_executable(futures-pipeline 
    main.cpp
    camera/camera.cpp
    camera/camera-capabilities.cpp
    utils/sync-log.cpp
    utils/exec-shell.cpp
)

# pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(futures-pipeline PRIVATE Threads::Threads)

#set(CPACK_PROJECT_NAME ${PROJECT_NAME})
#set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
#include(CPack)


# cmake -D CMAKE_C_COMPILER=gcc-7 -D CMAKE_CXX_COMPILER=g++-7 ..
# cmake -D CMAKE_C_COMPILER=clang-6.0 -D CMAKE_CXX_COMPILER=clang++-6.0 ..