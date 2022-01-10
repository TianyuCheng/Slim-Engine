# compile shaders
add_glsl(OUTPUT   shaders/matcap.spv
         VERTEX   shaders/matcap.vert
         FRAGMENT shaders/matcap.frag
         VERSION  vulkan1.0)

# compile target
add_executable(matcap main.cpp shaders/matcap.spv)
target_link_libraries(matcap PRIVATE slim)
