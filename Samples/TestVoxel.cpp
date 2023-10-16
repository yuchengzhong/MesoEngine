/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

/*
 * A brief tutorial how to run this beast:
 *
 * 1) Run the script "deploy_deps.py" from the IGL root folder.
 * 2) Run the script "deploy_content.py" from the IGL root folder.
 * 3) Run this app.
 *
 */

#if !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES
#endif // _USE_MATH_DEFINES
#include <cassert>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <stdio.h>
#include <thread>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

// GLI should be included after GLM
//#include <gli/texture2d.hpp>

#include <shared/Camera.h>
#include <shared/UtilsFPS.h>
#include <taskflow/taskflow.hpp>
//#include <tiny_obj_loader.h>

#include <lvk/LVK.h>
#include <lvk/HelpersImGui.h>

#include <GLFW/glfw3.h>

#include "BasicShape/Cube.h"
#include "Helper/FileHelper.h"
#include <Shader/ShaderFullScreen.h>

#ifdef __APPLE__
#warning Not supported. Currently Vulkan 1.2 + extensions are available in MoltenVK.
// Known issues: https://github.com/KhronosGroup/MoltenVK/issues/2011
#endif

#ifndef __APPLE__
constexpr int kNumSamplesMSAA = 1;
#else
constexpr int kNumSamplesMSAA = 4;
#endif
constexpr bool kPreferIntegratedGPU = false;
#if defined(NDEBUG)
constexpr bool kEnableValidationLayers = false;
#else
constexpr bool kEnableValidationLayers = true;
#endif // NDEBUG

std::unique_ptr<lvk::ImGuiRenderer> imgui_;

const char* kCodeFullscreenVS = R"(
layout (location=0) out vec2 uv;
void main() {
  // generate a triangle covering the entire screen
  uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  gl_Position = vec4(uv * vec2(2, -2) + vec2(-1, 1), 0.0, 1.0);
}
)";

const char* kCodeFullscreenFS = R"(
layout (location=0) in vec2 uv;
layout (location=0) out vec4 out_FragColor;

layout(push_constant) uniform constants {
	uint tex;
} pc;

void main() {
  out_FragColor = textureBindless2D(pc.tex, 0, uv);
}
)";

const char* kCodeVS = R"(
layout (location=0) in vec3 pos;
layout (location=1) in vec3 normal;
layout (location=2) in vec2 uv;

layout(std430, buffer_reference) readonly buffer PerFrame {
  mat4 proj;
  mat4 view;
};

layout(std430, buffer_reference) readonly buffer PerObject {
  mat4 model;
};
layout(push_constant) uniform constants
{
	PerFrame perFrame;
   PerObject perObject;
} pc;

// output
struct PerVertex {
  vec3 normal;
  vec2 uv;
};
layout (location=0) out PerVertex vtx;
//

void main() {
  mat4 proj = pc.perFrame.proj;
  mat4 view = pc.perFrame.view;
  mat4 model = pc.perObject.model;
  gl_Position = proj * view * model * vec4(pos, 1.0);

  // Compute the normal in world-space
  mat3 norm_matrix = transpose(inverse(mat3(model)));
  vtx.normal = normalize(norm_matrix * normal);
  vtx.uv = uv;
}
)";

const char* kCodeFS = R"(

layout(std430, buffer_reference) readonly buffer PerFrame {
  mat4 proj;
  mat4 view;
};

struct PerVertex {
  vec3 normal;
  vec2 uv;
};

layout(push_constant) uniform constants
{
	PerFrame perFrame;
} pc;


layout (location=0) in PerVertex vtx;

layout (location=0) out vec4 out_FragColor;

void main() {
  vec3 n = normalize(vtx.normal);
  float NdotL1 = clamp(dot(n, normalize(vec3(-1, 1,+1))), 0.0, 1.0);
  float NdotL2 = clamp(dot(n, normalize(vec3(-1, 1,-1))), 0.0, 1.0);
  float NdotL = 0.5 * (NdotL1+NdotL2);
  out_FragColor = vec4(NdotL);
}
)";

const char* kSkyboxVS = R"(
layout (location=0) out vec3 WorldPosition;

const vec3 positions[8] = vec3[8](
	vec3(-1.0,-1.0, 1.0), vec3( 1.0,-1.0, 1.0), vec3( 1.0, 1.0, 1.0), vec3(-1.0, 1.0, 1.0),
	vec3(-1.0,-1.0,-1.0), vec3( 1.0,-1.0,-1.0), vec3( 1.0, 1.0,-1.0), vec3(-1.0, 1.0,-1.0)
);

const int indices[36] = int[36](
	0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 7, 6, 5, 5, 4, 7, 4, 0, 3, 3, 7, 4, 4, 5, 1, 1, 0, 4, 3, 2, 6, 6, 7, 3
);

layout(std430, buffer_reference) readonly buffer PerFrame {
  mat4 proj;
  mat4 view;
};

layout(push_constant) uniform constants
{
	PerFrame perFrame;
} pc;

void main() {
  mat4 proj = pc.perFrame.proj;
  mat4 view = pc.perFrame.view;
  view = mat4(view[0], view[1], view[2], vec4(0, 0, 0, 1));
  mat4 transform = proj * view;
  vec3 pos = positions[indices[gl_VertexIndex]];
  gl_Position = (transform * vec4(pos, 1.0)).xyww;

  // skybox
  WorldPosition = pos;
}

)";
const char* kSkyboxFS = R"(
layout (location=0) in vec3 WorldPosition;
layout (location=0) out vec4 out_FragColor;

layout(std430, buffer_reference) readonly buffer PerFrame {
  mat4 proj;
  mat4 view;
};

layout(push_constant) uniform constants
{
	PerFrame perFrame;
} pc;

void main() 
{
    float t = 0.5 * (normalize(WorldPosition).y + 1.0);
    t = smoothstep(0.0, 1.0, t);
    float t2 = smoothstep(0.5, 1.0, t);

    vec4 skyColor = mix(vec4(0.1, 0.2, 0.5, 1.0), vec4(0.5, 0.7, 1.0, 1.0), t);
    vec4 skyColor2 = mix(vec4(0.01, 0.02, 0.05, 1.0), vec4(0.25, 0.35, 0.5, 1.0), t2);
    out_FragColor = mix(skyColor2, skyColor, t);
}
)";

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

GLFWwindow* window_ = nullptr;
int width_ = 0;
int height_ = 0;
FramesPerSecondCounter fps_;

constexpr uint32_t kNumBufferedFrames = 3;

std::unique_ptr<lvk::IContext> ctx_;
lvk::Framebuffer fbMain_; // swapchain
lvk::Framebuffer fbOffscreen_;
lvk::Holder<lvk::TextureHandle> fbOffscreenColor_;
lvk::Holder<lvk::TextureHandle> fbOffscreenDepth_;
lvk::Holder<lvk::TextureHandle> fbOffscreenResolve_;
lvk::Holder<lvk::ShaderModuleHandle> smMeshVert_;
lvk::Holder<lvk::ShaderModuleHandle> smMeshFrag_;
lvk::Holder<lvk::ShaderModuleHandle> smFullscreenVert_;
lvk::Holder<lvk::ShaderModuleHandle> smFullscreenFrag_;
lvk::Holder<lvk::ShaderModuleHandle> smSkyboxVert_;
lvk::Holder<lvk::ShaderModuleHandle> smSkyboxFrag_;
lvk::Holder<lvk::RenderPipelineHandle> renderPipelineState_Mesh_;
lvk::Holder<lvk::RenderPipelineHandle> renderPipelineState_Skybox_;
lvk::Holder<lvk::RenderPipelineHandle> renderPipelineState_Fullscreen_;
lvk::Holder<lvk::BufferHandle> vb0_, ib0_; // buffers for vertices and indices
std::vector<lvk::Holder<lvk::BufferHandle>> ubPerFrame_, ubPerObject_;
lvk::RenderPass renderPassOffscreen_;
lvk::RenderPass renderPassMain_;
lvk::DepthState depthState_;
lvk::DepthState depthStateLEqual_;

// scene navigation
CameraPositioner_FirstPerson positioner_(vec3(-100, 40, -47), vec3(0, 35, 0), vec3(0, 1, 0));
Camera camera_(positioner_);
glm::vec2 mousePos_ = glm::vec2(0.0f);
bool mousePressed_ = false;



struct UniformsPerFrame {
  mat4 proj;
  mat4 view;
} perFrame_;

struct UniformsPerObject {
  mat4 model;
};

void initIGL() 
{
  ctx_ = lvk::createVulkanContextWithSwapchain(window_,
                                               width_,
                                               height_,
                                               {
                                                   .enableValidation = kEnableValidationLayers,
                                               },
                                               kPreferIntegratedGPU ? lvk::HWDeviceType_Integrated : lvk::HWDeviceType_Discrete);

  for (uint32_t i = 0; i != kNumBufferedFrames; i++) 
  {
      ubPerFrame_.push_back(ctx_->createBuffer({ .usage = lvk::BufferUsageBits_Uniform,
                                                   .storage = lvk::StorageType_HostVisible,
                                                   .size = sizeof(UniformsPerFrame),
                                                   .debugName = "Buffer: uniforms (per frame)" },
          nullptr));
      ubPerObject_.push_back(ctx_->createBuffer({ .usage = lvk::BufferUsageBits_Uniform,
                                                    .storage = lvk::StorageType_HostVisible,
                                                    .size = sizeof(UniformsPerObject),
                                                    .debugName = "Buffer: uniforms (per object)" },
          nullptr));
  }

  depthState_ = {.compareOp = lvk::CompareOp_Less, .isDepthWriteEnabled = true};
  depthStateLEqual_ = {.compareOp = lvk::CompareOp_LessEqual, .isDepthWriteEnabled = true};

  renderPassOffscreen_ = 
  {
      .color = {{
          .loadOp = lvk::LoadOp_Clear,
          .storeOp = kNumSamplesMSAA > 1 ? lvk::StoreOp_MsaaResolve : lvk::StoreOp_Store,
          .clearColor = {0.0f, 0.0f, 0.0f, 1.0f},
      }},
      .depth = {
          .loadOp = lvk::LoadOp_Clear,
          .storeOp = lvk::StoreOp_Store,
          .clearDepth = 1.0f,
      }
  };

  renderPassMain_ = 
  {
      .color = {{.loadOp = lvk::LoadOp_Clear,
                 .storeOp = lvk::StoreOp_Store,
                 .clearColor = {0.0f, 0.0f, 0.0f, 1.0f}}},
  };
}

bool InitCube()
{
    auto BoxMeshes = FCube::GetBoxMeshes(10.0);
    auto& BoxVertexData = std::get<0>(BoxMeshes);
    auto& BoxIndexData = std::get<1>(BoxMeshes);
    vb0_ = ctx_->createBuffer({ .usage = lvk::BufferUsageBits_Vertex,
                                  .storage = lvk::StorageType_Device,
                                  .size = sizeof(FCubeVertexData) * BoxVertexData.size(),
                                  .data = BoxVertexData.data(),
                                  .debugName = "Buffer: vertex" },
                                    nullptr);
    ib0_ = ctx_->createBuffer({ .usage = lvk::BufferUsageBits_Index,
                                  .storage = lvk::StorageType_Device,
                                  .size = sizeof(uint32_t) * BoxIndexData.size(),
                                  .data = BoxIndexData.data(),
                                  .debugName = "Buffer: index" },
                                    nullptr);
    return true;
}

void createPipelines()
{
    if (renderPipelineState_Mesh_.valid())
    {
        return;
    }

    const lvk::VertexInput vdesc = 
    {
        .attributes =
            {
                {.location = 0, .format = lvk::VertexFormat::Float3, .offset = offsetof(FCubeVertexData, Position)},
                {.location = 1, .format = lvk::VertexFormat::Float3, .offset = offsetof(FCubeVertexData, Normal)},
                {.location = 2, .format = lvk::VertexFormat::Float2, .offset = offsetof(FCubeVertexData, UV)}, // UV can force convert from fp 16
            },
        .inputBindings = {{.stride = sizeof(FCubeVertexData)}},
    };

    smMeshVert_ = ctx_->createShaderModule({ kCodeVS, lvk::Stage_Vert, "Shader Module: main (vert)" });
    smMeshFrag_ = ctx_->createShaderModule({ kCodeFS, lvk::Stage_Frag, "Shader Module: main (frag)" });
    ShaderFullScreenVS ShaderFullScreenVSInstance;
    ShaderFullScreenFS ShaderFullScreenFSInstance;
    smFullscreenVert_ = ctx_->createShaderModule({ ShaderFullScreenVSInstance.GetShaderString().c_str(), lvk::Stage_Vert, "Shader Module: fullscreen (vert)"});
    smFullscreenFrag_ = ctx_->createShaderModule({ ShaderFullScreenFSInstance.GetShaderString().c_str(), lvk::Stage_Frag, "Shader Module: fullscreen (frag)" });
    smSkyboxVert_ = ctx_->createShaderModule({ kSkyboxVS, lvk::Stage_Vert, "Shader Module: skybox (vert)" });
    smSkyboxFrag_ = ctx_->createShaderModule({ kSkyboxFS, lvk::Stage_Frag, "Shader Module: skybox (frag)" });

    {
        lvk::RenderPipelineDesc desc = {
            .vertexInput = vdesc,
            .smVert = smMeshVert_,
            .smFrag = smMeshFrag_,
            .color = {{.format = ctx_->getFormat(fbOffscreen_.color[0].texture)}},
            .depthFormat = ctx_->getFormat(fbOffscreen_.depthStencil.texture),
            .cullMode = lvk::CullMode_Back,
            .frontFaceWinding = lvk::WindingMode_CCW,
            .samplesCount = kNumSamplesMSAA,
            .debugName = "Pipeline: mesh",
        };

        renderPipelineState_Mesh_ = ctx_->createRenderPipeline(desc, nullptr);
    }

    // fullscreen
    {
        const lvk::RenderPipelineDesc desc = {
            .smVert = smFullscreenVert_,
            .smFrag = smFullscreenFrag_,
            .color = {{.format = ctx_->getFormat(fbMain_.color[0].texture)}},
            .depthFormat = ctx_->getFormat(fbMain_.depthStencil.texture),
            .cullMode = lvk::CullMode_None,
            .debugName = "Pipeline: fullscreen",
        };
        renderPipelineState_Fullscreen_ = ctx_->createRenderPipeline(desc, nullptr);
    }

    // skybox
    {
        const lvk::RenderPipelineDesc desc = {
            .smVert = smSkyboxVert_,
            .smFrag = smSkyboxFrag_,
            .color = {{
                .format = ctx_->getFormat(fbOffscreen_.color[0].texture),
            }},
            .depthFormat = ctx_->getFormat(fbOffscreen_.depthStencil.texture),
            .cullMode = lvk::CullMode_Front,
            .frontFaceWinding = lvk::WindingMode_CCW,
            .samplesCount = kNumSamplesMSAA,
            .debugName = "Pipeline: skybox",
        };

        renderPipelineState_Skybox_ = ctx_->createRenderPipeline(desc, nullptr);
    }
}

void createOffscreenFramebuffer() {
  const uint32_t w = width_;
  const uint32_t h = height_;
  lvk::TextureDesc descDepth = {
      .type = lvk::TextureType_2D,
      .format = lvk::Format_Z_UN24,
      .dimensions = {w, h},
      .usage = lvk::TextureUsageBits_Attachment | lvk::TextureUsageBits_Sampled,
      .numMipLevels = lvk::calcNumMipLevels(w, h),
      .debugName = "Offscreen framebuffer (d)",
  };
  if (kNumSamplesMSAA > 1) {
    descDepth.usage = lvk::TextureUsageBits_Attachment;
    descDepth.numSamples = kNumSamplesMSAA;
    descDepth.numMipLevels = 1;
  }

  const uint8_t usage = lvk::TextureUsageBits_Attachment | lvk::TextureUsageBits_Sampled |
                        lvk::TextureUsageBits_Storage;
  const lvk::Format format = lvk::Format_RGBA_UN8;

  lvk::TextureDesc descColor = {
      .type = lvk::TextureType_2D,
      .format = format,
      .dimensions = {w, h},
      .usage = usage,
      .numMipLevels = lvk::calcNumMipLevels(w, h),
      .debugName = "Offscreen framebuffer (color)",
  };
  if (kNumSamplesMSAA > 1) {
    descColor.usage = lvk::TextureUsageBits_Attachment;
    descColor.numSamples = kNumSamplesMSAA;
    descColor.numMipLevels = 1;
  }

  fbOffscreenColor_ = ctx_->createTexture(descColor);
  fbOffscreenDepth_ = ctx_->createTexture(descDepth);
  lvk::Framebuffer fb = {
      .color = {{.texture = fbOffscreenColor_}},
      .depthStencil = {.texture = fbOffscreenDepth_},
  };

  if (kNumSamplesMSAA > 1) {
    fbOffscreenResolve_ = ctx_->createTexture({.type = lvk::TextureType_2D,
                                                      .format = format,
                                                      .dimensions = {w, h},
                                                      .usage = usage,
                                                      .debugName = "Offscreen framebuffer (color resolve)"});
    fb.color[0].resolveTexture = fbOffscreenResolve_;
  }

  fbOffscreen_ = fb;
}
uint32_t FirstTime = 0;
void render(lvk::TextureHandle nativeDrawable, uint32_t frameIndex) {
  LVK_PROFILER_FUNCTION();

  if (!width_ && !height_)
    return;

  fbMain_.color[0].texture = nativeDrawable;

  const float fov = float(45.0f * (M_PI / 180.0f));
  const float aspectRatio = (float)width_ / (float)height_;

  const mat4 scaleBias =
      mat4(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.5, 0.5, 0.0, 1.0);

  perFrame_ = UniformsPerFrame{
      .proj = glm::perspective(fov, aspectRatio, 0.5f, 500.0f),
      .view = camera_.getViewMatrix(),
  };
  ctx_->upload(ubPerFrame_[frameIndex], &perFrame_, sizeof(perFrame_));

  if (FirstTime < 3)
  {
      UniformsPerObject perObject;

      perObject.model = glm::identity<mat4>();// glm::scale(mat4(1.0f), vec3(0.05f));

      ctx_->upload(ubPerObject_[frameIndex], &perObject, sizeof(perObject));
      FirstTime += 1;
  }

  // Command buffers (1-N per thread): create, submit and forget

  // Pass 2: mesh
  {
    lvk::ICommandBuffer& buffer = ctx_->acquireCommandBuffer();

    // This will clear the framebuffer
    buffer.cmdBeginRendering(renderPassOffscreen_, fbOffscreen_);
    {
      // Scene
      buffer.cmdBindRenderPipeline(renderPipelineState_Mesh_);
      buffer.cmdPushDebugGroupLabel("Render Mesh", 0xff0000ff);
      buffer.cmdBindDepthState(depthState_);
      buffer.cmdBindVertexBuffer(0, vb0_, 0);

      struct {
        uint64_t perFrame;
        uint64_t perObject;
        uint64_t materials;
      } bindings = {
          .perFrame = ctx_->gpuAddress(ubPerFrame_[frameIndex]),
          .perObject = ctx_->gpuAddress(ubPerObject_[frameIndex]),
      };
      buffer.cmdPushConstants(bindings);
      buffer.cmdBindIndexBuffer(ib0_, lvk::IndexFormat_UI16);
      buffer.cmdDrawIndexed(FCube::GetBoxIndexSize());
      buffer.cmdPopDebugGroupLabel();

      // Skybox
      buffer.cmdBindRenderPipeline(renderPipelineState_Skybox_);
      buffer.cmdPushDebugGroupLabel("Render Skybox", 0x00ff00ff);
      buffer.cmdBindDepthState(depthStateLEqual_);
      buffer.cmdDraw(3 * 6 * 2);
      buffer.cmdPopDebugGroupLabel();
    }
    buffer.cmdEndRendering();
    buffer.transitionToShaderReadOnly(fbOffscreen_.color[0].texture);
    ctx_->submit(buffer);
  }

  // Pass 4: render into the swapchain image
  {
    lvk::ICommandBuffer& buffer = ctx_->acquireCommandBuffer();

    // This will clear the framebuffer
    buffer.cmdBeginRendering(renderPassMain_, fbMain_);
    {
      buffer.cmdBindRenderPipeline(renderPipelineState_Fullscreen_);
      buffer.cmdPushDebugGroupLabel("Swapchain Output", 0xff0000ff);
      buffer.cmdBindDepthState(depthState_);
      struct {
        uint32_t texture;
      } bindings = {
          .texture = kNumSamplesMSAA > 1 ? fbOffscreen_.color[0].resolveTexture.index()
                                         : fbOffscreen_.color[0].texture.index(),
      };
      buffer.cmdPushConstants(bindings);
      buffer.cmdDraw(3);
      buffer.cmdPopDebugGroupLabel();

      imgui_->endFrame(buffer);
    }
    buffer.cmdEndRendering();

    ctx_->submit(buffer, fbMain_.color[0].texture);
  }
}

int main(int argc, char* argv[])
{
    minilog::initialize(nullptr, { .threadNames = false });

    window_ = lvk::initWindow("Vulkan Bistro", width_, height_);
    initIGL();
    InitCube();

    //Resize the FB if windows size is changed
    glfwSetFramebufferSizeCallback(window_, [](GLFWwindow*, int width, int height) 
        {
            width_ = width;
            height_ = height;
            if (!width || !height)
                return;
            ctx_->recreateSwapchain(width, height);
            createOffscreenFramebuffer();
        });

    //Get the mouse position
    glfwSetCursorPosCallback(window_, [](auto* window, double x, double y) 
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            if (width && height) 
            {
                mousePos_ = vec2(x / width, 1.0f - y / height);
                ImGui::GetIO().MousePos = ImVec2(x, y);
            }
        });

    //Get the mouse button action
    glfwSetMouseButtonCallback(window_, [](auto* window, int button, int action, int mods) 
        {
            if (!ImGui::GetIO().WantCaptureMouse) 
            {
                if (button == GLFW_MOUSE_BUTTON_LEFT) 
                {
                    mousePressed_ = (action == GLFW_PRESS);
                }
            }
            else 
            {
                // release the mouse
                mousePressed_ = false;
            }
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            const ImGuiMouseButton_ imguiButton = (button == GLFW_MOUSE_BUTTON_LEFT)
                ? ImGuiMouseButton_Left
                : (button == GLFW_MOUSE_BUTTON_RIGHT ? ImGuiMouseButton_Right : ImGuiMouseButton_Middle);
            ImGuiIO& io = ImGui::GetIO();
            io.MousePos = ImVec2((float)xpos, (float)ypos);
            io.MouseDown[imguiButton] = action == GLFW_PRESS;
        });

    //Get the mouse scroll action
    glfwSetScrollCallback(window_, [](GLFWwindow* window, double dx, double dy) 
        {
            ImGuiIO& io = ImGui::GetIO();
            io.MouseWheelH = (float)dx;
            io.MouseWheel = (float)dy;
        });

    glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int, int action, int mods) 
        {
            const bool pressed = action != GLFW_RELEASE;
            if (key == GLFW_KEY_ESCAPE && pressed) 
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            if (key == GLFW_KEY_N && pressed) 
            {
                //perFrame_.bDrawNormals = (perFrame_.bDrawNormals + 1) % 2;
            }
            if (key == GLFW_KEY_ESCAPE && pressed)
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            if (key == GLFW_KEY_W) 
            {
                positioner_.movement_.forward_ = pressed;
            }
            if (key == GLFW_KEY_S) 
            {
                positioner_.movement_.backward_ = pressed;
            }
            if (key == GLFW_KEY_A) 
            {
                positioner_.movement_.left_ = pressed;
            }
            if (key == GLFW_KEY_D) 
            {
                positioner_.movement_.right_ = pressed;
            }
            if (key == GLFW_KEY_1) 
            {
                positioner_.movement_.up_ = pressed;
            }
            if (key == GLFW_KEY_2) 
            {
                positioner_.movement_.down_ = pressed;
            }
            if (mods & GLFW_MOD_SHIFT) 
            {
                positioner_.movement_.fastSpeed_ = pressed;
            }
            if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) 
            {
                positioner_.movement_.fastSpeed_ = pressed;
            }
            if (key == GLFW_KEY_SPACE) 
            {
                positioner_.setUpVector(vec3(0.0f, 1.0f, 0.0f));
            }
            if (key == GLFW_KEY_F9 && action == GLFW_PRESS) //Screen shot
            {
                //gli::texture screenshot = gli::texture2d(gli::FORMAT_BGRA8_UNORM_PACK8, gli::extent2d(width_, height_), 1);
                //ctx_->download(ctx_->getCurrentSwapchainTexture(), { .dimensions = {(uint32_t)width_, (uint32_t)height_} }, screenshot.data());
                //gli::save_ktx(screenshot, "screenshot.ktx");
            }
        });

    fbMain_ = 
    {
        .color = {{.texture = ctx_->getCurrentSwapchainTexture()}},
    };
    createOffscreenFramebuffer();
    createPipelines();
    printf("Font: %s\n", (FFileHelper::GetFontPath() + "/simhei.ttf").c_str());
    imgui_ = std::make_unique<lvk::ImGuiRenderer>(*ctx_, (FFileHelper::GetFontPath()+"/simhei.ttf").c_str(), float(height_) / 70.0f);

    double prevTime = glfwGetTime();

    uint32_t frameIndex = 0;

    // Main loop
    while (!glfwWindowShouldClose(window_)) 
    {
        glfwPollEvents();

        const double newTime = glfwGetTime();
        const double delta = newTime - prevTime;
        prevTime = newTime;

        if (!width_ || !height_)
            continue;

        fps_.tick(delta);

        {
            fbMain_.color[0].texture = ctx_->getCurrentSwapchainTexture();
            imgui_->beginFrame(fbMain_);
            ImGui::ShowDemoWindow();

            ImGui::Begin("Keyboard hints:", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("W/S/A/D - camera movement");
            ImGui::Text("1/2 - camera up/down");
            ImGui::Text("Shift - fast movement");
            ImGui::Text("N - toggle normals");
            ImGui::End();

            // a nice FPS counter
            {
                const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
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
                if (ImGui::Begin("##FPS", nullptr, flags)) {
                    ImGui::Text("FPS : %i", (int)fps_.getFPS());
                    ImGui::Text("Ms  : %.1f", 1000.0 / fps_.getFPS());
                }
                ImGui::End();
            }
        }

        positioner_.update(delta, mousePos_, mousePressed_);
        render(ctx_->getCurrentSwapchainTexture(), frameIndex);
        frameIndex = (frameIndex + 1) % kNumBufferedFrames;
    }

    imgui_ = nullptr;
    // destroy all the Vulkan stuff before closing the window
    vb0_ = nullptr;
    ib0_ = nullptr;
    ubPerFrame_.clear();
    ubPerObject_.clear();
    smMeshVert_ = nullptr;
    smMeshFrag_ = nullptr;
    smFullscreenVert_ = nullptr;
    smFullscreenFrag_ = nullptr;
    smSkyboxVert_ = nullptr;
    smSkyboxFrag_ = nullptr;
    renderPipelineState_Mesh_ = nullptr;
    renderPipelineState_Skybox_ = nullptr;
    renderPipelineState_Fullscreen_ = nullptr;
    ctx_->destroy(fbMain_);
    fbOffscreenColor_ = nullptr;
    fbOffscreenDepth_ = nullptr;
    fbOffscreenResolve_ = nullptr;
    ctx_ = nullptr;

    glfwDestroyWindow(window_);
    glfwTerminate();

    printf("Waiting for the loader thread to exit...\n");

    return 0;
}
