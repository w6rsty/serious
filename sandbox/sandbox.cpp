#include "serious/RHI.hpp"
#include "serious/VulkanRHI.hpp"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

struct AppState
{
    SDL_Window* window;
    serious::RHI* rhi;
    serious::WindowSpec spec {1024, 720, false};
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    *appstate = new AppState;
    AppState& state = *static_cast<AppState*>(*appstate);
    SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    state.window = SDL_CreateWindow("serious", state.spec.width, state.spec.height, flags);
    if (!state.window) {
        SDL_Log("failed to create window");
        return SDL_APP_FAILURE;
    }
    state.rhi = new serious::VulkanRHI;
    state.rhi->Init(
        [&state](void* instance) {
            VkSurfaceKHR surface;
            SDL_Vulkan_CreateSurface(state.window, static_cast<VkInstance>(instance), nullptr, &surface);
            return surface;
        },
        [&state]() {
            int width = 0, height = 0;
            SDL_GetWindowSize(state.window, &width, &height);
            return serious::WindowSpec{static_cast<uint32_t>(width), static_cast<uint32_t>(height), state.spec.vsync};
        }
    );

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState& state = *static_cast<AppState*>(appstate);
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState* state = static_cast<AppState*>(appstate);
    state->rhi->Update();
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate)
{
    AppState* state = static_cast<AppState*>(appstate);

    state->rhi->Shutdown();
    delete state->rhi;
    SDL_DestroyWindow(state->window);
    delete state;
}
