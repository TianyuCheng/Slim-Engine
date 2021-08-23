compile_glsl(raytracing_glsl_shaders shaders/)
add_executable(raytracing main.cpp)
target_link_libraries(raytracing PRIVATE slim)
add_dependencies(raytracing raytracing_glsl_shaders)
