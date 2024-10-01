#pragma once

#include <functional>

namespace serious
{

struct WindowSpec
{
    uint32_t width;
    uint32_t height;
    bool vsync;
};

using SurfaceCreateFunc = std::function<void*(void*)>;
using GetWindowSpecFunc = std::function<WindowSpec()>;

/// gradually replace export functions with RHI interface
class RHI
{
public:
    virtual ~RHI() = default;
    virtual void Init(SurfaceCreateFunc&& surfaceCreateFunc, GetWindowSpecFunc&& getWindowSizeFunc) = 0;
    virtual void Shutdown() = 0;
    virtual void Update() = 0;
};

}
