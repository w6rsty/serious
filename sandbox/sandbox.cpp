#include "serious/vulkan/VulkanRHI.hpp"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

struct AppState
{
    SDL_Window* window;
    serious::RHI* rhi;
    serious::Settings settings = {};
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    *appstate = new AppState;
    AppState& state = *static_cast<AppState*>(*appstate);
    SDL_WindowFlags flags = SDL_WINDOW_VULKAN;
    state.window = SDL_CreateWindow("serious", state.settings.width, state.settings.height, flags);
    if (!state.window) {
        SDL_Log("failed to create window");
        return SDL_APP_FAILURE;
    }
    state.rhi = new serious::VulkanRHI(state.settings);
    state.rhi->Init(state.window);

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