#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec2 vTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    // vec3 lightPos = vec3(1.0, 1.0, 1.0);
    vec3 texCoolor = texture(texSampler, vTexCoord).rgb;
    // vec3 normal = normalize(vNormal);
    // vec3 lightDir = normalize(lightPos - vNormal);
    // float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = texCoolor;
    outColor = vec4(pow(diffuse, vec3(1.0)), 1.0);
}
