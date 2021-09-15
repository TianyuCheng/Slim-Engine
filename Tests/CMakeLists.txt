compile_glsl(test_glsl_shaders shaders/)

add_executable(test_basics basics.cpp common.h common.cpp)
target_link_libraries(test_basics PRIVATE gtest slim)
target_include_directories(test_basics PRIVATE gtest slim)
add_dependencies(test_basics test_glsl_shaders)

add_executable(test_compute compute.cpp common.h common.cpp)
target_link_libraries(test_compute PRIVATE gtest slim)
target_include_directories(test_compute PRIVATE gtest slim)
add_dependencies(test_compute test_glsl_shaders)

add_executable(test_graphics graphics.cpp common.h common.cpp)
target_link_libraries(test_graphics PRIVATE gtest slim)
target_include_directories(test_graphics PRIVATE gtest slim)
add_dependencies(test_graphics test_glsl_shaders)
