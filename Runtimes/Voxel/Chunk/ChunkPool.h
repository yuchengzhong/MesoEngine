// Meso Engine 2024
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
#include "Thread/MemoryPool.h"
#include "Thread/ThreadSafeQueue.h"
#include "Thread/DoubleBufferQueue.h"

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
	Computing = 1 << 0,
	NonEmpty = 1 << 1,
	Empty = 1 << 2,
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

class FTLSModifyBuffer
{
public:
	FChunk ModifyChunk;
	uint32_t ModifyChunkIndex = INT_MAX;

	FEmptyChunk ModifyEmptyChunk;
	uint32_t ModifyEmptyChunkIndex = INT_MAX;

	FGPUChunk ModifyGPUChunk;
	uint32_t ModifyGPUChunkIndex = INT_MAX;

	FGPUSimpleInstanceData ModifyGPUInstance;
	uint32_t ModifyGPUInstanceIndex = INT_MAX;

	std::vector<FGPUBlock> ModifyGPUBlock;
	std::vector<uint32_t> ModifyGPUBlockIndex;
};

class FTLSChunkPool
{
public:
	using FModifyBufferQueue = TDoubleBufferQueue<FTLSModifyBuffer>;
	std::vector<FChunk> ChunksPool;
	uint32_t CurrentChunkIndex = 0;

	std::vector<FEmptyChunk> EmptyChunksPool;
	uint32_t CurrentEmptyChunkIndex = 0;

	std::vector<FGPUChunk> GPUChunksPool; //simulate gpu chunk first
	std::vector<FGPUBlock> GPUBlock;

	std::vector<FGPUSimpleInstanceData> GPUInstanceData;
	uint32_t CurrentGPUInstanceIndex = 0;

	uint32_t SubMaxChunkCount = 0;
	uint32_t SubMaxEmptyChunkCount = 0;
	uint32_t SubMaxBlockCount = 0;
	uint32_t SubMaxGPUInstanceCount = 0;

	uint32_t ChunkCountOffset = 0;
	uint32_t EmptyChunkCountOffset = 0;
	uint32_t BlockCountOffset = 0;
	uint32_t GPUInstanceOffset = 0;

	uint32_t SubCurrentDebugDrawInstanceCount = 0;
	FTLSChunkPool()
	{

	}
	void Initialize(const uint32_t& SubMaxChunkCount_, const uint32_t& SubMaxEmptyChunkCount_, const uint32_t& SubMaxBlockCount_, 
		const uint32_t& ChunkCountOffset_, const uint32_t& EmptyChunkCountOffset_, const uint32_t& BlockCountOffset_)
	{
		SubMaxChunkCount = SubMaxChunkCount_;
		SubMaxEmptyChunkCount = SubMaxEmptyChunkCount_;
		SubMaxBlockCount = SubMaxBlockCount_;
		SubMaxGPUInstanceCount = SubMaxChunkCount + SubMaxEmptyChunkCount;

		ChunkCountOffset = ChunkCountOffset_;
		EmptyChunkCountOffset = EmptyChunkCountOffset_;
		BlockCountOffset = BlockCountOffset_;
		GPUInstanceOffset = ChunkCountOffset + EmptyChunkCountOffset;

		ChunksPool.resize(SubMaxChunkCount);
		EmptyChunksPool.resize(SubMaxEmptyChunkCount);

		GPUChunksPool.resize(SubMaxChunkCount);
		GPUBlock.resize(SubMaxBlockCount);

		FGPUSimpleInstanceData DefaultInstanceData = { .ChunkLocation = {INT_MAX,INT_MAX,INT_MAX} };
		GPUInstanceData.resize(SubMaxChunkCount + SubMaxEmptyChunkCount_, DefaultInstanceData);
	}
	void ConsumeQueue(FModifyBufferQueue& Queue)
	{
		const uint32_t QueueSize = Queue.Size();
		for (uint32_t i = 0; i < QueueSize; i++)
		{
			FTLSModifyBuffer CurrentModifyBuffer;
			if (!Queue.Pop(CurrentModifyBuffer))
			{
				break;
			}
			if (CurrentModifyBuffer.ModifyChunkIndex != INT_MAX)
			{
				ChunksPool[CurrentModifyBuffer.ModifyChunkIndex] = CurrentModifyBuffer.ModifyChunk;//todo:move
			}
			if (CurrentModifyBuffer.ModifyEmptyChunkIndex != INT_MAX)
			{
				EmptyChunksPool[CurrentModifyBuffer.ModifyEmptyChunkIndex] = CurrentModifyBuffer.ModifyEmptyChunk;
			}
			if (CurrentModifyBuffer.ModifyGPUChunkIndex != INT_MAX)
			{
				GPUChunksPool[CurrentModifyBuffer.ModifyGPUChunkIndex] = CurrentModifyBuffer.ModifyGPUChunk;
			}
			if (CurrentModifyBuffer.ModifyGPUInstanceIndex != INT_MAX)
			{
				GPUInstanceData[CurrentModifyBuffer.ModifyGPUInstanceIndex] = CurrentModifyBuffer.ModifyGPUInstance;
			}
			for (uint32_t i = 0; i < CurrentModifyBuffer.ModifyGPUBlock.size(); i++)
			{
				GPUBlock[CurrentModifyBuffer.ModifyGPUBlockIndex[i]] = CurrentModifyBuffer.ModifyGPUBlock[i];
			}
		}
	}
};
class FChunkPool
{
public:
	std::vector<FTLSChunkPool> TLSChunkPool;
	std::vector<std::unique_ptr<FTLSChunkPool::FModifyBufferQueue>> TLSChunkPoolModifyBufferQueue;
	std::vector<FTLSChunkPool> TLSChunkPoolRead;
	uint32_t ThreadCount = 0;
	//For multi thread
	using FChunkLookupTable = TThreadSafeMap<ivec3, EChunkState, FIVec3Comparator>;
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
	uint32_t MaxBlockCount = 0;
	//Try
	uint32_t MaxChunkCheckTimes = 0;
	uint32_t MaxEmptyChunkCheckTimes = 0;
	uint32_t MaxBlockCheckTimes = 0;

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
	void Initialize(lvk::IContext* LVKContext, const FVoxelSceneConfig& VoxelSceneConfig, uint32_t ThreadCount_)
	{
		ThreadCount = ThreadCount_;
		AtomicVisibilityChunkFrameStamp.store(0);
		MaxChunkCount = VoxelSceneConfig.MaxChunkCount;
		MaxEmptyChunkCount = VoxelSceneConfig.MaxEmptyChunkCount;
		MaxBlockCount = VoxelSceneConfig.MaxBlockCount;

		TLSChunkPool.resize(ThreadCount);
		TLSChunkPoolRead.resize(ThreadCount);
		for (size_t i = 0; i < ThreadCount; ++i) 
		{
			TLSChunkPoolModifyBufferQueue.push_back(std::make_unique<FTLSChunkPool::FModifyBufferQueue>());
		}
		uint32_t AvgSubMaxChunkCount = MaxChunkCount / ThreadCount;
		uint32_t AvgSubMaxEmptyChunkCount = MaxEmptyChunkCount / ThreadCount;
		uint32_t AvgSubMaxBlockCount = MaxBlockCount / ThreadCount;

		MaxChunkCheckTimes = std::max(1u, VoxelSceneConfig.MaxChunkCheckTimes);
		MaxEmptyChunkCheckTimes = std::max(1u, VoxelSceneConfig.MaxEmptyChunkCheckTimes);
		MaxBlockCheckTimes = std::max(1u, VoxelSceneConfig.MaxBlockCheckTimes);
		for (uint32_t i = 0; i < ThreadCount; i++)
		{
			uint32_t SubMaxChunkCountStart = AvgSubMaxChunkCount * i;
			uint32_t SubMaxChunkCountEnd = (i == ThreadCount - 1) ? std::max(AvgSubMaxChunkCount * (i + 1), MaxChunkCount) : AvgSubMaxChunkCount * (i + 1);

			uint32_t SubMaxEmptyChunkCountStart = AvgSubMaxEmptyChunkCount * i;
			uint32_t SubMaxEmptyChunkCountEnd = (i == ThreadCount - 1) ? std::max(AvgSubMaxEmptyChunkCount * (i + 1), MaxEmptyChunkCount) : AvgSubMaxEmptyChunkCount * (i + 1);

			uint32_t SubMaxMaxBlockCountStart = AvgSubMaxBlockCount * i;
			uint32_t SubMaxMaxBlockCountEnd = (i == ThreadCount - 1) ? std::max(AvgSubMaxBlockCount * (i + 1), MaxBlockCount) : AvgSubMaxBlockCount * (i + 1);

			TLSChunkPool[i].Initialize(
				(SubMaxChunkCountEnd - SubMaxChunkCountStart), (SubMaxEmptyChunkCountEnd - SubMaxEmptyChunkCountStart), (SubMaxMaxBlockCountEnd - SubMaxMaxBlockCountStart),
				SubMaxChunkCountStart, SubMaxEmptyChunkCountStart, SubMaxMaxBlockCountStart);
			TLSChunkPoolRead[i].Initialize(
				(SubMaxChunkCountEnd - SubMaxChunkCountStart), (SubMaxEmptyChunkCountEnd - SubMaxEmptyChunkCountStart), (SubMaxMaxBlockCountEnd - SubMaxMaxBlockCountStart),
				SubMaxChunkCountStart, SubMaxEmptyChunkCountStart, SubMaxMaxBlockCountStart);
		}
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
	inline void PushToPool(uint32_t MaxChunkCount, FTLSChunkPool& MemoryPool, FTLSChunkPool::FModifyBufferQueue& ModifyQueue, 
		uint32_t CheckTimes, T&& NewItem, const EChunkState& NewState, 
		const FImportanceComputeInfo& CameraInfo, const float ChunkSize, const EChunkOverrideMode OverrideMode)
	{
		static_assert(std::is_base_of_v<FChunkBase, T>, "T must be derived from FChunkBase");
		const ivec3 NewLocation = NewItem.ChunkLocation;
		auto NewImportance = CameraInfo.CalculateImportance(NewLocation);

		auto HelperGetChunk = [&]() -> T& 
			{
				if constexpr (std::is_same_v<T, FChunk>) 
				{
					return MemoryPool.ChunksPool[MemoryPool.CurrentChunkIndex];
				}
				else
				{
					return MemoryPool.EmptyChunksPool[MemoryPool.CurrentEmptyChunkIndex];
				}
			};
		auto HelperSetIndex = [&](uint32_t DesiredIndex)
			{
				if constexpr (std::is_same_v<T, FChunk>) 
				{
					MemoryPool.CurrentChunkIndex = DesiredIndex;
				}
				else
				{
					MemoryPool.CurrentEmptyChunkIndex = DesiredIndex;
				}
			};

		auto HelperIncrementIndex = [&]()
			{
				if constexpr (std::is_same_v<T, FChunk>) 
				{
					MemoryPool.CurrentChunkIndex = (MemoryPool.CurrentChunkIndex + 1) % MemoryPool.SubMaxChunkCount;
				}
				else 
				{
					MemoryPool.CurrentEmptyChunkIndex = (MemoryPool.CurrentEmptyChunkIndex + 1) % MemoryPool.SubMaxEmptyChunkCount;
				}
			};
		auto HelperGetCurrentIndex = [&]()
			{
				if constexpr (std::is_same_v<T, FChunk>) 
				{
					return MemoryPool.CurrentChunkIndex;
				}
				else 
				{
					return MemoryPool.CurrentEmptyChunkIndex;
				}
			};
		//TODO: Merge this, now the FindLess mode
		// OverrideMode
		float MinImportance = 1e10;
		uint32_t OverrideLocationIndex = INT_MAX;
		ivec3 OverrideOldLocation;
		bool OverrideInvalidIndex = false;
		FTLSModifyBuffer ModifyBuffer;
		for (uint32_t i = 0; i < CheckTimes; i++)
		{
			auto& CurrentChunk = HelperGetChunk();
			const ivec3 OldLocation = CurrentChunk.ChunkLocation;
			if (CurrentChunk.FChunkBase::bIsValid())
			{
				const float OldImportance = CameraInfo.CalculateImportance(OldLocation);
				// If old location's importance larger than new location's
				// If using regressive mode, even tho old chunk is more importance, it will still override the less importance one
				if (OldImportance >= NewImportance && OverrideMode != EChunkOverrideMode::OverrideMin)
				{
					continue;
				}
				if (OverrideMode == EChunkOverrideMode::FindLess)
				{
					OverrideLocationIndex = HelperGetCurrentIndex();
					OverrideOldLocation = OldLocation;
					OverrideInvalidIndex = false;
					break;
				}
				else if (OverrideMode == EChunkOverrideMode::FindMin || OverrideMode == EChunkOverrideMode::OverrideMin)
				{	
					if (OldImportance < MinImportance)
					{
						OverrideLocationIndex = HelperGetCurrentIndex();
						OverrideOldLocation = OldLocation;
						MinImportance = OldImportance;
					}
				}
			}
			else
			{
				OverrideInvalidIndex = true;
				OverrideLocationIndex = HelperGetCurrentIndex();
				OverrideOldLocation = OldLocation;
				break;
			}
			HelperIncrementIndex();
		}
		if(OverrideLocationIndex != INT_MAX)
		{
			if (!OverrideInvalidIndex)
			{
				MemoryPool.SubCurrentDebugDrawInstanceCount--;
			}
			HelperSetIndex(OverrideLocationIndex);
			auto& CurrentChunk = HelperGetChunk();
			MemoryPool.SubCurrentDebugDrawInstanceCount++;
			ChunksLookupTable.ATOMIC_remove_and_insert(OverrideOldLocation, NewLocation, NewState);
			//
			FGPUSimpleInstanceData NewInstanceData =
			{
				.Position = {ChunkSize,ChunkSize,ChunkSize},
				.ChunkLocation = NewLocation,
				.Scale = ChunkSize * 0.1f,
				.Marker = (std::is_same_v<T, FChunk>) ? 1.0f : 0.0f,
			};
			ModifyBuffer.ModifyGPUInstance = NewInstanceData; //copy
			ModifyBuffer.ModifyGPUInstanceIndex = MemoryPool.CurrentGPUInstanceIndex;
			MemoryPool.GPUInstanceData[MemoryPool.CurrentGPUInstanceIndex] = std::move(NewInstanceData); //move
			MemoryPool.CurrentGPUInstanceIndex = (MemoryPool.CurrentGPUInstanceIndex + 1) % MemoryPool.SubMaxGPUInstanceCount;
			//
			if constexpr (std::is_same_v<T, FChunk>)
			{
				ModifyBuffer.ModifyChunk = NewItem; //Copy
				ModifyBuffer.ModifyChunkIndex = OverrideLocationIndex;
			}
			else
			{
				ModifyBuffer.ModifyEmptyChunk = NewItem; //Copy
				ModifyBuffer.ModifyEmptyChunkIndex = OverrideLocationIndex;
			}
			CurrentChunk = std::move(NewItem); // Move
			HelperIncrementIndex();
			// Push modify buffer to front
			ModifyQueue.Push(std::move(ModifyBuffer));
			// Mark dirty
			bAtomicDebugVisibleChunkDirty.store(true);
			bAtomicVisibleChunkDirty.store(true);
			return;
		}
		else
		{
			//Fail
			ChunksLookupTable.ATOMIC_remove(NewLocation); //Remove new reserved location
		}
	}
	inline void PushChunk(FChunk&& NewChunk, uint32_t ThreadId, const FImportanceComputeInfo& CameraInfo, const float ChunkSize, const EChunkOverrideMode OverrideMode)
	{
		PushToPool<FChunk>(MaxChunkCount, TLSChunkPool[ThreadId], *TLSChunkPoolModifyBufferQueue[ThreadId], MaxChunkCheckTimes, std::move(NewChunk), EChunkState::NonEmpty, CameraInfo, ChunkSize, OverrideMode);
	}
	inline void PushEmptyChunk(FEmptyChunk&& NewEmptyChunk, uint32_t ThreadId, const FImportanceComputeInfo& CameraInfo, const float ChunkSize, const EChunkOverrideMode OverrideMode)
	{
		PushToPool<FEmptyChunk>(MaxEmptyChunkCount, TLSChunkPool[ThreadId], *TLSChunkPoolModifyBufferQueue[ThreadId], MaxEmptyChunkCheckTimes, std::move(NewEmptyChunk), EChunkState::Empty, CameraInfo, ChunkSize, OverrideMode);
	}
	//
	//
	void GatherDebugInstanceInfo(const FVoxelSceneConfig& VoxelSceneConfig) // this can also be done in multi-thread
	{
		CurrentDebugDrawInstanceCount = 0;
		for (uint32_t i = 0; i < ThreadCount; i++)
		{
			TLSChunkPoolModifyBufferQueue[i]->Swap();
			TLSChunkPoolRead[i].ConsumeQueue(*TLSChunkPoolModifyBufferQueue[i]);
			CurrentDebugDrawInstanceCount += TLSChunkPool[i].SubCurrentDebugDrawInstanceCount;
		}
	}
	void UploadDebugInstanceInfo(lvk::IContext* LVKContext, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		for (uint32_t i = 0; i < ThreadCount; i++)
		{
			LVKContext->upload(DebugInstanceBuffer, TLSChunkPoolRead[i].GPUInstanceData.data(), 
				sizeof(FGPUSimpleInstanceData) * TLSChunkPoolRead[i].GPUInstanceData.size(), 
				sizeof(FGPUSimpleInstanceData) * TLSChunkPoolRead[i].GPUInstanceOffset);
		}
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
			GatherDebugInstanceInfo(VoxelSceneConfig);
			DebugTimerSet.Record(DebugMarkGatherVisibleChunk);

			DebugTimerSet.Start(DebugMarkUploadVisibleChunk);
			UploadDebugInstanceInfo(LVKContext, VoxelSceneConfig);
			DebugTimerSet.Record(DebugMarkUploadVisibleChunk);
		}
	}
};
