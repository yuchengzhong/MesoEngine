#include "Instance/VoxelWindowsInstance.h"
#include "Shape/Cube.h"
#include "Shape/Octahedron.h"
#include "Voxel/Chunk/ChunkManager.h"
#include "Helper/GeneratorHelper.h"

#if defined(NDEBUG)
constexpr bool kEnableValidationLayers = false;
#else
constexpr bool kEnableValidationLayers = true;
#endif
constexpr bool kPreferIntegratedGPU = false;
constexpr uint32_t kNumSamplesMSAA = 1;
constexpr uint32_t kNumBufferedFrames = 3;

class ShaderInstanceVoxelBase :public ShaderBase
{
public:
    uint32_t ChunkArraySize = 0;
    uint32_t BlockArraySize = 0;
    uint32_t VolumeArraySize = 0;
    void SetShaderProperty(uint32_t ChunkArraySize_, uint32_t BlockArraySize_, uint32_t VolumeArraySize_)
    {
        ChunkArraySize = ChunkArraySize_;
        BlockArraySize = BlockArraySize_;
        VolumeArraySize = VolumeArraySize_;
    }
};

class ShaderInstanceVoxelVS :public ShaderInstanceVoxelBase
{
public:
    lvk::ShaderStage GetShaderStage() const override
    {
        return lvk::Stage_Vert;
    }
    std::string GetShaderDebugInfo() const override
    {
        return std::string("Shader Module: Instance Voxel (vert)");
    }
    std::string GetShaderString() const override
    {
        //layout(location = 0) in vec3 Position;
        //layout(location = 1) in vec3 Normal;

        return FGPUSimpleVertexData::GetLayoutShader() + FGPUBlock::GetLayoutShader(2) + 
            FGPUUniformCamera::GetStructureShader() + FGPUUniformSceneConfig::GetStructureShader() +
            FGPUChunk::GetStructureShader() + 
            DefineConstant(ChunkArraySize, "CHUNK_ARRAY_SIZE") +
R"(
#define INT_MAX 2147483647
#extension GL_EXT_buffer_reference2 : require
layout(buffer_reference, std430) readonly buffer ChunksBuffer 
{
    GPUChunk ChunkData[CHUNK_ARRAY_SIZE];
};
layout(push_constant) uniform constants
{
	CameraInfo Camera;
	SceneInfo Scene;
    ChunksBuffer Chunks;
} pc;
// output
struct PerVertex 
{
    vec3 Normal;
    vec3 Color;
};
layout (location=0) out PerVertex vtx;

ivec3 UnpackU8Vec3(uint PackedValue)
{
    uint x = (PackedValue >> 24) & 0xFF;
    uint y = (PackedValue >> 16) & 0xFF;
    uint z = (PackedValue >> 8) & 0xFF;
    return ivec3(x, y, z);
}
ivec3 ReverseUnpackU8Vec3(uint PackedValue)
{
    uint x = (PackedValue >> 0) & 0xFF;
    uint y = (PackedValue >> 8) & 0xFF;
    uint z = (PackedValue >> 16) & 0xFF;
    return ivec3(x, y, z);
}
void main() 
{
    ivec3 InstanceChunkLocation = ivec3(INT_MAX, INT_MAX, INT_MAX);
    if(InstanceChunkIndex != INT_MAX) // Valid  // && InstanceChunkIndex < CHUNK_ARRAY_SIZE
    {
        GPUChunk CachedChunk = pc.Chunks.ChunkData[InstanceChunkIndex];
        if(CachedChunk.ChunkFrameStamp == InstanceBlockFrameStamp) //Valid
        {
            InstanceChunkLocation = CachedChunk.ChunkLocation;
        }
    }
    ivec3 ChunkOffset = InstanceChunkLocation - pc.Camera.CameraChunkLocation;
    ivec3 SubOffset = ReverseUnpackU8Vec3(InstanceBlockLocation);

    ivec3 ViewRelativeBlockOffset = ChunkOffset * int(pc.Scene.ChunkResolution) + SubOffset;
    float ChunkResolutionInv = 1.0 / pc.Scene.ChunkResolution;
    vec3 RealVertexPosition = 0.5 * VertexPosition * pc.Scene.BlockSize + ViewRelativeBlockOffset * ChunkResolutionInv * pc.Scene.ChunkSize;

    mat4 Projection = pc.Camera.Projection;
    mat4 View = pc.Camera.View;
    gl_Position = Projection * View * vec4(RealVertexPosition, 1.0); //TODO: If Camera Location is inside a cube, move cube surface backward in clip space(for reduce z fighting)

    vtx.Normal = normalize(VertexNormal);
    vtx.Color = vec3(1.0);
}
)";
    }
};
class ShaderInstanceVoxelFS :public ShaderInstanceVoxelBase
{
public:
    lvk::ShaderStage GetShaderStage() const override
    {
        return lvk::Stage_Frag;
    }
    std::string GetShaderDebugInfo() const override
    {
        return std::string("Shader Module: Instance Voxel (frag)");
    }
    std::string GetShaderString() const override
    {
        return R"(
struct PerVertex 
{
    vec3 Normal;
    vec3 Color;
};

layout (location=0) in PerVertex vtx;

layout (location=0) out vec4 out_FragColor;

void main() 
{
    vec3 n = normalize(vtx.Normal);
    out_FragColor = vec4(n*0.5+0.5, 0.0);
}
)";
    }
};

class SimpleVoxelWindowsInstance : public VoxelWindowsInstance
{
public:
    lvk::Holder<lvk::TextureHandle> TEXOffscreenDepth;

    lvk::Framebuffer FBVoxelInstance;
    lvk::RenderPass RPVoxelInstance;
    lvk::Holder<lvk::RenderPipelineHandle> RPLVoxelInstance;


    ShaderInstanceVoxelVS ShaderInstanceVoxelVSInstance;
    ShaderInstanceVoxelFS ShaderInstanceVoxelFSInstance;

    //Shape
    FCubeHolder CubeMesh;

    //Multi thread
    uint32_t ThreadCount = 4;

    //High level manager
    FBlockPoolHolder BlockPool;
    FChunkManage ChunkManager;
    SimpleVoxelWindowsInstance() {}
    void InitializeContext() override
    {
        VoxelWindowsInstance::InitializeContext();
        //Mesh shape
        CubeMesh.Initialize(LVKContext.get());

        BlockPool.Initialize(LVKContext.get(), VoxelSceneConfig);
    }
    void InitializeBegin() override
    {
        VoxelWindowsInstance::InitializeBegin();
        uint32_t MaxCPUCoreNum = boost::thread::hardware_concurrency();
        ThreadCount = std::min(std::max(MaxCPUCoreNum - 2u, 1u), VoxelSceneConfig.MaxUnsyncedLoadChunkCount);
        ChunkManager.Initialize(LVKContext.get(), ThreadCount, VoxelSceneConfig, [](ivec3 StartLocation, float BlockSize, unsigned char ChunkResolution, uint32_t MipmapLevel)
            {
                return FGeneratorHelper::GenerateSphere(StartLocation, BlockSize, ChunkResolution, MipmapLevel);
            });
    }
    void WhenCameraChunkUpdate() override
    {
        printf("Chunk Update\n");
    }
    void WhenCameraUpdate() override
    {
        ChunkManager.UpdateChunks(WindowsCamera.CameraChunkLocation, WindowsCamera.CameraForward, VoxelSceneConfig);
    }
    void UpdatePhysics() override
    {
        ChunkManager.UpdateLoadingQueue(LVKContext.get(), WindowsCamera.CameraChunkLocation, WindowsCamera.CameraForward, VoxelSceneConfig);
    }
    void CreateWindowsFrameBuffer() override
    {
        VoxelWindowsInstance::CreateWindowsFrameBuffer();
        lvk::TextureDesc DescDepth =
        {
            .type = lvk::TextureType_2D,
            .format = lvk::Format_Z_UN24,
            .dimensions = {(uint32_t)WindowsWidth, (uint32_t)WindowsHeight},
            .usage = lvk::TextureUsageBits_Attachment | lvk::TextureUsageBits_Sampled,
            .numMipLevels = lvk::calcNumMipLevels((uint32_t)WindowsWidth, (uint32_t)WindowsHeight),
            .debugName = "Offscreen framebuffer (depth)",
        };
        TEXOffscreenDepth = LVKContext->createTexture(DescDepth);

        lvk::Framebuffer FB =
        {
            .color = {{.texture = TEXOffscreenColor}},
            .depthStencil = {.texture = TEXOffscreenDepth},
        };
        FBVoxelInstance = FB;

        ChunkManager.ChunkPool.InitializeDebugFrameBuffer(LVKContext.get(), TEXOffscreenColor, TEXOffscreenDepth);
    }

    void InitializeRenderPass() override
    {
        VoxelWindowsInstance::InitializeRenderPass();
        RPVoxelInstance =
        {
            .color = 
            {
                {
                    .loadOp = lvk::LoadOp_Clear,
                    .storeOp = lvk::StoreOp_Store,
                    .clearColor = {0.0f, 0.0f, 0.0f, 1.0f}
                }
            },
            .depth = 
            {
                .loadOp = lvk::LoadOp_Clear,
                .storeOp = lvk::StoreOp_Store,
                .clearDepth = 1.0f,
            }
        };
    }
    void InitializePipeline() override
    {
        VoxelWindowsInstance::InitializePipeline();
        ShaderInstanceVoxelVSInstance.SetShaderProperty(VoxelSceneConfig.MaxChunkCount, VoxelSceneConfig.MaxBlockCount, VoxelSceneConfig.MaxVolumeCount);
        ShaderInstanceVoxelVSInstance.UpdateShaderHandle(LVKContext.get());
        ShaderInstanceVoxelFSInstance.UpdateShaderHandle(LVKContext.get());


        {
            const lvk::RenderPipelineDesc Desc = 
            {
                .vertexInput = FGPUBlock::GetInstanceDescriptor(),
                .smVert = ShaderInstanceVoxelVSInstance.SMHandle,
                .smFrag = ShaderInstanceVoxelFSInstance.SMHandle,
                .color = {{.format = LVKContext->getFormat(TEXOffscreenColor)}},
                .depthFormat = LVKContext->getFormat(TEXOffscreenDepth),
                .cullMode = lvk::CullMode_None,
                .frontFaceWinding = lvk::WindingMode_CCW,
                .samplesCount = 1, //Currently only 1 is support(for convenient)
                .debugName = "Pipeline: voxel instance",
            };
            RPLVoxelInstance = LVKContext->createRenderPipeline(Desc, nullptr);
        }
    }
    void Render() override
    {
        VoxelWindowsInstance::Render();

        struct
        {
            uint64_t Camera;
            uint64_t Scene;
        } GlobalUBOBinding = {
            .Camera = LVKContext->gpuAddress(UBOCamera[RenderFrameIndex]),
            .Scene = LVKContext->gpuAddress(UBOSceneConfig),
        };
        struct
        {
            uint64_t Camera;
            uint64_t Scene;
            uint64_t Chunks;
        } GlobalChunkBlockUBOBinding = {
            .Camera = LVKContext->gpuAddress(UBOCamera[RenderFrameIndex]),
            .Scene = LVKContext->gpuAddress(UBOSceneConfig),
            .Chunks = LVKContext->gpuAddress(ChunkManager.ChunkPool.ChunkBuffer),
        };
        {
            lvk::ICommandBuffer& Buffer = LVKContext->acquireCommandBuffer();
            Buffer.cmdBeginRendering(RPVoxelInstance, FBVoxelInstance);
            {
                // Scene
                Buffer.cmdBindRenderPipeline(RPLVoxelInstance);
                BindViewportScissor(Buffer);
                Buffer.cmdPushDebugGroupLabel("Render Voxels", 0xff0000ff);
                Buffer.cmdBindDepthState({ .compareOp = lvk::CompareOp_Less, .isDepthWriteEnabled = true });
                Buffer.cmdBindVertexBuffer(0, CubeMesh.VertexBuffer, 0);
                Buffer.cmdBindVertexBuffer(1, ChunkManager.ChunkPool.BlockBuffer, 0);
                Buffer.cmdBindIndexBuffer(CubeMesh.IndexBuffer, lvk::IndexFormat_UI16);

                Buffer.cmdPushConstants(GlobalChunkBlockUBOBinding);
                Buffer.cmdDrawIndexed(CubeMesh.GetIndexSize(), ChunkManager.ChunkPool.MaxBlockCount); // <-------- TODO: Culling scan in cs, draw indirect
                Buffer.cmdPopDebugGroupLabel();
            }
            Buffer.cmdEndRendering();
            Buffer.transitionToShaderReadOnly(FBVoxelInstance.color[0].texture); //Transit
            LVKContext->submit(Buffer);
        }
        //Debug visible chunk
        ChunkManager.DrawDebugVisibleChunk(LVKContext.get(), GlobalUBOBinding);
    }
    void ProcessRenderImgui() override
    {
        VoxelWindowsInstance::ProcessRenderImgui();
        SubprocessRenderRuntimeInfo();
        ChunkManager.RenderManagerInfo();
    }
    void SubprocessRenderRuntimeInfo()
    {
        ImVec2 NewWindowsPosition = ImVec2(50, 175);
        const float Offset = 200.0f;
        ImGui::SetNextWindowPos(NewWindowsPosition);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, FColorHelper::GetBackgroundRed() );
        ImGui::Begin("Runtime information:", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Block Resolution :");
        ImGui::SameLine(Offset);
        ImGui::Text("%d voxels per block", VoxelSceneConfig.BlockResolution);

        ImGui::Text("Block Size :");
        ImGui::SameLine(Offset);
        ImGui::Text("%.2fm per block", VoxelSceneConfig.BlockSize);

        ImGui::Text("Chunk Resolution :");
        ImGui::SameLine(Offset);
        ImGui::Text("%d blocks per chunk", VoxelSceneConfig.ChunkResolution);

        ImGui::Separator();
        vec3 CameraPosition = WindowsCamera.CameraPositioner.getPosition();
        vec3 CameraForward = WindowsCamera.GetForwardVector();
        ivec3 CameraChunkPosition = WindowsCamera.CameraChunkLocation;
        ImGui::Text("Camera Position :");
        ImGui::SameLine(Offset);
        ImGui::Text("(%.2f,%.2f,%.2f)", CameraPosition.x, CameraPosition.y, CameraPosition.z);

        ImGui::Text("Camera Forward :");
        ImGui::SameLine(Offset);
        ImGui::Text("(%.2f,%.2f,%.2f)", CameraForward.x, CameraForward.y, CameraForward.z);

        ImGui::Text("Camera Chunk Position :");
        ImGui::SameLine(Offset);
        ImGui::Text("(%d,%d,%d)", CameraChunkPosition.x, CameraChunkPosition.y, CameraChunkPosition.z);
        ImGui::End();
        ImGui::PopStyleColor();
    }
};

int main(int argc, char* argv[])
{
    SimpleVoxelWindowsInstance Instance;
    Instance.Initialize({
        .bEnableValidationLayers = kEnableValidationLayers,
        .bPreferIntegratedGPU = kPreferIntegratedGPU,
        .kNumBufferedFrames = kNumBufferedFrames,
        .kNumSamplesMSAA = kNumSamplesMSAA,
        .bInitialFullScreen = false,
        .bShowDemoWindow = false
        });

    // Main loop
    Instance.RunInstance();
    return 0;
}