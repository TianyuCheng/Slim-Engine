# compile shaders
add_glsl(OUTPUT   shaders/main.spv
         VERTEX   shaders/simple.vert
         FRAGMENT shaders/simple.frag
         VERSION  vulkan1.0)

# compile target
add_executable(descriptorindexing main.cpp shaders/main.spv)
target_link_libraries(descriptorindexing PRIVATE slim)
