cmake_minimum_required(VERSION 2.8.11)
project(hpgvis)

#
#
# Options
#
#
set(BUILD_HPGV ON CACHE BOOL "build hpgv library?")
set(BUILD_VIEWER ON CACHE BOOL "build viewer?")
set(BUILD_TESTS ON CACHE BOOL "build the test suite?")

#
#
# include directories
#
#
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/hpgv
    ${CMAKE_SOURCE_DIR}/hpgv/oo
    ${CMAKE_SOURCE_DIR}/hpgv/demo
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/libs/jsoncpp
    ${CMAKE_SOURCE_DIR}/libs/jsoncpp/json)

#
#
# MPI
#
#
if(BUILD_HPGV)
	find_package(MPI REQUIRED)
	include_directories(${MPI_INCLUDE_PATH})
endif(BUILD_HPGV)

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
if(BUILD_HPGV)
	add_subdirectory(hpgv)
endif(BUILD_HPGV)

#
#
# viewer
#
#
if(BUILD_VIEWER)
	add_subdirectory(viewer)
endif(BUILD_VIEWER)

#
#
# test
#
#
if(BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif(BUILD_TESTS)