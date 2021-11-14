#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Camera {
    mat4 MVP;
};

layout(location = 0) in vec4 inVertex;

void main() {
    gl_Position = MVP * inVertex;
}
