# compile shaders
add_glsl(OUTPUT   shaders/main.spv
         VERTEX   shaders/direction.vert
         FRAGMENT shaders/direction.frag
         VERSION  vulkan1.0)
add_glsl(OUTPUT   shaders/generate.spv
         COMPUTE  shaders/generate.comp
         INCLUDE  ${PROJECT_SOURCE_DIR}/Library/shaderlib
         VERSION  vulkan1.0)

# compile target
add_executable(hemisphere main.cpp
    shaders/main.spv shaders/generate.spv)
target_link_libraries(hemisphere PRIVATE slim)
