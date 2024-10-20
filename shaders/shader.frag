#version 450

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec3 texColor = texture(texSampler, inTexCoord).rgb;
    outColor = vec4(mix(inColor, texColor, 0.5), 1.0);
}
