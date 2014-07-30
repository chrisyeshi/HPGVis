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
        ${CMAKE_SOURCE_DIR}/libs/jsoncpp/jsoncpp.cpp)

set (HEADERS
        hpgv.h
        hpgv_block.h
        hpgv_composite.h
        hpgv_error.h
        hpgv_gl.h
        hpgv_render.h
        hpgv_socket.h
        hpgv_tf.h
        hpgv_util.h
        hpgv_utilmath.h
        hpgv_utiltiming.h
        oo/hpgvis.h
        oo/image.h
        oo/imageraf.h
        oo/imagergba.h
        oo/parameter.h
        oo/volume.h)

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

#
#
# demo
#
#
add_subdirectory(h5ren)

#
#
# Install
#
#
install(TARGETS hpgv DESTINATION libs)
install(FILES ${HEADERS} DESTINATION include)