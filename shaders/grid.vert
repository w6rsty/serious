#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 vPosition;
layout(location = 1) out vec2 vTexCoord;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    vec4 pos = vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * ubo.model * pos;
    vPosition = vec3(ubo.model * pos);
    vTexCoord = inTexCoord;
}