compile_glsl(hybrid_glsl_shaders shaders/)
add_executable(hybrid main.cpp scene.h)
target_link_libraries(hybrid PRIVATE slim)
add_dependencies(hybrid hybrid_glsl_shaders)
