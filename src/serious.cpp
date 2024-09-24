#include "serious/serious.hpp"

namespace serious {

void Application::Init(std::vector<const char*> surface_extensions, WindowOptions options) {
    surface_extensions_ = surface_extensions;
    options_ = options;
    
    vk_context_.reset(new Context);
    vk_context_->Prepare(surface_extensions_, options_);    
}

void Application::Quit() {
    if (vk_context_) {
        vk_context_.reset();
    }
}

void Application::Update() {
    vk_context_->Update();
}

}
