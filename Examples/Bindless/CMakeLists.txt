compile_glsl(bindless_glsl_shaders shaders/)
add_executable(bindless main.cpp)
target_link_libraries(bindless PRIVATE slim)
add_dependencies(bindless bindless_glsl_shaders)
