#include "serious/context.hpp"
#include "serious/serious.hpp"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

#include <vector>

static SDL_Window *window = nullptr;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("serious", 1024, 720, SDL_WINDOW_VULKAN);
    if (!window) {
        SDL_Log("failed to create window");
        return SDL_APP_FAILURE;
    }

    /// Get SDL surface extensions
    uint32_t count = 0;
    auto required = SDL_Vulkan_GetInstanceExtensions(&count);
    if (count == 0) { return SDL_APP_FAILURE; }
    std::vector<const char*> extensions(required, required + count);

    serious::WindowOptions options;
    options.extent.setWidth(1024);
    options.extent.setHeight(720);
    options.create_surface_callback = [](vk::Instance instance) {
        VkSurfaceKHR surface;
        SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);
        return surface;
    };

    serious::Application::Init(extensions, options);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    serious::Application::Update();
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate)
{
    serious::Application::Quit();
}
