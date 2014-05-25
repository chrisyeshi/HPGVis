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
add_subdirectory(demo)

#
#
# viewer
#
#
add_subdirectory(viewer)

#
#
# controlelr
#
#
#add_subdirectory(controller)
