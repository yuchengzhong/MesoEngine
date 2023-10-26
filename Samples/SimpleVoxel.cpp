#include "Instance/VoxelWindowsInstance.h"
#include "Shape/Cube.h"
#include "Shape/Octahedron.h"
#include "Voxel/Chunk/ChunkManager.h"
#include "Helper/GeneratorHelper.h"
#include "Shape/TriplePlanarCube.h"

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

        return FGPUBlock::GetInstanceLayoutShader(0) + 
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
#define BOX_SIZE 0.5
const vec3 TriplanarPositions[56] = vec3[56](
	vec3(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE), //0
	vec3(-BOX_SIZE, BOX_SIZE, BOX_SIZE), vec3(-BOX_SIZE, BOX_SIZE, -BOX_SIZE), 
	vec3(BOX_SIZE, BOX_SIZE, -BOX_SIZE), vec3(BOX_SIZE, -BOX_SIZE, -BOX_SIZE), 
	vec3(BOX_SIZE, -BOX_SIZE, BOX_SIZE), vec3(-BOX_SIZE, -BOX_SIZE, BOX_SIZE),

	vec3(BOX_SIZE, -BOX_SIZE, -BOX_SIZE), //1
	vec3(BOX_SIZE, BOX_SIZE, BOX_SIZE), vec3(BOX_SIZE, BOX_SIZE, -BOX_SIZE), 
	vec3(-BOX_SIZE, BOX_SIZE, -BOX_SIZE), vec3(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE), 
	vec3(-BOX_SIZE, -BOX_SIZE, BOX_SIZE), vec3(BOX_SIZE, -BOX_SIZE, BOX_SIZE),

	vec3(-BOX_SIZE, BOX_SIZE, -BOX_SIZE), //2
	vec3(-BOX_SIZE, -BOX_SIZE, BOX_SIZE), vec3(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE), 
	vec3(BOX_SIZE, -BOX_SIZE, -BOX_SIZE), vec3(BOX_SIZE, BOX_SIZE, -BOX_SIZE), 
	vec3(BOX_SIZE, BOX_SIZE, BOX_SIZE), vec3(-BOX_SIZE, BOX_SIZE, BOX_SIZE),

	vec3(BOX_SIZE, BOX_SIZE, -BOX_SIZE), //3
	vec3(BOX_SIZE, -BOX_SIZE, BOX_SIZE), vec3(BOX_SIZE, -BOX_SIZE, -BOX_SIZE), 
	vec3(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE), vec3(-BOX_SIZE, BOX_SIZE, -BOX_SIZE), 
	vec3(-BOX_SIZE, BOX_SIZE, BOX_SIZE), vec3(BOX_SIZE, BOX_SIZE, BOX_SIZE),

	vec3(-BOX_SIZE, -BOX_SIZE, BOX_SIZE), //4
	vec3(-BOX_SIZE, BOX_SIZE, -BOX_SIZE), vec3(-BOX_SIZE, BOX_SIZE, BOX_SIZE), 
	vec3(BOX_SIZE, BOX_SIZE, BOX_SIZE), vec3(BOX_SIZE, -BOX_SIZE, BOX_SIZE), 
	vec3(BOX_SIZE, -BOX_SIZE, -BOX_SIZE), vec3(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE),

	vec3(BOX_SIZE, -BOX_SIZE, BOX_SIZE), //5
	vec3(BOX_SIZE, BOX_SIZE, -BOX_SIZE), vec3(BOX_SIZE, BOX_SIZE, BOX_SIZE), 
	vec3(-BOX_SIZE, BOX_SIZE, BOX_SIZE), vec3(-BOX_SIZE, -BOX_SIZE, BOX_SIZE), 
	vec3(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE), vec3(BOX_SIZE, -BOX_SIZE, -BOX_SIZE),

	vec3(-BOX_SIZE, BOX_SIZE, BOX_SIZE), //6
	vec3(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE), vec3(-BOX_SIZE, -BOX_SIZE, BOX_SIZE), 
	vec3(BOX_SIZE, -BOX_SIZE, BOX_SIZE), vec3(BOX_SIZE, BOX_SIZE, BOX_SIZE), 
	vec3(BOX_SIZE, BOX_SIZE, -BOX_SIZE), vec3(-BOX_SIZE, BOX_SIZE, -BOX_SIZE),

	vec3(BOX_SIZE, BOX_SIZE, BOX_SIZE), //7
	vec3(BOX_SIZE, -BOX_SIZE, -BOX_SIZE), vec3(BOX_SIZE, -BOX_SIZE, BOX_SIZE), 
	vec3(-BOX_SIZE, -BOX_SIZE, BOX_SIZE), vec3(-BOX_SIZE, BOX_SIZE, BOX_SIZE), 
	vec3(-BOX_SIZE, BOX_SIZE, -BOX_SIZE), vec3(BOX_SIZE, BOX_SIZE, -BOX_SIZE)
);

int GetOctantId(vec3 v) 
{
    int id = 0;
    if(v.x < 0.0) id |= 1;     // 001
    if(v.y < 0.0) id |= 2;     // 010
    if(v.z < 0.0) id |= 4;     // 100
    return id;
}
vec3 GetOctantColor(vec3 v) 
{
    vec3 c = vec3(0.0);
    if(v.x > 0.0) c.x = 1.0;
    if(v.y > 0.0) c.y = 1.0;
    if(v.z > 0.0) c.z = 1.0;
    return c;
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
    ivec3 ChunkOffset = InstanceChunkLocation - pc.Camera.CameraChunkLocation.xyz;
    ivec3 SubOffset = ReverseUnpackU8Vec3(InstanceBlockLocation);
/*
    ivec3 InstanceChunkLocation = ivec3(0, 0, 0);
    ivec3 ChunkOffset = InstanceChunkLocation - pc.Camera.CameraChunkLocation.xyz;
    ivec3 SubOffset = ivec3(0, 0, 0);
*/
//
    mat4 Projection = pc.Camera.Projection;
    mat4 View = pc.Camera.View;
    vec3 CameraPosition = pc.Camera.SubCameraLocation.xyz;
//
    ivec3 ViewChunkRelativeBlockOffset = ChunkOffset * int(pc.Scene.ChunkResolution) + SubOffset;
    float ChunkResolutionInv = 1.0 / pc.Scene.ChunkResolution;
    vec3 RealViewChunkRelativeBlockOffset = ViewChunkRelativeBlockOffset * ChunkResolutionInv * pc.Scene.ChunkSize;
//
    vec3 RealViewRelativeBlockOffset = RealViewChunkRelativeBlockOffset - CameraPosition;
    int OctantId = GetOctantId(RealViewRelativeBlockOffset);
    vec3 ImposterVertexPosition = TriplanarPositions[gl_VertexIndex + OctantId * 7];
//    
    vec3 RealVertexPosition = (ImposterVertexPosition + 0.5) * pc.Scene.BlockSize + RealViewChunkRelativeBlockOffset;
//    
    gl_Position = Projection * View * vec4(RealVertexPosition, 1.0); //TODO: If Camera Location is inside a cube, move cube surface backward in clip space(for reduce z fighting)
    vtx.Normal = ImposterVertexPosition;
    vtx.Color = GetOctantColor(RealViewRelativeBlockOffset);//vec3(OctantId * 1.0 / 7.0);
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
    out_FragColor = vec4(vtx.Normal*0.5+0.5, 0.0);
    //out_FragColor = vec4(vtx.Color, 0.0);
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
    FTriplePlanarCubeHolder TripleCubeIndex;

    //Multi thread
    uint32_t ThreadCount = 4;

    //High level manager
    FChunkManage ChunkManager;
    SimpleVoxelWindowsInstance() {}
    void InitializeContext() override
    {
        VoxelWindowsInstance::InitializeContext();
        //Mesh shape
        TripleCubeIndex.Initialize(LVKContext.get());
    }
    void InitializeBegin() override
    {
        VoxelWindowsInstance::InitializeBegin();
        uint32_t MaxCPUCoreNum = boost::thread::hardware_concurrency();
        ThreadCount = std::min(std::max(MaxCPUCoreNum - 2u, 1u), VoxelSceneConfig.MaxUnsyncedLoadChunkCount);

        auto GeneratorInstance = [](ivec3 StartLocation, float BlockSize, unsigned char ChunkResolution, uint32_t MipmapLevel)
            {
                return FGeneratorHelper::GenerateSphere(StartLocation, BlockSize, ChunkResolution, MipmapLevel);
            };
        ChunkManager.Initialize(LVKContext.get(), ThreadCount, VoxelSceneConfig, GeneratorInstance, bLVKReverseZ, kNumBufferedFrames);
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
        ChunkManager.UpdateLoadingQueue(LVKContext.get(), WindowsCamera.CameraChunkLocation, WindowsCamera.CameraForward, VoxelSceneConfig, RenderFrameIndex);
    }
    void CreateWindowsFrameBuffer() override
    {
        VoxelWindowsInstance::CreateWindowsFrameBuffer();
        lvk::TextureDesc DescDepth =
        {
            .type = lvk::TextureType_2D,
            .format = bLVKReverseZ ? lvk::Format_Z_F32 : lvk::Format_Z_UN24, 
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
                .clearDepth = bLVKReverseZ ? 0.0f : 1.0f,
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
                .topology = lvk::Topology_TriangleFan,
                .vertexInput = FGPUBlock::GetInstanceDescriptor(),
                .smVert = ShaderInstanceVoxelVSInstance.SMHandle,
                .smFrag = ShaderInstanceVoxelFSInstance.SMHandle,
                .color = {{.format = LVKContext->getFormat(TEXOffscreenColor)}},
                .depthFormat = LVKContext->getFormat(TEXOffscreenDepth),
                .cullMode = lvk::CullMode_None,
                .frontFaceWinding = lvk::WindingMode_CCW,
                //.polygonMode = lvk::PolygonMode_Line,
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
            .Chunks = LVKContext->gpuAddress(ChunkManager.ChunkPool.ChunkBuffer[RenderFrameIndex]),
        };
        {
            lvk::ICommandBuffer& Buffer = LVKContext->acquireCommandBuffer();
            Buffer.cmdBeginRendering(RPVoxelInstance, FBVoxelInstance);
            {
                // Scene
                Buffer.cmdBindRenderPipeline(RPLVoxelInstance);
                BindViewportScissor(Buffer);
                Buffer.cmdPushDebugGroupLabel("Render Voxels", 0xa0ffa0ff);
                Buffer.cmdBindDepthState({ .compareOp = bLVKReverseZ ? lvk::CompareOp_Greater : lvk::CompareOp_Less, .isDepthWriteEnabled = true });
                Buffer.cmdBindVertexBuffer(0, ChunkManager.ChunkPool.BlockBuffer[RenderFrameIndex], 0);
                Buffer.cmdBindIndexBuffer(TripleCubeIndex.IndexBuffer, lvk::IndexFormat_UI16);

                Buffer.cmdPushConstants(GlobalChunkBlockUBOBinding);
                Buffer.cmdDrawIndexed(TripleCubeIndex.GetIndexSize(), ChunkManager.ChunkPool.MaxBlockCount); // ChunkManager.ChunkPool.MaxBlockCount //1  // <-------- TODO: Culling scan in cs, draw indirect
                Buffer.cmdPopDebugGroupLabel();
            }
            Buffer.cmdEndRendering();
            //Buffer.transitionToShaderReadOnly(FBVoxelInstance.color[0].texture);
            LVKContext->submit(Buffer); //ChunkManager.ChunkPool.MainRenderThreadSummitHandle = 
        }
        //Debug visible chunk
        {
            ChunkManager.DrawDebugVisibleChunk(LVKContext.get(), GlobalUBOBinding);
        }
        //
        {
            //lvk::ICommandBuffer& Buffer = LVKContext->acquireCommandBuffer();
            //Buffer.transitionToShaderReadOnly(FBVoxelInstance.color[0].texture); //Transit
            //LVKContext->submit(Buffer);
        }
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