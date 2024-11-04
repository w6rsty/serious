#pragma once
#include "serious/graphics/Objects.hpp"
#include "serious/graphics/vulkan/VulkanRHI.hpp"
#include "serious/geo/StaticMesh.hpp"

#include <memory>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

using namespace serious;

class Application
{
public:
    enum class State
    {
        Pending,
        Continue,
        Quit
    };

    Application()
    {
    }

    ~Application()
    {
        rhi->DestroyPipeline(pipeline);
        rhi->Shutdown();
        SDL_DestroyWindow(window);
        SEInfo("Quit application");
    }

    void SetupWindow()
    {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("serious", settings.width, settings.height, windowFlags);
        if (!window) {
            SEFatal("Failed to create window");
        }
    }

    void SetupGraphics()
    {
        rhi = std::make_unique<VulkanRHI>(settings);
        rhi->SetAPI(GraphicsAPI::Vulkan);
        rhi->Init(window);
        rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        rhi->SetClearDepth(1.0f);

        Camera& camera = rhi->GetCamera();
        camera.SetRotationSpeed(0.1f);

        RHIResourceIdx vertShader = rhi->CreateShader({
            .file  = "D:/w6rsty/dev/Cpp/serious/shaders/grid_vert.spv",
            .stage = ShaderStage::Vertex
        });
        RHIResourceIdx fragShader = rhi->CreateShader({
            .file  = "D:/w6rsty/dev/Cpp/serious/shaders/grid_frag.spv",
            .stage = ShaderStage::Fragment
        });

        // Setup pipeline
        PipelineDescription pipelineDescription = {
            .shaders = {vertShader, fragShader},
            .blendingMode = ColorBlendingMode::AlphaBlending
        };
        pipeline = rhi->CreatePipeline(pipelineDescription);
        rhi->BindPipeline(pipeline);

        RHIResourceIdx vertexBuffer = rhi->CreateBuffer({
            .usage = BufferUsage::Vertex,
            .size  = sizeof(Vertex) * mesh::Plane::vertices.size(),
            .data  = mesh::Plane::vertices.data()
        });
        RHIResourceIdx indexBuffer = rhi->CreateBuffer({
            .usage = BufferUsage::Index,
            .size  = sizeof(uint32_t) * mesh::Plane::indices.size(),
            .data  = mesh::Plane::indices.data()
        });

        RenderPassDescription pass = {
            .pipeline = pipeline,
            .vertexBuffer = vertShader,
            .indexBuffer = indexBuffer,
            .size = (uint32_t)mesh::Plane::indices.size()
        };

        rhi->SetPasses({pass});
    }

    void Run()
    {
        if (!rhi->AssureResource()) {
            SEError("Uncompleted resources");
            return;
        }
        running = true;
        while (running) {
            static auto startTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
            startTime = currentTime;
            if (HandleEvent() == State::Quit) {
                running = false;
                SEInfo("Window closed");
                break;
            }

            Camera& camera = rhi->GetCamera();
            camera.Update(deltaTime);

            rhi->Update();
        }
    }
private:
    State HandleEvent()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running  = false;
                return State::Quit;
            }
            Camera& camera = rhi->GetCamera();
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_W) {
                    camera.keys.forward = true;
                }
                if (event.key.key == SDLK_S) {
                    camera.keys.backward = true;
                }
                if (event.key.key == SDLK_A) {
                    camera.keys.left = true;
                }
                if (event.key.key == SDLK_D) {
                    camera.keys.right = true;
                }
                if (event.key.key == SDLK_SPACE) {
                    camera.keys.up = true;
                }
                if (event.key.key == SDLK_LCTRL) {
                    camera.keys.down = true;
                }
            }
            if (event.type == SDL_EVENT_KEY_UP) {
                if (event.key.key == SDLK_W) {
                    camera.keys.forward = false;
                }
                if (event.key.key == SDLK_S) {
                    camera.keys.backward = false;
                }
                if (event.key.key == SDLK_A) {
                    camera.keys.left = false;
                }
                if (event.key.key == SDLK_D) {
                    camera.keys.right = false;
                }
                if (event.key.key == SDLK_SPACE) {
                    camera.keys.up = false;
                }
                if (event.key.key == SDLK_LCTRL) {
                    camera.keys.down = false;
                }
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    clickx = event.button.x;
                    clicky = event.button.y;
                }
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (event.motion.state & SDL_BUTTON_LMASK) {
                    glm::vec3 delta = glm::vec3(clicky - event.motion.y, event.motion.x - clickx, 0.0f);
                    camera.Rotate(delta);
                    clickx = event.motion.x;
                    clicky = event.motion.y;
                }
            }
        }
        return State::Continue;
    }
private:
    uint64_t windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    Settings settings = {
        .width = 800,
        .height = 600,
        .validation = true,
        .vsync = false,
    };
    SDL_Window* window = nullptr;

    std::unique_ptr<RHI> rhi;
    RHIResource pipeline;
    int clickx, clicky;

    bool running = false;
};