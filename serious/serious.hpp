#pragma once

#include "serious/context.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <memory>


namespace serious {

class Application {
public:
    static void Init(std::vector<const char*> extensions, WindowOptions options);
    static void Quit();
    static void Update();
private:
    static inline std::vector<const char*> surface_extensions_;
    static inline WindowOptions options_;
    static inline std::unique_ptr<Context> vk_context_ = nullptr;
};

}
