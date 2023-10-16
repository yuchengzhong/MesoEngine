#pragma once
#include <vector>
#include <queue>
#include <set>
#include <map>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include "Voxel/VoxelSceneConfig.h"
#include "Voxel/Block/Block.h"
#include "Chunk.h"
#include "ChunkManagerHelper.h"
#include "Shape/Shape.h"
#include "Shape/Octahedron.h"
#include "Helper/TimerSet.h"
#include "Helper/VoxelMathHelper.h"
#include "Shader/ShaderWireFrame.h"
#include "Thread/ThreadSafeMap.h"
#include "Thread/AtomicVector.h"
#include "Thread/ThreadSafeMemoryPool.h"

#include <functional>
#include <climits>
using glm::ivec3;
using glm::vec3;
using glm::ivec4;
using glm::u8vec3;
using glm::u8vec4;


struct FIVec3Comparator
{
	bool operator()(const glm::ivec3& A, const glm::ivec3& B) const
	{
		if (A.x != B.x) return A.x < B.x;
		if (A.y != B.y) return A.y < B.y;
		return A.z < B.z;
	}
};
enum class EChunkState : uint8_t
{
	Computing = 1 << 0, // 0b0001 or 1
	NonEmpty = 1 << 1, // 0b0010 or 2
	Empty = 1 << 2, // 0b0100 or 4
	Reading = 1 << 3, // 0b1000 or 8
};

class EChunkStateUtils 
{
public:
	static EChunkState AddState(EChunkState CurrentState, EChunkState NewState) 
	{
		return static_cast<EChunkState>(static_cast<uint8_t>(CurrentState) | static_cast<uint8_t>(NewState));
	}

	static EChunkState RemoveState(EChunkState CurrentState, EChunkState OldState)
	{
		return static_cast<EChunkState>(static_cast<uint8_t>(CurrentState) & (~static_cast<uint8_t>(OldState)));
	}

	static bool bHasState(EChunkState CurrentState, EChunkState CheckState)
	{
		return (static_cast<int>(CurrentState) & static_cast<int>(CheckState)) != 0;
	}
};

class FChunkPool
{
public:
	TThreadSafeMemoryPool<FChunk> AtomicChunksPool;
	TThreadSafeMemoryPool<FEmptyChunk> AtomicEmptyChunksPool;
	//For multi thread
	using FChunkLookupTable = TThreadSafeMap<ivec3, EChunkState, FIVec3Comparator>; //if empty,
	FChunkLookupTable ChunksLookupTable; 
	// Before computing, mark as COMPUTING
	// After computing, mask as NonEmpty/Empty
	// Before reading, [threadsafe] read if not computing then mark as reading
	// read
	// After reading, set to original
	std::atomic<uint64_t> AtomicVisibilityChunkFrameStamp = 0;
	//Constant
	uint32_t MaxChunkCount = 0;
	uint32_t MaxEmptyChunkCount = 0;

	//Runtime
	std::atomic<bool> bAtomicDebugVisibleChunkDirty = false;
	std::atomic<bool> bAtomicVisibleChunkDirty = false;
	//Debug
	inline static std::string DebugMarkGatherVisibleChunk = "GatherVisibleChunk";
	inline static std::string DebugMarkUploadVisibleChunk = "UploadVisibleChunk";
	//Debug Draw
	ShaderWireFrameVS ShaderWireFrameVSInstance;
	ShaderWireFrameFS ShaderWireFrameFSInstance;
	lvk::Framebuffer FBDebugInstance;
	lvk::RenderPass RPDebugInstance;
	lvk::Holder<lvk::RenderPipelineHandle> RPLDebugInstance;
	std::vector<FGPUSimpleInstanceData> DebugInstanceBufferCPU;
	lvk::Holder<lvk::BufferHandle> DebugInstanceBuffer;
	FOctahedronHolder OctahedronMesh;

	lvk::VertexInput DescDebugInstance;
	uint32_t CurrentDebugDrawInstanceCount = 0;

	inline static uint32_t LockOffset = 63;
	// Debug
	FTimerSet DebugTimerSet;
	bool bDebugGatherChunk = true;
	FChunkPool()
	{

	}
	uint64_t AtomicGetCurrentChunkFrameStamp() const
	{
		return AtomicVisibilityChunkFrameStamp.load();
	}
	void IncreaseFrameStamp()
	{
		AtomicVisibilityChunkFrameStamp.fetch_add(1);
	}
	void Initialize(lvk::IContext* LVKContext, const FVoxelSceneConfig& VoxelSceneConfig, uint32_t ThreadCount)
	{
		AtomicVisibilityChunkFrameStamp.store(0);
		MaxChunkCount = VoxelSceneConfig.MaxChunkCount;
		MaxEmptyChunkCount = VoxelSceneConfig.MaxEmptyChunkCount;
		AtomicChunksPool.Initialize(MaxChunkCount, ThreadCount);
		AtomicEmptyChunksPool.Initialize(MaxEmptyChunkCount, ThreadCount);
		//
		//For Debug
		OctahedronMesh.Initialize(LVKContext);
		DebugInstanceBuffer = LVKContext->createBuffer(
			{
				.usage = lvk::BufferUsageBits_Vertex,
				.storage = lvk::StorageType_HostVisible,
				.size = sizeof(FGPUSimpleInstanceData) * (MaxChunkCount + MaxEmptyChunkCount),
				.data = nullptr,
				.debugName = "Buffer: instance of visible chunk debug"
			},
			nullptr);
		DescDebugInstance = FGPUSimpleInstanceData::GetInstanceDescriptor();
		RPDebugInstance =
		{
			.color =
			{
				{
					.loadOp = lvk::LoadOp_Load,
					.storeOp = lvk::StoreOp_Store,
					.clearColor = {0.0f, 0.0f, 0.0f, 1.0f}
				}
			},
			.depth =
			{
				.loadOp = lvk::LoadOp_Load,
				.storeOp = lvk::StoreOp_Store,
				.clearDepth = 1.0f,
			}
		};
		ShaderWireFrameVSInstance.UpdateShaderHandle(LVKContext);
		ShaderWireFrameFSInstance.UpdateShaderHandle(LVKContext);
	}
	void InitializeDebugFrameBuffer(lvk::IContext* LVKContext, const lvk::TextureHandle& DebugCanvas, const lvk::TextureHandle& DebugCanvasDepth)
	{
		FBDebugInstance =
		{
			.color = {{.texture = DebugCanvas}},
			.depthStencil = {.texture = DebugCanvasDepth},
		};
		lvk::RenderPipelineDesc DebugInstanceDescriptor =
		{
			.vertexInput = FGPUSimpleInstanceData::GetInstanceDescriptor(),
			.smVert = ShaderWireFrameVSInstance.SMHandle,
			.smFrag = ShaderWireFrameFSInstance.SMHandle,
			.color = {{.format = LVKContext->getFormat(FBDebugInstance.color[0].texture)}},
			.depthFormat = LVKContext->getFormat(FBDebugInstance.depthStencil.texture), //??
			.cullMode = lvk::CullMode_Back,
			.frontFaceWinding = lvk::WindingMode_CCW,
			.polygonMode = lvk::PolygonMode_Line,
			.samplesCount = 1,
			.debugName = "Pipeline: debug chunk (wireframe)",
		};
		RPLDebugInstance = LVKContext->createRenderPipeline(DebugInstanceDescriptor, nullptr);
	}

	template<typename T>
	inline void PushToPool(uint32_t MaxChunkCount, TThreadSafeMemoryPool<T>& MemoryPool, uint32_t ReservedMemoryPoolIndex, T&& NewItem, const EChunkState& NewState, const FImportanceComputeInfo& CameraInfo)
	{
		static_assert(std::is_base_of_v<FChunkBase, T>, "T must be derived from FChunkBase");
		//Unload, clear old
		const ivec3 OldLocation = MemoryPool[ReservedMemoryPoolIndex].ChunkLocation;
		const ivec3 NewLocation = NewItem.ChunkLocation;

		//If old location's importance larger than new location's
		if (CameraInfo.CalculateImportance(OldLocation) >= CameraInfo.CalculateImportance(NewLocation))
		{
			ChunksLookupTable.ATOMIC_remove(NewLocation); //Remove new reserved location
			MemoryPool.Release(ReservedMemoryPoolIndex); //Remove locked memory pool
			return;
		}
		else//If can override
		{
			//If Chunk is not reading, then remove old chunk
			if (!ChunksLookupTable.ATOMIC_remove_insert_by_condition(OldLocation, NewLocation, NewState, [](EChunkState State) { return !EChunkStateUtils::bHasState(State, EChunkState::Reading); }))
			{
				MemoryPool.Release(ReservedMemoryPoolIndex);
				return;
			}
		}
		//Load, add new
		//ChunksLookupTable.ATOMIC_remove_and_insert(OldLocation, NewLocation, NewState);
		MemoryPool[ReservedMemoryPoolIndex] = std::move(NewItem);
		MemoryPool.Release(ReservedMemoryPoolIndex);
		//Mark dirty
		bAtomicDebugVisibleChunkDirty.store(true);
		bAtomicVisibleChunkDirty.store(true);
	}
	inline void PushChunk(FChunk&& NewChunk, uint32_t ReservedMemoryPoolIndex, const FImportanceComputeInfo& CameraInfo)
	{
		PushToPool<FChunk>(MaxChunkCount, AtomicChunksPool, ReservedMemoryPoolIndex, std::move(NewChunk), EChunkState::NonEmpty, CameraInfo);
	}
	inline void PushEmptyChunk(FEmptyChunk&& NewEmptyChunk, uint32_t ReservedEmptyMemoryPoolIndex, const FImportanceComputeInfo& CameraInfo)
	{
		PushToPool<FEmptyChunk>(MaxEmptyChunkCount, AtomicEmptyChunksPool, ReservedEmptyMemoryPoolIndex, std::move(NewEmptyChunk), EChunkState::Empty, CameraInfo);
	}
	//
	std::vector<FGPUSimpleInstanceData> GetDebugInstanceInfo(const FVoxelSceneConfig& VoxelSceneConfig) const
	{
		std::vector<FGPUSimpleInstanceData> Result;
		//
		std::map<ivec3, EChunkState, FIVec3Comparator> ChunksLookupTableCopy = ChunksLookupTable.ATOMIC_get_copy();
		//
		Result.reserve(ChunksLookupTableCopy.size());
		float ChunkSize = VoxelSceneConfig.GetChunkSize();
		for (const auto& Chunk : ChunksLookupTableCopy)
		{
			Result.push_back(
				{
					.Position = {ChunkSize,ChunkSize,ChunkSize},
					.ChunkLocation = Chunk.first,
					.Scale = ChunkSize * 0.1f,
					.Marker = 0.0,
				});
		}
		return Result;
	}
	void UpdateDebugVisibleChunk(lvk::IContext* LVKContext, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		if (!bDebugGatherChunk)
		{
			return;
		}
		bool Expected = true;
		if (bAtomicDebugVisibleChunkDirty.compare_exchange_strong(Expected, false))// Compare, Release
		{
			DebugTimerSet.Start(DebugMarkGatherVisibleChunk);
			DebugInstanceBufferCPU = GetDebugInstanceInfo(VoxelSceneConfig);
			DebugTimerSet.Record(DebugMarkGatherVisibleChunk);
			//
			CurrentDebugDrawInstanceCount = (uint32_t)DebugInstanceBufferCPU.size();
			if (CurrentDebugDrawInstanceCount > VoxelSceneConfig.MaxEmptyChunkCount + VoxelSceneConfig.MaxChunkCount)
			{
				printf("UpdateDebugVisibleChunk: CurrentDebugDrawInstanceCount larger than (MaxEmptyChunkCount + MaxChunkCount).\n");
				CurrentDebugDrawInstanceCount = VoxelSceneConfig.MaxEmptyChunkCount + VoxelSceneConfig.MaxChunkCount;
			}
			DebugTimerSet.Start(DebugMarkUploadVisibleChunk);
			LVKContext->upload(DebugInstanceBuffer, DebugInstanceBufferCPU.data(), sizeof(FGPUSimpleInstanceData) * CurrentDebugDrawInstanceCount);
			DebugTimerSet.Record(DebugMarkUploadVisibleChunk);
		}
	}
};
