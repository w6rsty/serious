#include "serious/RHI.hpp"
#include "serious/vulkan/VulkanRHI.hpp"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

struct AppState
{
    SDL_Window* window;
    uint64_t windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    std::unique_ptr<serious::RHI> rhi;
    serious::Settings settings = {
        .width = 800,
        .height = 600,
        .validation = true,
        .vsync = false
    };
    serious::SEPipeline* pipeline;
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    using namespace serious;

    SDL_Init(SDL_INIT_VIDEO);

    *appstate = new AppState;
    AppState* state = static_cast<AppState*>(*appstate);
    state->window = SDL_CreateWindow("serious", state->settings.width, state->settings.height, state->windowFlags);
    if (!state->window) { return SDL_APP_FAILURE; }

    state->rhi = std::make_unique<VulkanRHI>(state->settings);
    state->rhi->SetAPI(SEGraphicsAPI::Vulkan);
    state->rhi->Init(state->window);

    // Setup pipeline
    SEPipeline* pipeline = state->rhi->CreatePipeline({
        state->rhi->CreateShader({
            .file = "shaders/vert.spv",
            .stage = SEShaderStage::Vertex
        }),
        state->rhi->CreateShader({
            .file = "shaders/frag.spv",
            .stage = SEShaderStage::Fragment
        })
    });
    state->rhi->BindPipeline(pipeline);
    state->pipeline = pipeline;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState* state = static_cast<AppState*>(appstate);
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
    if (state) {
        state->rhi->DestroyPipeline(state->pipeline);
        state->rhi->Shutdown();
        SDL_DestroyWindow(state->window);
        delete state;
    }
    VKInfo("Quit application");
} 