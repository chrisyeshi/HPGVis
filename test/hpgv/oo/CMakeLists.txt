add_executable (test_oo_parameter test_oo_parameter.cpp)
target_link_libraries (test_oo_parameter hpgv)
add_test (test_oo_parameter test_oo_parameter)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/hpgv_render_1408_1080_1100_FLOAT_1024_1024.cfg DESTINATION ${CMAKE_CURRENT_BINARY_DIR})