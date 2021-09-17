compile_glsl(matcap_glsl_shaders shaders/)
add_executable(matcap main.cpp)
target_link_libraries(matcap PRIVATE slim)
add_dependencies(matcap matcap_glsl_shaders)
