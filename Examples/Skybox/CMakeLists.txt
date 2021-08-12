compile_glsl(skybox_glsl_shaders shaders/)
add_executable(skybox main.cpp)
target_link_libraries(skybox PRIVATE slim)
add_dependencies(skybox skybox_glsl_shaders)
