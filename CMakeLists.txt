cmake_minimum_required(VERSION 3.10)
project(endian-conv)

# C++
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-Wall -O3 -lstdc++ -std=c++14 -fopenmp -D_FILE_OFFSET_BITS=64")

find_package(Boost REQUIRED COMPONENTS filesystem endian)
message(STATUS "Boost version: ${Boost_VERSION}")

set(_ENDCV endian-conv)

file(GLOB_RECURSE _ENDCV_SRC ${CMAKE_SOURCE_DIR}/src/*)
add_executable(${_ENDCV} ${ENDCV_SRC})
add_dependencies(${_ENDCV} PUBLIC Boost::filesystem Boost::endian)
