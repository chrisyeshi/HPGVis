cmake_minimum_required(VERSION 2.8)
project(viewer)

#
#
# Qt
#
#
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(OpenGL REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5OpenGL REQUIRED)

#
#
# hpgv
#
#
link_directories(${CMAKE_BINARY_DIR}/hpgv)

#
#
# SOURCES
#
#
set(SHADERS shaders/raf.vert shaders/raf.frag shaders/simple.vert shaders/simple.frag)
set(SOURCES
        TF.cpp
        TFEditor.cpp
        main.cpp
        mainwindow.cpp
        mainwindow.ui
        viewer.cpp)

#
#
# Qt Resources
#
#
qt5_add_resources(SHADERS res.qrc)

#
#
# Executable
#
#
add_executable(viewer ${SOURCES} ${SHADERS})
qt5_use_modules(viewer Core Widgets OpenGL)
qt5_wrap_ui(ui_mainwindow.h mainwindow.ui)
target_link_libraries(viewer hpgv ${HDF5_LIBRARIES} ${OPENGL_LIBRARIES})
