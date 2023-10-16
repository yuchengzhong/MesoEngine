#pragma once
#include <vector>
#include <atomic>
#include <queue>
#include <set>
#include <map>
#include <functional>
#include <climits>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <lvk/LVK.h>

#include "Voxel/VoxelSceneConfig.h"
#include "Voxel/Block/Block.h"
#include "Helper/ColorHelper.h"
#include "Helper/Timer.h"
#include "Helper/TimerSet.h"
#include "Thread/ThreadPool.h"
#include "Shader/GPUStructures.h"
#include "ChunkManagerHelper.h"
#include "Chunk.h"
#include "ChunkPool.h"
using glm::ivec3;
using glm::ivec4;
using glm::vec3;
using glm::u8vec3;
using glm::u8vec4;

class FChunkManage
{
	uint32_t BakeVisibilityViewNum = 0;

	uint32_t DebugVisibleChunkNum = 0;
	uint32_t DebugNewVisibleChunkNum = 0;
	uint32_t DebugMaxVisibleChunkNum = 0;
	uint32_t DebugMaxSyncedLoadChunkNum = 0;

	double DebugAllSyncedGenerationTime = 0.0;
	uint32_t DebugAllSyncedGeneratedChunkNum = 0;

	double DebugAllMultiThreadGenerationTime = 0.0;
	uint32_t DebugAllMultiThreadGeneratedChunkNum = 0;

	inline static std::string DebugMarkFindAllVisibleChunkTime = "FindAllVisibleChunkTime";
	inline static std::string DebugMarkGatherAllChunkTime = "GatherAllChunkTime";

	bool bDebugDisableUpdateChunk = false;
	bool bDebugMultiThreading = false;
	bool bDebugVisibleChunk = true;
	// Debug
	FTimerSet DebugTimerSet;
	/* 
	Generator, takes in start location, block size, chunk resolution, uint32_t mipmap_level; 
	Start location lays in the most [left down back] voxel's most [left down back]'s vertex location
	*/
	using GeneratorType = std::function<FChunk(ivec3, float, unsigned char, uint32_t)>;
	GeneratorType Generator;
	void SetGenerator(GeneratorType Generator_)
	{
		Generator = std::move(Generator_);
	}

	TNearestMap<FChunkManageHelper::FImportanceChunkQueue> BakedVisibility;
public:
	ThreadPool GeneratorThreadPool;
	//Pool
	FChunkPool ChunkPool;
	//Queue
	std::queue<ivec3> DesiredToLoadChunkLocations;
	//GPU
	lvk::Holder<lvk::BufferHandle> ChunkBuffer;
	std::vector<FGPUChunkData> ChunkBufferCPU;
	uint32_t CurrentChunkCount = 0;
	//
	//Wait for all thread finish
	FChunkManage()
	{

	}
	~FChunkManage()
	{
		GeneratorThreadPool.WaitForTasksToComplete();
	}
	void Initialize(lvk::IContext* LVKContext, const FVoxelSceneConfig& VoxelSceneConfig, GeneratorType Generator_)
	{
		uint32_t MaxCPUCoreNum = boost::thread::hardware_concurrency();
		const uint32_t ThreadCount = std::min(std::max(MaxCPUCoreNum - 4u, 1u), VoxelSceneConfig.MaxUnsyncedLoadChunkCount);
		GeneratorThreadPool.Initialize(ThreadCount);// leave some cores for youtube
		ChunkPool.Initialize(LVKContext, VoxelSceneConfig, ThreadCount);
		//
		SetGenerator(std::move(Generator_));
		//Bake visibility
		BakeVisibilityViewNum = VoxelSceneConfig.BakeVisibilityViewNum;
		//BakedVisibility = FChunkManageHelper::BakeVisibilityByView(VoxelSceneConfig, BakeVisibilityViewNum);
		BakedVisibility = FChunkManageHelper::BakeVisibilityByView(VoxelSceneConfig, BakeVisibilityViewNum);


		ChunkBuffer = LVKContext->createBuffer(
			{
				.usage = lvk::BufferUsageBits_Storage,
				.storage = lvk::StorageType_HostVisible,
				.size = sizeof(FGPUChunkData) * VoxelSceneConfig.MaxChunkCount,
				.data = nullptr,
				.debugName = "Buffer: for storing chunk instance data"
			},
			nullptr);
	}
	// This is slow
	inline std::queue<ivec3> GetDesiredShowChunkLocation(ivec3 ChunkLocation, vec3 ForwardVector, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		DebugTimerSet.Start(DebugMarkFindAllVisibleChunkTime);
		std::queue<ivec3> Result;//More importance more front
		FChunkManageHelper::FImportanceChunkQueue ImportancePriorityQueue;
		if (BakeVisibilityViewNum <= 0)
		{
			ImportancePriorityQueue = FChunkManageHelper::GetDesiredShowChunkLocationByView(ForwardVector, VoxelSceneConfig);
		}
		else
		{
			ImportancePriorityQueue = BakedVisibility.Query(ForwardVector);
		}
		while (!ImportancePriorityQueue.empty())
		{
			Result.push(ChunkLocation + ImportancePriorityQueue.top().second);
			//printf("Pushed: %d,%d,%d\n", ImportancePriorityQueue.top().second.x, ImportancePriorityQueue.top().second.y, ImportancePriorityQueue.top().second.z);
			ImportancePriorityQueue.pop();
		}
		if ((uint32_t)Result.size() > VoxelSceneConfig.MaxChunkCount)
		{
			//Warning
			//printf("GetDesiredShowChunkLocation: Warning, single frame chunk needed to load (%d) exceed MaxChunkCount (%d)\n", (uint32_t)Result.size(), VoxelSceneConfig.MaxChunkCount);
		}
		DebugTimerSet.Record(DebugMarkFindAllVisibleChunkTime);
		//Debug
		DebugVisibleChunkNum = (uint32_t)Result.size();
		DebugMaxVisibleChunkNum = VoxelSceneConfig.MaxChunkCount;
		DebugMaxSyncedLoadChunkNum = VoxelSceneConfig.MaxSyncedLoadChunkCount;
		return Result;//rvo
	}
private:
	struct FUpdateChunksCacheType
	{
		ivec3 NewChunkLocation = {};
		vec3 NewForwardVector = {};
		FVoxelSceneConfig VoxelSceneConfig = {};
	} UpdateChunksCache;
public:
	void UpdateChunks(ivec3 NewChunkLocation, vec3 NewForwardVector, const FVoxelSceneConfig& VoxelSceneConfig) //If view dir, chunk is changed
	{
		UpdateChunksCache =
		{
			.NewChunkLocation = NewChunkLocation,
			.NewForwardVector = NewForwardVector,
			.VoxelSceneConfig = VoxelSceneConfig,
		};
		if (bDebugDisableUpdateChunk)
		{
			return;
		}
		if (!Generator)
		{
			//Warning
			printf("UpdateChunks: Generator is empty.\n");
			return;
		}
		//Query every
		//Old Chunk...
		//New Chunk...
		//Overlap no need to count
		DesiredToLoadChunkLocations = GetDesiredShowChunkLocation(NewChunkLocation, NewForwardVector, VoxelSceneConfig);
		ChunkPool.IncreaseFrameStamp();
	}
	void MultiThreadGenerator(const ivec3 CurrentDesiredChunkLocation, const uint32_t MipmapLevel, const FImportanceComputeInfo& CameraInfo, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		const uint32_t ThreadId = GeneratorThreadPool.GetCurrentThreadID();
		if (ThreadId > GeneratorThreadPool.GetSize())
		{
			printf("Unknown thread id %d, max %d\n", ThreadId, GeneratorThreadPool.GetSize());
		}
		FChunk NewChunk = Generator(CurrentDesiredChunkLocation, VoxelSceneConfig.BlockSize, VoxelSceneConfig.ChunkResolution, MipmapLevel);
		NewChunk.ChunkLocation = CurrentDesiredChunkLocation;// just make sure
		uint32_t ReservedMemoryPoolIndex = 0;
		bool bChunkEmpty = NewChunk.Blocks.size() <= 0;
		if (bChunkEmpty) //Empty
		{
			FEmptyChunk NewEmptyChunk;
			NewEmptyChunk.ChunkLocation = CurrentDesiredChunkLocation;//Just ensure

			bool bMemoryPoolValid = ChunkPool.AtomicEmptyChunksPool.Acquire(ReservedMemoryPoolIndex, ThreadId);
			if (!bMemoryPoolValid)
			{
				ChunkPool.ChunksLookupTable.ATOMIC_remove(CurrentDesiredChunkLocation);
				return;
			}
			ChunkPool.PushEmptyChunk(std::move(NewEmptyChunk), ReservedMemoryPoolIndex, CameraInfo);
		}
		else
		{
			bool bMemoryPoolValid = ChunkPool.AtomicChunksPool.Acquire(ReservedMemoryPoolIndex, ThreadId);
			if (!bMemoryPoolValid)
			{
				ChunkPool.ChunksLookupTable.ATOMIC_remove(CurrentDesiredChunkLocation);
				return;
			}
			ChunkPool.PushChunk(std::move(NewChunk), ReservedMemoryPoolIndex, CameraInfo);
		}
	}
	void UpdateLoadingQueue(lvk::IContext* LVKContext, ivec3 CameraChunkLocation, vec3 CameraForwardVector, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		uint32_t CurrentSyncedChunkCount = 0;
		double DeltaSyncedTime = 0;
		uint32_t CurrentMultiThreadChunkCount = 0;
		double DeltaMultiThreadTime = 0;
		FTimer Timer;

		const uint64_t CurrentTaskFrameStamp = ChunkPool.AtomicGetCurrentChunkFrameStamp();
		uint32_t TotalNum = DesiredToLoadChunkLocations.size();

		FImportanceComputeInfo CameraInfo = 
		{ 
			.CameraChunk = CameraChunkLocation , 
			.CameraForwardVector = CameraForwardVector 
		};
		for (uint32_t i = 0; i < TotalNum; i++)
		{
			ivec3 CurrentDesiredChunkLocation = DesiredToLoadChunkLocations.front();
			uint32_t MipmapLevel = 0;
			EChunkState OldState = EChunkState::Computing;
			if (ChunkPool.ChunksLookupTable.ATOMIC_not_contains_insert(CurrentDesiredChunkLocation, EChunkState::Computing, OldState)) //Not found
			{
				if (CurrentSyncedChunkCount >= VoxelSceneConfig.MaxSyncedLoadChunkCount && CurrentMultiThreadChunkCount >= VoxelSceneConfig.MaxUnsyncedLoadChunkCount)//If reach limit
				{
					goto FailedToDispatch;
				}
				if (CurrentSyncedChunkCount < (VoxelSceneConfig.MaxSyncedLoadChunkCount))
				{
					// Main thread load
					Timer.Start();
					MultiThreadGenerator(CurrentDesiredChunkLocation, MipmapLevel, CameraInfo, VoxelSceneConfig);
					CurrentSyncedChunkCount++;
					DeltaSyncedTime += Timer.Step();
					DesiredToLoadChunkLocations.pop();
				}
				else // Multi-thread load
				{
					//Dispatch MultiThreadGenerator
					Timer.Start();
					auto BoundFunction = boost::bind(&FChunkManage::MultiThreadGenerator, this, _1, _2, _3, _4);
					std::function<void()> TaskFunc = boost::bind(BoundFunction, CurrentDesiredChunkLocation, MipmapLevel, CameraInfo, VoxelSceneConfig);
					bool Success = GeneratorThreadPool.EnqueueForward(TaskFunc);
					DeltaMultiThreadTime += Timer.Step();
					if (Success)
					{
						CurrentMultiThreadChunkCount++;
						DesiredToLoadChunkLocations.pop();
					}
					else
					{
						goto FailedToDispatch;
					}
				}
			FailedToDispatch:
				{
					ChunkPool.ChunksLookupTable.ATOMIC_remove(CurrentDesiredChunkLocation);// Modify back
					break;
				}
			}
			else//Already loaded
			{
				DesiredToLoadChunkLocations.pop();
				if (OldState == EChunkState::Empty)
				{
				}
				else if (OldState == EChunkState::NonEmpty)
				{
				}
				else
				{
				}
			}
		}
		const uint32_t CurrentTotallyAddedChunkNum = CurrentSyncedChunkCount + CurrentMultiThreadChunkCount;
		if (CurrentTotallyAddedChunkNum > 0)
		{
			DebugAllSyncedGenerationTime += DeltaSyncedTime;
			DebugAllSyncedGeneratedChunkNum += CurrentSyncedChunkCount;
			DebugAllMultiThreadGenerationTime += DeltaMultiThreadTime;
			DebugAllMultiThreadGeneratedChunkNum += CurrentMultiThreadChunkCount;
			DebugNewVisibleChunkNum = CurrentTotallyAddedChunkNum;
			//printf("Chunk %d Loaded\n", CurrentTotallyAddedChunkNum);
		}
		//Visualize
		UpdateVisibleBlock(LVKContext, VoxelSceneConfig);
		//For Debug
		ChunkPool.UpdateDebugVisibleChunk(LVKContext, VoxelSceneConfig);
	}
	template<typename Struct>
	void DrawDebugVisibleChunk(lvk::IContext* LVKContext, const Struct& PushConstantData)
	{
		//Debug visible chunk
		if (bDebugVisibleChunk)
		{
			lvk::ICommandBuffer& Buffer = LVKContext->acquireCommandBuffer();
			Buffer.cmdBeginRendering(ChunkPool.RPDebugInstance, ChunkPool.FBDebugInstance);
			{
				// Scene
				Buffer.cmdBindRenderPipeline(ChunkPool.RPLDebugInstance);
				Buffer.cmdPushDebugGroupLabel("Render Visibility Chunk Wireframe", 0xff0000ff);
				Buffer.cmdBindDepthState({ .compareOp = lvk::CompareOp_Less, .isDepthWriteEnabled = true });
				Buffer.cmdBindVertexBuffer(0, ChunkPool.OctahedronMesh.VertexBuffer, 0);
				Buffer.cmdBindVertexBuffer(1, ChunkPool.DebugInstanceBuffer, 0);

				Buffer.cmdPushConstants(PushConstantData);
				Buffer.cmdBindIndexBuffer(ChunkPool.OctahedronMesh.IndexBuffer, lvk::IndexFormat_UI16);
				Buffer.cmdDrawIndexed(ChunkPool.OctahedronMesh.GetIndexSize(), ChunkPool.CurrentDebugDrawInstanceCount);
				Buffer.cmdPopDebugGroupLabel();
			}
			Buffer.cmdEndRendering();
			Buffer.transitionToShaderReadOnly(ChunkPool.FBDebugInstance.color[0].texture); //Transit
			LVKContext->submit(Buffer);
		}
	}
	//
	std::vector<FGPUChunkData> GatherChunkInfo()
	{
		std::vector<FGPUChunkData> Result = 
			ChunkPool.AtomicChunksPool.GetValidItem<FGPUChunkData>(
				[](const FChunk& Chunk)
				{
					return FGPUChunkData{ .ChunkLocation = Chunk.ChunkLocation };
				});
		/*
		for (uint32_t i = 0; i < ChunkPool.AtomicChunksPool.Size; i++)
		{
			if (ChunkPool.AtomicChunksPool.TryLock(i))
			{
				if (ChunkPool.AtomicChunksPool.Pool[i].bIsvalid())
				{
					FGPUChunkData ChunkData =
					{
						.ChunkLocation = ChunkPool.AtomicChunksPool.Pool[i].ChunkLocation,
					};
					Result.push_back(std::move(ChunkData));
					ChunkPool.AtomicChunksPool.TryUnlock(i);
				}
				else
				{
					if (!ChunkPool.AtomicChunksPool.TryUnlock(i))
					{
						throw std::runtime_error("Something wrong with unlock AtomicChunksPool.");
					}
				}
			}
		}
		*/
		return Result;
	}
	//For drawing
	//TODO: Stop upload if nothing new
	void UpdateVisibleBlock(lvk::IContext* LVKContext, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		bool Expected = true;
		if (ChunkPool.bAtomicVisibleChunkDirty.compare_exchange_strong(Expected, false))// Compare, Release
		{
			//Update Chunk First
			DebugTimerSet.Start(DebugMarkGatherAllChunkTime);
			ChunkBufferCPU = GatherChunkInfo();
			//printf("%d\n", ChunkBufferCPU.size());
			DebugTimerSet.Record(DebugMarkGatherAllChunkTime);
			//
			/*
			CurrentChunkCount = (uint32_t)ChunkBufferCPU.size();
			if (CurrentDebugDrawInstanceCount > VoxelSceneConfig.MaxEmptyChunkCount + VoxelSceneConfig.MaxChunkCount)
			{
				printf("UpdateDebugVisibleChunk: CurrentDebugDrawInstanceCount larger than (MaxEmptyChunkCount + MaxChunkCount).\n");
				CurrentDebugDrawInstanceCount = VoxelSceneConfig.MaxEmptyChunkCount + VoxelSceneConfig.MaxChunkCount;
			}
			Timer.Start();
			LVKContext->upload(DebugInstanceBuffer, DebugInstanceBufferCPU.data(), sizeof(FGPUSimpleInstanceData) * CurrentDebugDrawInstanceCount);
			DebugUploadVisibleChunkTime += Timer.Step();
			DebugUploadVisibleChunkTimes += 1;
			//Update Block(Visible Only)
			*/
		}
	}

	//GUI
	void RenderManagerInfo()
	{
		ImVec2 NewWindowsPosition = ImVec2(50, 400);
		const float Offset = 200.0f;
		ImGui::SetNextWindowPos(NewWindowsPosition);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, FColorHelper::GetBackgroundGreen());
		ImGui::Begin("Chunk Manager information:", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		if (ImGui::Checkbox("Disable Chunk Updating", &bDebugDisableUpdateChunk))
		{
			if (!bDebugDisableUpdateChunk)
			{
				UpdateChunks(UpdateChunksCache.NewChunkLocation, UpdateChunksCache.NewForwardVector, UpdateChunksCache.VoxelSceneConfig);
			}
		}
		ImGui::Checkbox("Debug Multi-Threading(Slow)", &bDebugMultiThreading);
		ImGui::Checkbox("Debug Visible Chunk", &bDebugVisibleChunk);
		ImGui::Checkbox("Debug Gather Visible Chunk", &ChunkPool.bDebugGatherChunk);
		if (bDebugMultiThreading)
		{
			ImGui::Separator();
			ImGui::Text("Multi-Threading Info:");

			uint32_t LoadingSlotNum = 0xFFFFFFFF;
			uint32_t LoadingEmptySlotNum = 0xFFFFFFFF;
			ImGui::Text("Currently Busy Slot Num:");
			ImGui::SameLine(Offset);
			ImGui::Text("%d", LoadingSlotNum);

			ImGui::Text("Currently Busy Empty Slot Num:");
			ImGui::SameLine(Offset);
			ImGui::Text("%d", LoadingEmptySlotNum);
		}
		ImGui::Separator();
		ImGui::Text("Chunk Info:");

		ImGui::Text("Visibility Frame Identity:");
		ImGui::SameLine(Offset);
		ImGui::Text("%zd", ChunkPool.AtomicGetCurrentChunkFrameStamp());

		ImGui::Text("Visible Chunk:");
		ImGui::SameLine(Offset);
		ImGui::Text("%d", DebugVisibleChunkNum);

		ImGui::Text("Loaded Chunk:");
		ImGui::SameLine(Offset);
		ImGui::Text("%d", ChunkPool.CurrentDebugDrawInstanceCount);

		ImGui::Text("Newly Added Visible Chunk:");
		ImGui::SameLine(Offset);
		ImGui::Text("%d", DebugNewVisibleChunkNum);

		ImGui::Text("Max Synced Load Visible Chunk:");
		ImGui::SameLine(Offset);
		ImGui::Text("%d", DebugMaxSyncedLoadChunkNum);

		ImGui::Separator();
		ImGui::Text("Computation Time:");

		double SyncedGenerationTimePerChunk = DebugAllSyncedGenerationTime / std::max((double)DebugAllSyncedGeneratedChunkNum, 1.0);
		double MultiThreadDispatchTimePerChunk = DebugAllMultiThreadGenerationTime / std::max((double)DebugAllMultiThreadGeneratedChunkNum, 1.0);
		ImGui::Text("Synced Computation Per Chunk:");
		ImGui::SameLine(Offset);
		ImGui::Text("%.4lf ms", SyncedGenerationTimePerChunk * 1000.0);

		ImGui::Text("Multi-Thread Dispatch Per Chunk:");
		ImGui::SameLine(Offset);
		ImGui::Text("%.4lf ms", MultiThreadDispatchTimePerChunk * 1000.0);

		double FindAllVisibleChunkTime = DebugTimerSet.GetAverage(DebugMarkFindAllVisibleChunkTime);
		ImGui::Text("Find All Visible Chunk Time:");
		ImGui::SameLine(Offset);
		ImGui::Text("%.4lf ms", FindAllVisibleChunkTime * 1000.0);

		double GatherVisibleChunkTime = ChunkPool.DebugTimerSet.GetAverage(ChunkPool.DebugMarkGatherVisibleChunk);
		ImGui::Text("Debug Gather Visible Chunk Time:");
		ImGui::SameLine(Offset);
		ImGui::Text("%.4lf ms", GatherVisibleChunkTime * 1000.0);

		double UploadVisibleChunkTime = ChunkPool.DebugTimerSet.GetAverage(ChunkPool.DebugMarkUploadVisibleChunk);
		ImGui::Text("Debug Upload Visible Chunk Time:");
		ImGui::SameLine(Offset);
		ImGui::Text("%.4lf ms", UploadVisibleChunkTime * 1000.0);

		double GatherAllChunkTime = DebugTimerSet.GetAverage(DebugMarkGatherAllChunkTime);
		ImGui::Text("Gather All Chunk Time:");
		ImGui::SameLine(Offset);
		ImGui::Text("%.4lf ms", GatherAllChunkTime * 1000.0);

		ImGui::End();
		ImGui::PopStyleColor();
	}
};