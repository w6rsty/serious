// grid shader
// https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8#fce6

#version 450

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vTexCoord;

layout(location = 0) out vec4 outColor;

const float lineWidth = 0.04;
const float strengthAA = 5.0;

void main()
{
    vec2 uv = vPosition.xz;

    vec4 uvDDXY = vec4(dFdx(uv), dFdy(uv));
    vec2 uvDeriv = vec2(length(uvDDXY.xz), length(uvDDXY.yw));
    vec2 drawWidth = clamp(vec2(lineWidth), uvDeriv, vec2(0.5));
    vec2 lineAA = uvDeriv * strengthAA;
    vec2 gridUV = 1.0 - abs(fract(uv) * 2.0 - 1.0);
    vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV) * clamp(lineWidth / drawWidth, 0.0, 1.0);
    float grid = mix(grid2.x, 1.0, grid2.y);

    outColor = vec4(vec3(grid), 1.0);
}
