// Meso Engine 2024
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
	inline static std::string DebugMarkSumitRenderingDebugChunkTime = "SumitRenderingDebugChunkTime";

	bool bDebugDisableUpdateChunk = false;
	bool bDebugMultiThreading = false;
	bool bDebugVisibleChunk = true;
	bool bDebugReverseZ = true;
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
	FChunkManageHelper::FImportanceChunkQueue DesiredToLoadChunkLocations;
	std::queue<ivec3> RestDesiredToLoadChunkLocations;
	//GPU
	uint32_t CurrentChunkCount = 0;
	//Buffer Num
	uint32_t BufferedFramesNum = 1;
	uint32_t RenderFrameIndex = 0;
	//Wait for all thread finish
	FChunkManage()
	{

	}
	~FChunkManage()
	{
		GeneratorThreadPool.WaitForTasksToComplete();
	}
	void Initialize(lvk::IContext* LVKContext, const uint32_t& ThreadCount, const FVoxelSceneConfig& VoxelSceneConfig, GeneratorType Generator_, bool bDebugReverseZ_ = true, uint32_t BufferedFramesNum_ = 1)
	{
		bDebugReverseZ = bDebugReverseZ_;
		BufferedFramesNum = BufferedFramesNum_;

		GeneratorThreadPool.Initialize(ThreadCount);// leave some cores for youtube
		ChunkPool.Initialize(LVKContext, VoxelSceneConfig, ThreadCount, bDebugReverseZ, BufferedFramesNum);
		//
		SetGenerator(std::move(Generator_));
		//Bake visibility
		BakeVisibilityViewNum = VoxelSceneConfig.BakeVisibilityViewNum;
		BakedVisibility = FChunkManageHelper::BakeVisibilityByView(VoxelSceneConfig, BakeVisibilityViewNum);
	}
private:
	FChunkManageHelper::FImportanceChunkQueue DummyThisFrameChunkQueue;
public:
	// This is slow
	inline FChunkManageHelper::FImportanceChunkQueue& GetDesiredShowChunkLocation(ivec3 ChunkLocation, vec3 ForwardVector, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		DebugTimerSet.Start(DebugMarkFindAllVisibleChunkTime);
		FChunkManageHelper::FImportanceChunkQueue& ImportancePriorityQueue = DummyThisFrameChunkQueue;
		if (BakeVisibilityViewNum <= 0)
		{
			DummyThisFrameChunkQueue = FChunkManageHelper::GetDesiredShowChunkLocationByView(ForwardVector, VoxelSceneConfig);//This need a copy
		}
		else
		{
			ImportancePriorityQueue = BakedVisibility.Query(ForwardVector);
		}
		DebugTimerSet.Record(DebugMarkFindAllVisibleChunkTime);
		//Debug
		DebugVisibleChunkNum = (uint32_t)ImportancePriorityQueue.size();
		DebugMaxVisibleChunkNum = VoxelSceneConfig.MaxChunkCount;
		DebugMaxSyncedLoadChunkNum = VoxelSceneConfig.MaxSyncedLoadChunkCount;
		return ImportancePriorityQueue;//rvo
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
		DesiredToLoadChunkLocations = GetDesiredShowChunkLocation(NewChunkLocation, NewForwardVector, VoxelSceneConfig); //copy
		std::queue<ivec3>().swap(RestDesiredToLoadChunkLocations);
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
		NewChunk.CalculateOccupancyErodeMipmaps(VoxelSceneConfig.ChunkResolution, VoxelSceneConfig.ChunkOccupancyDepth);//Calculate inner properties
		bool bChunkEmpty = NewChunk.Blocks.size() <= 0;
		if (bChunkEmpty) //Empty
		{
			FEmptyChunk NewEmptyChunk;
			NewEmptyChunk.ChunkLocation = CurrentDesiredChunkLocation;//Just ensure

			ChunkPool.PushEmptyChunk(std::move(NewEmptyChunk), ThreadId, ChunkPool.GetFrameStamp(), VoxelSceneConfig.ChunkResolution, CameraInfo, VoxelSceneConfig.GetChunkSize(), VoxelSceneConfig.ChunkOverrideMode);
		}
		else
		{
			ChunkPool.PushChunk(std::move(NewChunk), ThreadId, ChunkPool.GetFrameStamp(), VoxelSceneConfig.ChunkResolution, CameraInfo, VoxelSceneConfig.GetChunkSize(), VoxelSceneConfig.ChunkOverrideMode);
		}
	}
	void MultiThreadGeneratorBatched(const std::vector<ivec3> CurrentDesiredChunkLocations, const std::vector<uint32_t> MipmapLevels, const FImportanceComputeInfo& CameraInfo, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		const uint32_t ThreadId = GeneratorThreadPool.GetCurrentThreadID();
		if (ThreadId > GeneratorThreadPool.GetSize())
		{
			printf("Unknown thread id %d, max %d\n", ThreadId, GeneratorThreadPool.GetSize());
		}
		for (uint32_t i=0;i< CurrentDesiredChunkLocations.size();i++)
		{
			const ivec3 CurrentDesiredChunkLocation = CurrentDesiredChunkLocations[i];
			const uint32_t MipmapLevel = MipmapLevels[i];
			FChunk NewChunk = Generator(CurrentDesiredChunkLocation, VoxelSceneConfig.BlockSize, VoxelSceneConfig.ChunkResolution, MipmapLevel);
			NewChunk.ChunkLocation = CurrentDesiredChunkLocation;// just make sure
			NewChunk.CalculateOccupancyErodeMipmaps(VoxelSceneConfig.ChunkResolution, VoxelSceneConfig.ChunkOccupancyDepth);//Calculate inner properties
			bool bChunkEmpty = NewChunk.Blocks.size() <= 0;
			if (bChunkEmpty) //Empty
			{
				FEmptyChunk NewEmptyChunk;
				NewEmptyChunk.ChunkLocation = CurrentDesiredChunkLocation;//Just ensure

				ChunkPool.PushEmptyChunk(std::move(NewEmptyChunk), ThreadId, ChunkPool.GetFrameStamp(), VoxelSceneConfig.ChunkResolution, CameraInfo, VoxelSceneConfig.GetChunkSize(), VoxelSceneConfig.ChunkOverrideMode);
			}
			else
			{
				ChunkPool.PushChunk(std::move(NewChunk), ThreadId, ChunkPool.GetFrameStamp(), VoxelSceneConfig.ChunkResolution, CameraInfo, VoxelSceneConfig.GetChunkSize(), VoxelSceneConfig.ChunkOverrideMode);
			}
		}
	}
	void UpdateLoadingQueue(lvk::IContext* LVKContext, ivec3 CameraChunkLocation, vec3 CameraForwardVector, const FVoxelSceneConfig& VoxelSceneConfig, uint32_t RenderFrameIndex_)
	{
		uint32_t CurrentSyncedChunkCount = 0;
		double DeltaSyncedTime = 0;
		uint32_t CurrentMultiThreadChunkCount = 0;
		double DeltaMultiThreadTime = 0;
		RenderFrameIndex = RenderFrameIndex_;
		FTimer Timer;

		const uint64_t CurrentTaskFrameStamp = ChunkPool.AtomicGetCurrentChunkFrameStamp();
		uint32_t TotalNum = DesiredToLoadChunkLocations.size();

		FImportanceComputeInfo CameraInfo = 
		{ 
			.CameraChunk = CameraChunkLocation , 
			.CameraForwardVector = CameraForwardVector 
		};
		if (VoxelSceneConfig.ChunkTaskPerCore <= 1)
		{
			for (uint32_t i = 0; i < TotalNum; i++)
			{
				ivec3 CurrentDesiredChunkLocation = DesiredToLoadChunkLocations.top().second + CameraChunkLocation;
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
						continue;
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
							continue;
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
				}
			}
		}
		else
		{
			std::vector<ivec3> BatchedChunkLocations;
			std::vector<uint32_t> BatchedMipmapLevels;
			const uint32_t RestChunkNum = RestDesiredToLoadChunkLocations.size();
			for (uint32_t i = 0; i < TotalNum + RestChunkNum; i++)
			{
				const bool bIsResetChunk = RestDesiredToLoadChunkLocations.size() > 0;
				ivec3 CurrentDesiredChunkLocation = bIsResetChunk ? RestDesiredToLoadChunkLocations.front() : (DesiredToLoadChunkLocations.top().second + CameraChunkLocation);
				uint32_t MipmapLevel = 0;
				EChunkState OldState = EChunkState::Computing;

				if (ChunkPool.ChunksLookupTable.ATOMIC_not_contains_insert(CurrentDesiredChunkLocation, EChunkState::Computing, OldState)) //Not found
				{
					BatchedChunkLocations.push_back(CurrentDesiredChunkLocation);
					BatchedMipmapLevels.push_back(MipmapLevel);
					if (CurrentSyncedChunkCount >= VoxelSceneConfig.MaxSyncedLoadChunkCount && CurrentMultiThreadChunkCount >= VoxelSceneConfig.MaxUnsyncedLoadChunkCount) //If reach limit
					{
						goto FailedToDispatchBatch;
					}
					if (CurrentSyncedChunkCount < (VoxelSceneConfig.MaxSyncedLoadChunkCount))
					{
						if (BatchedChunkLocations.size() >= VoxelSceneConfig.ChunkTaskPerCore)
						{
							// Main thread load
							Timer.Start();
							MultiThreadGeneratorBatched(BatchedChunkLocations, BatchedMipmapLevels, CameraInfo, VoxelSceneConfig);
							BatchedChunkLocations.clear();
							BatchedMipmapLevels.clear();
							DeltaSyncedTime += Timer.Step();
						}
						CurrentSyncedChunkCount++;
						if (bIsResetChunk)
						{
							RestDesiredToLoadChunkLocations.pop();
						}
						else
						{
							DesiredToLoadChunkLocations.pop();
						}
						continue;
					}
					else // Multi-thread load
					{
						if (BatchedChunkLocations.size() >= VoxelSceneConfig.ChunkTaskPerCore)
						{
							//Dispatch MultiThreadGenerator
							Timer.Start();
							auto BoundFunction = boost::bind(&FChunkManage::MultiThreadGeneratorBatched, this, _1, _2, _3, _4);
							std::function<void()> TaskFunc = boost::bind(BoundFunction, BatchedChunkLocations, BatchedMipmapLevels, CameraInfo, VoxelSceneConfig);
							bool Success = GeneratorThreadPool.EnqueueForward(TaskFunc);
							DeltaMultiThreadTime += Timer.Step();
							if (Success)
							{
								BatchedChunkLocations.clear();
								BatchedMipmapLevels.clear();
							}
							else
							{
								goto FailedToDispatchBatch;
							}
						}
						CurrentMultiThreadChunkCount++;
						if (bIsResetChunk)
						{
							RestDesiredToLoadChunkLocations.pop();
						}
						else
						{
							DesiredToLoadChunkLocations.pop();
						}
						continue;
					}
				FailedToDispatchBatch:
					{
						if (bIsResetChunk)
						{
							RestDesiredToLoadChunkLocations.pop();
						}
						else
						{
							DesiredToLoadChunkLocations.pop();
						}
						//ChunkPool.ChunksLookupTable.ATOMIC_remove(CurrentDesiredChunkLocation);// Modify back
						for (auto CurrentLoadedDesiredChunkLocation : BatchedChunkLocations)
						{
							RestDesiredToLoadChunkLocations.push(CurrentLoadedDesiredChunkLocation);
							ChunkPool.ChunksLookupTable.ATOMIC_remove(CurrentLoadedDesiredChunkLocation);
						}
						break;
					}
				}
				else //Already loaded
				{
					if (bIsResetChunk)
					{
						RestDesiredToLoadChunkLocations.pop();
					}
					else
					{
						DesiredToLoadChunkLocations.pop();
					}
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
		//For Debug
		ChunkPool.UpdateDebugVisibleChunk(LVKContext, VoxelSceneConfig, RenderFrameIndex);
	}
	template<typename Struct>
	void DrawDebugVisibleChunk(lvk::IContext* LVKContext, const Struct& PushConstantData)
	{
		//Debug visible chunk
		if (bDebugVisibleChunk)
		{
			DebugTimerSet.Start(DebugMarkSumitRenderingDebugChunkTime);
			lvk::ICommandBuffer& Buffer = LVKContext->acquireCommandBuffer();
			Buffer.cmdBeginRendering(ChunkPool.RPDebugInstance, ChunkPool.FBDebugInstance);
			{
				// Scene, wireframe
				Buffer.cmdBindRenderPipeline(ChunkPool.RPLDebugInstance);
				Buffer.cmdPushDebugGroupLabel("Render Visibility Chunk Wireframe", 0xff0000ff);
				Buffer.cmdBindDepthState({ .compareOp = bDebugReverseZ ? lvk::CompareOp_Greater:lvk::CompareOp_Less, .isDepthWriteEnabled = true });
				Buffer.cmdBindVertexBuffer(0, ChunkPool.OctahedronMesh.VertexBuffer, 0);
				Buffer.cmdBindVertexBuffer(1, ChunkPool.DebugInstanceBuffer[RenderFrameIndex], 0);

				Buffer.cmdPushConstants(PushConstantData);
				Buffer.cmdBindIndexBuffer(ChunkPool.OctahedronMesh.IndexBuffer, lvk::IndexFormat_UI16);
				Buffer.cmdDrawIndexed(ChunkPool.OctahedronMesh.GetIndexSize(), ChunkPool.MaxChunkCount + ChunkPool.MaxEmptyChunkCount); // <-------- TODO: Culling scan in cs, draw indirect
				Buffer.cmdPopDebugGroupLabel();
			}
			Buffer.cmdEndRendering();
			//Buffer.transitionToShaderReadOnly(ChunkPool.FBDebugInstance.color[0].texture); //Transit
			LVKContext->submit(Buffer);
			DebugTimerSet.Record(DebugMarkSumitRenderingDebugChunkTime);
		}
	}

	//GUI
	void RenderManagerInfo()
	{
		ImVec2 NewWindowsPosition = ImVec2(50, 300);
		const float Offset = 300.0f;
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

		ImGui::Text("Loaded Block:");
		ImGui::SameLine(Offset);
		ImGui::Text("%d", ChunkPool.CurrentBlockCount);

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

		double UploadChunkTime = ChunkPool.DebugTimerSet.GetAverage(ChunkPool.DebugMarkUploadChunk);
		ImGui::Text("Debug Upload Chunk Time:");
		ImGui::SameLine(Offset);
		ImGui::Text("%.4lf ms", UploadChunkTime * 1000.0);

		double UploadBlockTime = ChunkPool.DebugTimerSet.GetAverage(ChunkPool.DebugMarkUploadBlock);
		ImGui::Text("Debug Upload Block Time:");
		ImGui::SameLine(Offset);
		ImGui::Text("%.4lf ms", UploadBlockTime * 1000.0);

		double GatherAllChunkTime = DebugTimerSet.GetAverage(DebugMarkGatherAllChunkTime);
		ImGui::Text("Gather All Chunk Time:");
		ImGui::SameLine(Offset);
		ImGui::Text("%.4lf ms", GatherAllChunkTime * 1000.0);

		double SumitRenderingDebugChunkTime = DebugTimerSet.GetAverage(DebugMarkSumitRenderingDebugChunkTime);
		ImGui::Text("Debug Sumit Rendering Debug Chunk Time:");
		ImGui::SameLine(Offset);
		ImGui::Text("%.4lf ms", SumitRenderingDebugChunkTime * 1000.0);

		ImGui::End();
		ImGui::PopStyleColor();
	}
};