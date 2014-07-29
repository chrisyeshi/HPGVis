#
#
# gather source files
#
#
set (SOURCES
        hpgv_block.c
        hpgv_composite.c
        hpgv_gl.c
        hpgv_tf.c
        hpgv_render.cpp
        hpgv_socket.c
        hpgv_util.c
        hpgv_utilmath.c
        oo/parameter.cpp
        oo/hpgvis.cpp
        oo/image.cpp
        oo/imageraf.cpp
        oo/imagergba.cpp
        oo/volume.h
        ${CMAKE_SOURCE_DIR}/jsoncpp/jsoncpp.cpp)

#
#
# add library
#
#
add_library (hpgv STATIC ${SOURCES})
target_link_libraries (hpgv ${MPI_LIBRARIES})

#
#
# More MPI
#
#
if (MPI_COMPILE_FLAGS)
	set_target_properties (hpgv PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif (MPI_COMPILE_FLAGS)
if (MPI_LINK_FLAGS)
	set_target_properties (hpgv PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
endif (MPI_LINK_FLAGS)
