cmake_minimum_required(VERSION 2.8.11)
project(hpgvis)

#
#
# include directories
#
#
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/hpgv
    ${CMAKE_SOURCE_DIR}/hpgv/oo
    ${CMAKE_SOURCE_DIR}/demo
    ${CMAKE_SOURCE_DIR}/io)

#
#
# MPI
#
#
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

#
#
# OpenMP
#
#
find_package(OpenMP REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

#
#
# hpgv
#
#
add_subdirectory(hpgv)

#
#
# test
#
#
enable_testing()
add_subdirectory(test)

#
#
# demo
#
#
#add_subdirectory(demo)

#
#
# viewer
#
#
add_subdirectory(viewer)
