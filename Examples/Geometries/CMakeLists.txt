compile_glsl(geometries_glsl_shaders shaders/)
add_executable(geometries main.cpp)
target_link_libraries(geometries PRIVATE slim)
add_dependencies(geometries geometries_glsl_shaders)
