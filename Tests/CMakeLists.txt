add_slim_project(
    TARGET test_basics
    SOURCES basics.cpp common.h common.cpp
    SPV vulkan1.0)
target_link_libraries(test_basics PRIVATE gtest)
target_include_directories(test_basics PRIVATE gtest)

add_slim_project(
    TARGET test_compute
    SOURCES compute.cpp common.h common.cpp
    SHADERS shaders/simple.comp
    SPV vulkan1.0)
target_link_libraries(test_compute PRIVATE gtest)
target_include_directories(test_compute PRIVATE gtest)

add_slim_project(
    TARGET test_graphics
    SOURCES graphics.cpp common.h common.cpp
    SHADERS shaders/simple.frag shaders/simple.vert
    SPV vulkan1.0)
target_link_libraries(test_graphics PRIVATE gtest)
target_include_directories(test_graphics PRIVATE gtest)
