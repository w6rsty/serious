#pragma once

namespace serious
{

struct Settings
{
    unsigned int width = 1200;
    unsigned int height = 600;
    bool validation = false;
    bool vsync = false;
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
protected:
    virtual void WindowResize() = 0; 
};

}
