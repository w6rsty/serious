#pragma once

#include "serious/RHI.hpp"
#include "serious/VulkanUtils.hpp"

namespace serious
{

class VulkanInstance;
class VulkanWindow;
class VulkanSwapchain;
class VulkanDevice;
class VulkanRenderer;

class VulkanRHI final : public RHI
{
public:
    VulkanRHI();
    virtual ~VulkanRHI() = default;
    virtual void Init(SurfaceCreateFunc&& surfaceCreateFunc, GetWindowSpecFunc&& getWindowSizeFunc) override;
    virtual void Shutdown() override;
    virtual void Update() override;
private:
    Ref<VulkanInstance> m_Instance;
    Ref<VulkanWindow> m_Window;
    Ref<VulkanDevice> m_Device;
    Ref<VulkanSwapchain> m_Swapchain;
    Ref<VulkanRenderer> m_Renderer;
};

}