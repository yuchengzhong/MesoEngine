// Meso Engine 2024
#include "VoxelWindowsInstance.h"
#include <lvk/LVK.h>
#include <lvk/HelpersImGui.h>

#include <GLFW/glfw3.h>

#include <shared/Camera.h>
#include <shared/UtilsFPS.h>
#include <taskflow/taskflow.hpp>

VoxelWindowsInstance::~VoxelWindowsInstance()
{
    if (LVKWindow)
    {
        glfwDestroyWindow(LVKWindow);
    }
    glfwTerminate();
}

void VoxelWindowsInstance::SetImguiFont(const std::string& NewFontPath)
{
    if (NewFontPath != "")
    {
        Fonts = NewFontPath;
    }
}

void VoxelWindowsInstance::Initialize(const VoxelInstanceInitialConfig& InitialConfig)
{
    {
        bLVKEnableValidationLayers = InitialConfig.bEnableValidationLayers;
        bLVKPreferIntegratedGPU = InitialConfig.bPreferIntegratedGPU;
        LVKNumBufferedFrames = InitialConfig.kNumBufferedFrames;
        LVKNumSamplesMSAA = InitialConfig.kNumSamplesMSAA;
        bUseHDRForFinalBuffer = InitialConfig.bUseHDRForFinalBuffer;
        bWindowsResizeable = InitialConfig.bWindowsResizeable;

#ifndef __APPLE__
        LVKNumSamplesMSAA = std::min(LVKNumSamplesMSAA, 8u);
#else
        LVKNumSamplesMSAA = std::min(LVKNumSamplesMSAA, 4u);
#endif

    }
    if (InitialConfig.bInitialFullScreen)
    {
        WindowsWidth = 0;
        WindowsHeight = 0;
    }
    else
    {
        WindowsWidth = InitialConfig.WindowsWidth;
        WindowsHeight = InitialConfig.WindowsHeight;
    }
    //Voxel config
    {
        VoxelSceneConfig = InitialConfig.VoxelSceneConfig;
    }
    SetImguiFont(InitialConfig.FontPath);
    InitializeCameraAndScene(InitialConfig);
    //Rest of the Initialize
    minilog::initialize(nullptr, { .threadNames = false });

    LVKWindow = lvk::initWindow("Meso Voxel Engine", WindowsWidth, WindowsHeight, bWindowsResizeable);
    InitializeContext();
    InitializeBegin();
    InitializeWindowsCallBacks();
    InitializeRenderPass();
    InitializeFrameBuffer();
    InitializePipeline();
    InitializeImgui();
    InitializeRender();
    InitializeFinal();
}

void VoxelWindowsInstance::InitializeCameraAndScene(const VoxelInstanceInitialConfig& InitialConfig)
{
    //Initialize Camera
    WindowsCamera.InitializeVoxelCamera(InitialConfig.CameraFOV);
    //Binding
    WindowsCamera.SetCameraChunkUpdateCallback([this]()
        {
            this->DummyWhenCameraChunkUpdate();
        });
    WindowsCamera.SetCameraUpdateCallback([this]()
        {
            this->DummyWhenCameraUpdate();
        });
    SceneConfig = 
    {
        .BlockSize = InitialConfig.VoxelSceneConfig.BlockSize,
        .BlockResolution = (uint32_t)InitialConfig.VoxelSceneConfig.BlockResolution,
        .ChunkSize = InitialConfig.VoxelSceneConfig.GetChunkSize(),
        .ChunkResolution = (uint32_t)InitialConfig.VoxelSceneConfig.ChunkResolution,
    };
}

void VoxelWindowsInstance::InitializeContext()
{
    LVKContext = lvk::createVulkanContextWithSwapchain(LVKWindow,
        WindowsWidth,
        WindowsHeight,
        {
            .enableValidation = bLVKEnableValidationLayers,
        },
        bLVKPreferIntegratedGPU ? lvk::HWDeviceType_Integrated : lvk::HWDeviceType_Discrete);

    UBOCamera.clear();
    for (uint32_t i = 0; i != LVKNumBufferedFrames; i++)
    {
        UBOCamera.push_back(LVKContext->createBuffer({ .usage = lvk::BufferUsageBits_Uniform,
                                                     .storage = lvk::StorageType_HostVisible,
                                                     .size = sizeof(FGPUUniformCamera),
                                                     .debugName = "Buffer: camera uniforms (per frame)" }, nullptr));
    }
    UBOSceneConfig = LVKContext->createBuffer({ .usage = lvk::BufferUsageBits_Uniform,
                                                 .storage = lvk::StorageType_HostVisible,
                                                 .size = sizeof(FGPUUniformSceneConfig),
                                                 .debugName = "Buffer: voxel scene uniforms (static)" }, nullptr);

    LVKContext->upload(UBOSceneConfig, &SceneConfig, sizeof(SceneConfig));
}

void VoxelWindowsInstance::InitializeBegin()
{
}

void VoxelWindowsInstance::InitializeWindowsCallBacks()
{
    glfwSetWindowUserPointer(LVKWindow, this);
    //
    SubInitializeFramebufferSizeCallback();
    SubInitializeCursorPosCallback();
    SubInitializeMouseButtonCallback();
    SubInitializeScrollCallback();
    SubInitializeKeyCallback();
}

void VoxelWindowsInstance::SubInitializeFramebufferSizeCallback()
{
    //Resize the FB if windows size is changed
    glfwSetFramebufferSizeCallback(LVKWindow, [](GLFWwindow* Window, int Width, int Height)
        {
            VoxelWindowsInstance* Instance = static_cast<VoxelWindowsInstance*>(glfwGetWindowUserPointer(Window));
            Instance->WindowsWidth = Width;
            Instance->WindowsHeight = Height;
            if (!Width || !Height)
            {
                return;
            }
            Instance->LVKContext->recreateSwapchain(Width, Height);
            Instance->CreateWindowsFrameBuffer();
        });
}

void VoxelWindowsInstance::SubInitializeCursorPosCallback()
{
    //Get the mouse position
    glfwSetCursorPosCallback(LVKWindow, [](GLFWwindow* Window, double PositionX, double PositionY)
        {
            VoxelWindowsInstance* Instance = static_cast<VoxelWindowsInstance*>(glfwGetWindowUserPointer(Window));
            int Width, Height;
            glfwGetFramebufferSize(Window, &Width, &Height);
            if (Width && Height)
            {
                Instance->WindowsMousePosition = vec2(PositionX / Width, 1.0f - PositionY / Height);
                ImGui::GetIO().MousePos = ImVec2(PositionX, PositionY);
            }
        });
}

void VoxelWindowsInstance::SubInitializeMouseButtonCallback()
{
    //Get the mouse button action
    glfwSetMouseButtonCallback(LVKWindow, [](GLFWwindow* Window, int Button, int Action, int Mods)
        {
            VoxelWindowsInstance* Instance = static_cast<VoxelWindowsInstance*>(glfwGetWindowUserPointer(Window));
            if (!ImGui::GetIO().WantCaptureMouse)
            {
                if (Button == GLFW_MOUSE_BUTTON_LEFT)
                {
                    Instance->bWindowsLeftMousePressed = (Action == GLFW_PRESS);
                }
                if (Button == GLFW_MOUSE_BUTTON_RIGHT)
                {
                    Instance->bWindowsRightMousePressed = (Action == GLFW_PRESS);
                }
                if (Button == GLFW_MOUSE_BUTTON_MIDDLE)
                {
                    Instance->bWindowsMiddleMousePressed = (Action == GLFW_PRESS);
                }
            }
            else
            {
                //Release the mouse
                Instance->bWindowsLeftMousePressed = false;
                Instance->bWindowsRightMousePressed = false;
                Instance->bWindowsMiddleMousePressed = false;
            }
            double PositionX, PositionY;
            glfwGetCursorPos(Window, &PositionX, &PositionY);
            const ImGuiMouseButton_ ImguiButton = (Button == GLFW_MOUSE_BUTTON_LEFT)
                ? ImGuiMouseButton_Left
                : (Button == GLFW_MOUSE_BUTTON_RIGHT ? ImGuiMouseButton_Right : ImGuiMouseButton_Middle);
            ImGuiIO& IO = ImGui::GetIO();
            IO.MousePos = ImVec2((float)PositionX, (float)PositionY);
            IO.MouseDown[ImguiButton] = (Action == GLFW_PRESS);
        });
}

void VoxelWindowsInstance::SubInitializeScrollCallback()
{
    //Get the mouse scroll action
    glfwSetScrollCallback(LVKWindow, [](GLFWwindow* Window, double DeltaX, double DeltaY)
        {
            ImGuiIO& IO = ImGui::GetIO();
            IO.MouseWheelH = (float)DeltaX;
            IO.MouseWheel = (float)DeltaY;
        });
}

void VoxelWindowsInstance::SubInitializeKeyCallback()
{
#define VOXEL_KEY_CLICK(DesiredKey) ((Key == GLFW_KEY_##DesiredKey##) && Pressed)
#define VOXEL_KEY_SHIFT_CLICK(DesiredKey) ((Key == GLFW_KEY_##DesiredKey##) && Pressed && (mods & GLFW_MOD_SHIFT))
#define VOXEL_KEY_CTRL_CLICK(DesiredKey) ((Key == GLFW_KEY_##DesiredKey##) && Pressed && (mods & GLFW_MOD_CONTROL))
#define VOXEL_KEY_CTRL_SHIFT_CLICK(DesiredKey) ((Key == GLFW_KEY_##DesiredKey##) && Pressed && (mods & GLFW_MOD_CONTROL) && (mods & GLFW_MOD_SHIFT))
#define VOXEL_KEY_PRESS(DesiredKey) ((Key == GLFW_KEY_##DesiredKey##))
    //Key Callback
    glfwSetKeyCallback(LVKWindow, [](GLFWwindow* Window, int Key, int, int Action, int Mods)
        {
            VoxelWindowsInstance* Instance = static_cast<VoxelWindowsInstance*>(glfwGetWindowUserPointer(Window));
            const bool Pressed = Action != GLFW_RELEASE;
            if (VOXEL_KEY_CLICK(ESCAPE))//(key == GLFW_KEY_ESCAPE && Pressed)
            {
                glfwSetWindowShouldClose(Window, GLFW_TRUE);
            }
            if (VOXEL_KEY_CLICK(N))//(key == GLFW_KEY_N && Pressed)
            {
                //perFrame_.bDrawNormals = (perFrame_.bDrawNormals + 1) % 2;
            }
            if (VOXEL_KEY_CLICK(ESCAPE))//(key == GLFW_KEY_ESCAPE && Pressed)
            {
                glfwSetWindowShouldClose(Window, GLFW_TRUE);
            }
            if (VOXEL_KEY_PRESS(W)) // key == GLFW_KEY_W // Continuous action
            {
                Instance->WindowsCamera.CameraPositioner.movement_.forward_ = Pressed;
            }
            if (VOXEL_KEY_PRESS(S))
            {
                Instance->WindowsCamera.CameraPositioner.movement_.backward_ = Pressed;
            }
            if (VOXEL_KEY_PRESS(A))
            {
                Instance->WindowsCamera.CameraPositioner.movement_.left_ = Pressed;
            }
            if (VOXEL_KEY_PRESS(D))
            {
                Instance->WindowsCamera.CameraPositioner.movement_.right_ = Pressed;
            }
            if (VOXEL_KEY_PRESS(Q))
            {
                Instance->WindowsCamera.CameraPositioner.movement_.up_ = Pressed;
            }
            if (VOXEL_KEY_PRESS(E))
            {
                Instance->WindowsCamera.CameraPositioner.movement_.down_ = Pressed;
            }
            if (VOXEL_KEY_PRESS(LEFT_SHIFT)) //key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT
            {
                Instance->WindowsCamera.CameraPositioner.movement_.fastSpeed_ = Pressed;
            }
        });
}

void VoxelWindowsInstance::CreateWindowsFrameBuffer()
{
    const lvk::Format Format = bUseHDRForFinalBuffer ? lvk::Format_RGBA_F16 : lvk::Format_RGBA_UN8;
    lvk::TextureDesc DescColor = 
    {
        .type = lvk::TextureType_2D,
        .format = Format,
        .dimensions = {(uint32_t)WindowsWidth, (uint32_t)WindowsHeight},
        .usage = lvk::TextureUsageBits_Attachment | lvk::TextureUsageBits_Sampled | lvk::TextureUsageBits_Storage,
        .numMipLevels = lvk::calcNumMipLevels((uint32_t)WindowsWidth, (uint32_t)WindowsHeight),
        .debugName = "Offscreen framebuffer (color)",
    };
    TEXOffscreenColor = LVKContext->createTexture(DescColor);
}

void VoxelWindowsInstance::InitializeRenderPass()
{
    RPSwapchain =
    {
        .color = {{.loadOp = lvk::LoadOp_Clear,
                   .storeOp = lvk::StoreOp_Store,
                   .clearColor = {0.0f, 0.0f, 0.0f, 1.0f}}},
    };
}

void VoxelWindowsInstance::InitializeFrameBuffer()
{
    CreateWindowsFrameBuffer();
    FBSwapchain =
    {
        .color = {{.texture = LVKContext->getCurrentSwapchainTexture()}},
    };
}

void VoxelWindowsInstance::InitializePipeline()
{
    ShaderFullScreenVSInstance.UpdateShaderHandle(LVKContext.get());
    ShaderFullScreenFSInstance.UpdateShaderHandle(LVKContext.get());
    {
        const lvk::RenderPipelineDesc Desc = {
            .smVert = ShaderFullScreenVSInstance.SMHandle,
            .smFrag = ShaderFullScreenFSInstance.SMHandle,
            .color = {{.format = LVKContext->getFormat(FBSwapchain.color[0].texture)}},
            .depthFormat = LVKContext->getFormat(FBSwapchain.depthStencil.texture),
            .cullMode = lvk::CullMode_None,
            .debugName = "Pipeline: fullscreen",
        };
        RPLSwapchain = LVKContext->createRenderPipeline(Desc, nullptr);
    }
}

void VoxelWindowsInstance::InitializeImgui()
{
    WindowsImgui = std::make_unique<lvk::ImGuiRenderer>(*LVKContext, Fonts.c_str(), float(WindowsHeight) / 70.0f);
}

void VoxelWindowsInstance::InitializeFinal()
{
}

void VoxelWindowsInstance::InitializeRender()
{
    RenderPreviousTime = glfwGetTime();
    RenderCurrentTime = RenderPreviousTime;
    RenderDeltaTime = 0.0;
    RenderFrameIndex = 0;
    RenderGlobalFrameIndex = 0;
}

void VoxelWindowsInstance::RunInstance()
{
    while (!glfwWindowShouldClose(LVKWindow))
    {
        glfwPollEvents();
        //
        {
            RenderCurrentTime = glfwGetTime();
            RenderDeltaTime = RenderCurrentTime - RenderPreviousTime;
            RenderPreviousTime = RenderCurrentTime;

            if (!WindowsWidth || !WindowsHeight)
            {
                continue;
            }
            FPSCounter.tick(RenderDeltaTime);
        }
        //
        UpdateCamera();
        UpdatePhysics();
        RenderStart();
        Render();
        RenderEnd();
        UpdateFrameIndex();
    }
}
void VoxelWindowsInstance::UpdateCamera()
{
    WindowsCamera.CameraPositioner.update(RenderDeltaTime, WindowsMousePosition, bWindowsLeftMousePressed);
    WindowsCamera.UpdateCamera(VoxelSceneConfig);
}

void VoxelWindowsInstance::UpdatePhysics()
{
}

void VoxelWindowsInstance::UpdateFrameIndex()
{
    RenderGlobalFrameIndex = RenderGlobalFrameIndex + 1;
    RenderFrameIndex = RenderGlobalFrameIndex % LVKNumBufferedFrames;
}

void VoxelWindowsInstance::RenderStart()
{
    LVK_PROFILER_FUNCTION();
}

void VoxelWindowsInstance::Render()
{
}

void VoxelWindowsInstance::RenderEnd()
{
    ProcessRenderImgui();
    if (!WindowsWidth && !WindowsHeight)
    {
        return;
    }

    FBSwapchain.color[0].texture = LVKContext->getCurrentSwapchainTexture();
    FGPUUniformCamera CurrentCameraUniform = WindowsCamera.GetCameraUniform(WindowsWidth, WindowsHeight);
    LVKContext->upload(UBOCamera[RenderFrameIndex], &CurrentCameraUniform, sizeof(CurrentCameraUniform));

    //Command buffers (1-N per thread): create, submit and forget
    //Render into the swapchain image
    {
        lvk::ICommandBuffer& Buffer = LVKContext->acquireCommandBuffer();

        // This will clear the framebuffer
        Buffer.cmdBeginRendering(RPSwapchain, FBSwapchain);
        {
            Buffer.cmdBindRenderPipeline(RPLSwapchain);
            Buffer.cmdPushDebugGroupLabel("Swapchain Output", 0xff0000ff);
            Buffer.cmdBindDepthState({ .compareOp = lvk::CompareOp_Less, .isDepthWriteEnabled = true });

            struct
            {
                uint32_t Texture;
            } Bindings =
            {
                .Texture = TEXOffscreenColor.index(),
            };
            Buffer.cmdPushConstants(Bindings);

            Buffer.cmdDraw(3);
            Buffer.cmdPopDebugGroupLabel();

            WindowsImgui->endFrame(Buffer);
        }
        Buffer.cmdEndRendering();

        LVKContext->submit(Buffer, FBSwapchain.color[0].texture);
    }
}


void VoxelWindowsInstance::ProcessRenderImgui()
{
    SubprocessRenderImguiInitialize();
    SubprocessRenderImguiDemoWindow();
    SubprocessRenderImguiHints();
    SubprocessRenderImguiFPS();
}

void VoxelWindowsInstance::SubprocessRenderImguiInitialize()
{
    FBSwapchain.color[0].texture = LVKContext->getCurrentSwapchainTexture();
    WindowsImgui->beginFrame(FBSwapchain);
}

void VoxelWindowsInstance::SubprocessRenderImguiDemoWindow()
{
    ImGui::ShowDemoWindow();
}

void VoxelWindowsInstance::SubprocessRenderImguiHints()
{
    ImGui::Begin("Keyboard hints:", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("W/S/A/D - camera movement");
    ImGui::Text("Q/E - camera up/down");
    ImGui::Text("Shift - fast movement");
    ImGui::Text("N - toggle normals");
    ImGui::End();
}

void VoxelWindowsInstance::SubprocessRenderImguiFPS()
{
    const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
    const ImGuiViewport* v = ImGui::GetMainViewport();
    LVK_ASSERT(v);
    ImGui::SetNextWindowPos(
        {
            v->WorkPos.x + v->WorkSize.x - 15.0f,
            v->WorkPos.y + 15.0f,
        },
        ImGuiCond_Always,
        { 1.0f, 0.0f });
    ImGui::SetNextWindowBgAlpha(0.30f);
    ImGui::SetNextWindowSize(ImVec2(ImGui::CalcTextSize("FPS : _______").x, 0));
    if (ImGui::Begin("##FPS", nullptr, Flags)) {
        ImGui::Text("FPS : %i", (int)FPSCounter.getFPS());
        ImGui::Text("Ms  : %.1f", 1000.0 / FPSCounter.getFPS());
    }
    ImGui::End();
}
