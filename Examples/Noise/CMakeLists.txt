# compile shaders
add_hlsl(OUTPUT   shaders/noise.spv
         SOURCE   shaders/noise.hlsl
         VERTEX   vert
         FRAGMENT frag
         VERSION  vulkan1.0)

# compile target
add_executable(noise main.cpp shaders/noise.spv)
target_link_libraries(noise PRIVATE slim)
