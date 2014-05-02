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

#
#
# copy the parameter file
#
#
file(COPY supernova_600.hp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY par.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})