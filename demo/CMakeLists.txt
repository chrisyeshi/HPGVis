#
#
# HDF5
#
#
find_package(HDF5 REQUIRED)
include_directories(${HDF5_INCLUDE_PATH})

#
#
# hpgv
#
#
link_directories(${CMAKE_BINARY_DIR}/hpgv)

#
#
# add executable
#
#
add_executable(demo main.cpp ../io/h5reader.cpp)
target_link_libraries(demo hpgv ${HDF5_LIBRARIES})