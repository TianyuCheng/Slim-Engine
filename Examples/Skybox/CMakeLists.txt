# compile shaders
add_glsl(OUTPUT   shaders/common.spv
         VERTEX   shaders/common.vert
         VERSION  vulkan1.0)
add_glsl(OUTPUT   shaders/reflect.spv
         FRAGMENT shaders/reflect.frag
         VERSION  vulkan1.0)
add_glsl(OUTPUT   shaders/refract.spv
         FRAGMENT shaders/refract.frag
         VERSION  vulkan1.0)
add_glsl(OUTPUT   shaders/skybox.spv
         VERTEX   shaders/skybox.vert
         FRAGMENT shaders/skybox.frag
         VERSION  vulkan1.0)

# compile target
add_executable(skybox main.cpp
    shaders/common.spv
    shaders/reflect.spv
    shaders/refract.spv
    shaders/skybox.spv)
target_link_libraries(skybox PRIVATE slim)
