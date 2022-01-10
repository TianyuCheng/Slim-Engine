# compile shaders
add_glsl(OUTPUT  shaders/main.spv
         COMPUTE shaders/simple.comp
         VERSION vulkan1.0)

# compile target
add_executable(compute main.cpp shaders/main.spv)
target_link_libraries(compute PRIVATE slim)
