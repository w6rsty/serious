#pragma once
#include <string_view>
#include <vector>

namespace serious
{

struct Settings
{
    unsigned int width = 1200;
    unsigned int height = 600;
    bool validation = false;
    bool vsync = false;
};

using SEShaderIdx = size_t;

enum class SEShaderStage
{
    Vertex,
    Fragment,
    Compute,
};

struct SEShaderDescription
{
    std::string_view file;
    std::string_view entry = "main";
    SEShaderStage stage;
};

struct SEPipeline {};

enum class SEGraphicsAPI
{
    None,
    Vulkan
};

class RHI
{
public:
    virtual ~RHI() = default;
    virtual void Init(void* window) = 0;
    virtual void Shutdown() = 0;
    virtual void PrepareFrame() = 0;
    virtual void SubmitFrame() = 0;
    virtual void Update() {};
    // RHI implementation maintains a structure to hold shader handles
    // and is responsible for creating and destroying them
    virtual SEShaderIdx CreateShader(SEShaderDescription description) = 0;
    virtual SEPipeline* CreatePipeline(const std::vector<SEShaderIdx>& shaders) = 0;
    virtual void BindPipeline(SEPipeline* pipeline) = 0;
    virtual void DestroyPipeline(SEPipeline* pipeline) = 0;

    inline static void SetAPI(SEGraphicsAPI api) { s_API = api; }
protected:
    virtual void WindowResize() = 0; 
    inline static SEGraphicsAPI s_API = SEGraphicsAPI::None;
};

}
