#include "serious/Application.hpp"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

struct AppState
{
    SDL_Window* window;
    int width = 1024;
    int height = 720;
    bool vsync = true;
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    *appstate = new AppState;
    AppState& state = *static_cast<AppState*>(*appstate);
    SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    state.window = SDL_CreateWindow("serious", state.width, state.height, flags);
    if (!state.window) {
        SDL_Log("failed to create window");
        return SDL_APP_FAILURE;
    }

    serious::Application::Init(
        [&state](VkInstance instance) {
            VkSurfaceKHR surface;
            SDL_Vulkan_CreateSurface(state.window, instance, nullptr, &surface);
            return surface;
        },
        state.width,
        state.height,
        state.vsync,
        [&state]() {
            int width = 0, height = 0;
            SDL_GetWindowSize(state.window, &width, &height);
            return VkExtent2D { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
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
    serious::Application::OnUpdate();
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate)
{
    AppState* state = static_cast<AppState*>(appstate);

    serious::Application::Shutdown();

    SDL_DestroyWindow(state->window);
    delete state;
}
