#include "Instance/VoxelWindowsInstance.h"
#include <Shader/ShaderShadertoy.h>

#if defined(NDEBUG)
constexpr bool kEnableValidationLayers = false;
#else
constexpr bool kEnableValidationLayers = true;
#endif
constexpr bool kPreferIntegratedGPU = false;
constexpr uint32_t kNumSamplesMSAA = 1;
constexpr uint32_t kNumBufferedFrames = 3;



class SimpleShadertoyWindowsInstance : public VoxelWindowsInstance
{
public:
    lvk::RenderPass RPShadertoy;
    lvk::Framebuffer FBShadertoy;
    lvk::Holder<lvk::RenderPipelineHandle> RPLShadertoy;


    ShaderFullScreenVS ShaderFullScreenVSInstance;
    ShaderShadertoyFS ShaderShadertoyFSInstance;
    void CreateWindowsFrameBuffer() override
    {
        VoxelWindowsInstance::CreateWindowsFrameBuffer();
        lvk::Framebuffer FB = 
        {
            .color = {{.texture = TEXOffscreenColor}},
        };
        FBShadertoy = FB;
    }

    void InitializeRenderPass() override
    {
        VoxelWindowsInstance::InitializeRenderPass();
        RPShadertoy =
        {
            .color = {{.loadOp = lvk::LoadOp_Clear,
                       .storeOp = lvk::StoreOp_Store,
                       .clearColor = {0.0f, 0.0f, 0.0f, 1.0f}}},
        };
    }
    void InitializePipeline() override
    {
        VoxelWindowsInstance::InitializePipeline();
        ShaderFullScreenVSInstance.UpdateShaderHandle(LVKContext.get());
        ShaderShadertoyFSInstance.UpdateShaderHandle(LVKContext.get());
        {
            const lvk::RenderPipelineDesc Desc = {
                .smVert = ShaderFullScreenVSInstance.SMHandle,
                .smFrag = ShaderShadertoyFSInstance.SMHandle,
                .color = {{.format = LVKContext->getFormat(TEXOffscreenColor)}},
                .depthFormat = LVKContext->getFormat(FBSwapchain.depthStencil.texture),
                .cullMode = lvk::CullMode_None,
                .debugName = "Pipeline: shadertoy",
            };
            RPLShadertoy = LVKContext->createRenderPipeline(Desc, nullptr);
        }
    }
    void Render() override
    {
        VoxelWindowsInstance::Render();
        {
            lvk::ICommandBuffer& Buffer = LVKContext->acquireCommandBuffer();
            Buffer.cmdBeginRendering(RPShadertoy, FBShadertoy);
            {
                // Scene
                Buffer.cmdBindRenderPipeline(RPLShadertoy);
                Buffer.cmdPushDebugGroupLabel("Render FullScreen", 0xff0000ff);
                Buffer.cmdBindDepthState({ .compareOp = lvk::CompareOp_Less, .isDepthWriteEnabled = true });
                Buffer.cmdDraw(3);
                Buffer.cmdPopDebugGroupLabel();
            }
            Buffer.cmdEndRendering();
            Buffer.transitionToShaderReadOnly(FBShadertoy.color[0].texture); //Transit
            LVKContext->submit(Buffer);
        }
    }

};

int main(int argc, char* argv[])
{
    SimpleShadertoyWindowsInstance Instance;
    Instance.Initialize({
        .bEnableValidationLayers = kEnableValidationLayers,
        .bPreferIntegratedGPU = kPreferIntegratedGPU,
        .kNumBufferedFrames = kNumBufferedFrames,
        .kNumSamplesMSAA = kNumSamplesMSAA,
        .bInitialFullScreen = false
        });

    // Main loop
    Instance.RunInstance();
    return 0;
}