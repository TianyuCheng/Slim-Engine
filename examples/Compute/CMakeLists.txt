compile_glsl(compute_glsl_shaders shaders/)
add_executable(compute compute.cpp)
target_link_libraries(compute PRIVATE slim)
add_dependencies(compute compute_glsl_shaders)
