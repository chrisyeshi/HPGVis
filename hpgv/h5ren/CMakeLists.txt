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
add_executable(h5ren main.cpp h5reader.cpp)
target_link_libraries(h5ren hpgv ${HDF5_LIBRARIES})

#
#
# copy the parameter file
#
#
file(COPY h5renpar.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#
#
# Install
#
#
install(TARGETS h5ren DESTINATION bin)
install(FILES h5renpar.sh DESTINATION bin)