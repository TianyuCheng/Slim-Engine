# compile shaders
add_glsl(OUTPUT   shaders/decode_hemioct.spv
         VERTEX   shaders/decode_hemioct.vert
         FRAGMENT shaders/decode_hemioct.frag
         VERSION  vulkan1.0)

# compile target
add_executable(octmap main.cpp shaders/decode_hemioct.spv)
target_link_libraries(octmap PRIVATE slim)
