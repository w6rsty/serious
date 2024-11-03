#pragma once
#include <string_view>

namespace serious
{

using RHIResource = void*;

struct Settings
{
    unsigned int width = 800;
    unsigned int height = 600;
    bool validation = false;
    bool vsync = false;
};

using RHIResourceIdx = size_t;

enum class ShaderStage
{
    Vertex,
    Fragment,
    Compute,
};

struct ShaderDescription
{
    std::string_view file;
    std::string_view entry = "main";
    ShaderStage stage;
};

enum class BufferUsage
{
    Vertex,
    Index,
    Uniform
};

struct BufferDescription
{
    BufferUsage usage;
    size_t size;
    void* data;
};

struct RenderPassDescription
{
    RHIResource pipeline;
    RHIResourceIdx vertexBuffer;
    RHIResourceIdx indexBuffer;
    uint32_t size;
};

enum class GraphicsAPI
{
    None,
    Vulkan
};

}