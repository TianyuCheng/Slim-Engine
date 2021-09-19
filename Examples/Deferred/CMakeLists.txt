compile_glsl(deferred_glsl_shaders shaders/)
add_executable(deferred main.cpp fairy.h scene.h)
target_link_libraries(deferred PRIVATE slim)
add_dependencies(deferred deferred_glsl_shaders)
