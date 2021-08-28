compile_glsl(scenegraph_glsl_shaders shaders/)
add_executable(scenegraph main.cpp)
target_link_libraries(scenegraph PRIVATE slim)
add_dependencies(scenegraph scenegraph_glsl_shaders)
