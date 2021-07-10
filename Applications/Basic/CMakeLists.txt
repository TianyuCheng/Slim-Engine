compile_glsl(GLSLShaders shaders/)

add_executable(example example.cpp Time.h Time.cpp)
target_link_libraries(example PRIVATE slim)
add_dependencies(example GLSLShaders)
