# compile shaders
add_glsl(OUTPUT   shaders/textured.spv
         VERTEX   shaders/textured.vert
         FRAGMENT shaders/textured.frag
         VERSION  vulkan1.0)

# compile target
add_executable(texturemapping main.cpp shaders/textured.spv)
target_link_libraries(texturemapping PRIVATE slim)
