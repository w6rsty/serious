#pragma once
#include "serious/graphics/Objects.hpp"
#include "serious/graphics/Camera.hpp"

namespace serious
{

class RHI
{
public:
    virtual ~RHI() = default;
    virtual void Init(void* window) = 0;
    // Must call at the before rendering loop
    virtual bool AssureResource() { return false; };
    virtual void Shutdown() = 0;
    virtual void PrepareFrame() = 0;
    virtual void SubmitFrame() = 0;
    virtual void Update() {};
    // RHI implementation maintains a structure to hold shader handles
    // and is responsible for creating and destroying them
    virtual RHIResourceIdx CreateShader(const ShaderDescription& description) = 0;
    virtual RHIResource CreatePipeline(const PipelineDescription& description) = 0;
    virtual RHIResourceIdx CreateBuffer(const BufferDescription& decription) = 0;
    virtual void BindPipeline(RHIResource pipeline) = 0;
    virtual void DestroyPipeline(RHIResource pipeline) = 0;

    virtual Camera& GetCamera() = 0;
    
    virtual void SetClearColor(float r, float g, float b, float a) = 0;
    virtual void SetClearDepth(float depth) = 0;

    virtual void SetPasses(const std::vector<RenderPassDescription>& descriptions) = 0;

    inline static void SetAPI(GraphicsAPI api) { s_API = api; }
protected:
    virtual void WindowResize() = 0; 
    inline static GraphicsAPI s_API = GraphicsAPI::None;
};

}
