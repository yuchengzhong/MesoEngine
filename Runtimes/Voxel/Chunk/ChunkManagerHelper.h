// Meso Engine 2024
#pragma once
#include <queue>
#include <set>
#include <map>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "Voxel/VoxelSceneConfig.h"
#include "Voxel/Spatial/NearestMap.h"
#include "Helper/VoxelMathHelper.h"
using glm::ivec3;
using glm::vec3;
using glm::ivec4;
using glm::u8vec3;
using glm::u8vec4;
struct FImportanceComputeInfo
{
	ivec3 CameraChunk = { 0,0,0 };
	vec3 CameraForwardVector = {};

	inline static float CalculateChunkImportance(const FImportanceComputeInfo& CameraInfo, ivec3 ChunkLocation)
	{
		float Importance = 0.0f;
		ivec3 CurrentOffset = ChunkLocation - CameraInfo.CameraChunk;
		const float Far = 64.0f;
		if (CurrentOffset.x >= -2 && CurrentOffset.x <= 2 && CurrentOffset.y >= -2 && CurrentOffset.y <= 2 && CurrentOffset.z >= -2 && CurrentOffset.z <= 2)
		{
			Importance = 1.0e6f;
		}
		else
		{
			vec3 Direction = normalize(vec3(CurrentOffset));
			float Distance = length(vec3(CurrentOffset));
			float AngleImportance = std::max((std::max(0.0f, dot(Direction, CameraInfo.CameraForwardVector)) - 0.5f) * 2.0f, 0.75f);
			float DistanceImportance = std::max(0.25f, Far - Distance);
			Importance = AngleImportance * DistanceImportance;
		}
		return Importance;
	}
	inline float CalculateChunkImportance(ivec3 ChunkLocation) const
	{
		return CalculateChunkImportance(*this, ChunkLocation);
	}

	inline static float CalculateBlockImportance(const FImportanceComputeInfo& CameraInfo, ivec3 ChunkLocation, u8vec3 BlockLocation, uint32_t ChunkResolution = 16)
	{
		float Importance = 0.0f;
		ivec3 CurrentOffset = (ChunkLocation - CameraInfo.CameraChunk) * (int)ChunkResolution + ivec3(BlockLocation);
		const float Far = 64.0f * ChunkResolution;
		if (CurrentOffset.x >= -2 * ChunkResolution && CurrentOffset.x <= 2 * ChunkResolution && 
			CurrentOffset.y >= -2 * ChunkResolution && CurrentOffset.y <= 2 * ChunkResolution && 
			CurrentOffset.z >= -2 * ChunkResolution && CurrentOffset.z <= 2 * ChunkResolution)
		{
			Importance = 1.0e6f;
		}
		else
		{
			vec3 Direction = normalize(vec3(CurrentOffset));
			float Distance = length(vec3(CurrentOffset));
			float AngleImportance = std::max((std::max(0.0f, dot(Direction, CameraInfo.CameraForwardVector)) - 0.5f) * 2.0f, 0.75f);
			float DistanceImportance = std::max(0.25f, Far - Distance);
			Importance = AngleImportance * DistanceImportance;
		}
		return Importance;
	}
	inline float CalculateBlockImportance(ivec3 ChunkLocation, u8vec3 BlockLocation, uint32_t ChunkResolution = 16) const
	{
		return CalculateBlockImportance(*this, ChunkLocation, BlockLocation, ChunkResolution);
	}
};
struct FChunkManageHelper
{
	using FTempChunkDataType = std::pair<float, ivec3>; // <Importance, ChunkLocation>
	struct Comparator 
	{
		bool operator()(const FTempChunkDataType& A, const FTempChunkDataType& B) const 
		{
			return A.first < B.first;
		}
	};

	using FImportanceChunkQueue = std::priority_queue<FTempChunkDataType, std::vector<FTempChunkDataType>, Comparator>;

	inline static FImportanceChunkQueue GetDesiredShowChunkLocationByView(vec3 ForwardVector, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		//auto Compare = [](const FTempChunkDataType& A, const FTempChunkDataType& B) { return A.first < B.first; };
		//FImportanceChunkQueue ImportancePriorityQueue(FChunkManageHelper::Compare);
		FImportanceChunkQueue ImportancePriorityQueue;

		int32_t ForwardLoadChunkSize = (int32_t)VoxelSceneConfig.ViewForwardLoadChunkSize;
		int32_t BackwardLoadChunkSize = (int32_t)VoxelSceneConfig.ViewBackwardLoadChunkSize;
		float ViewThreshold = std::max(cos(glm::radians(VoxelSceneConfig.ViewChunkAngle) * 0.5f), 0.01f); //angle0 -> 1, angle 180 -> 0
		for (int32_t X = -ForwardLoadChunkSize; X <= ForwardLoadChunkSize; X++)
		{
			for (int32_t Y = -ForwardLoadChunkSize; Y <= ForwardLoadChunkSize; Y++)
			{
				for (int32_t Z = -ForwardLoadChunkSize; Z <= ForwardLoadChunkSize; Z++)
				{
					ivec3 CurrentOffset = { X, Y, Z };
					if (length(vec3(CurrentOffset)) > ForwardLoadChunkSize + 1e-6)
					{
						continue;
					}
					bool bAcceptable = false;
					float Importance = 0.0f;
					//Constantly load nearest
					if (X >= -1 && X <= 1 && Y >= -1 && Y <= 1 && Z >= -1 && Z <= 1)
					{
						bAcceptable = true;
					}
					else
					{
						vec3 CurrentOffsetFloat = { X, Y, Z };
						vec3 CurrentOffsetDirection = normalize(CurrentOffsetFloat);
						vec3 CurrentForwardVector = normalize(ForwardVector);
						float DistanceAlphaThreshold = std::max(dot(CurrentForwardVector, CurrentOffsetDirection), 0.0f);
						bool bInViewCone = DistanceAlphaThreshold > ViewThreshold;
						DistanceAlphaThreshold = bInViewCone ? 1.0f : DistanceAlphaThreshold / ViewThreshold;
						DistanceAlphaThreshold = std::min(std::max(DistanceAlphaThreshold, 0.0f), 1.0f);
						float DistanceThreshold = DistanceAlphaThreshold * ForwardLoadChunkSize + (1.0f - DistanceAlphaThreshold) * BackwardLoadChunkSize;
						float ChunkDistance = length(CurrentOffsetFloat);
						if (ChunkDistance < DistanceThreshold)
						{
							//Acceptable Chunk!
							bAcceptable = true;
						}
					}
					if (bAcceptable)
					{
						FImportanceComputeInfo CameraInfo =
									{
										.CameraChunk = {0,0,0},
										.CameraForwardVector = ForwardVector
									};
						ImportancePriorityQueue.push(
							{ 
								CameraInfo.CalculateChunkImportance(CurrentOffset), 
								CurrentOffset 
							});
					}
				}
			}
		}
		return ImportancePriorityQueue;
	}
	inline static FImportanceChunkQueue GetDesiredShowChunkLocationSimple(vec3 ForwardVector, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		//auto Compare = [](const FTempChunkDataType& A, const FTempChunkDataType& B) { return A.first < B.first; };
		//FImportanceChunkQueue ImportancePriorityQueue(FChunkManageHelper::Compare);
		FImportanceChunkQueue ImportancePriorityQueue;

		int32_t ForwardLoadChunkSize = (int32_t)VoxelSceneConfig.ViewForwardLoadChunkSize;
		int32_t BackwardLoadChunkSize = (int32_t)VoxelSceneConfig.ViewBackwardLoadChunkSize;
		float ViewThreshold = std::max(cos(glm::radians(VoxelSceneConfig.ViewChunkAngle) * 0.5f), 0.01f); //angle0 -> 1, angle 180 -> 0
		for (int32_t X = -ForwardLoadChunkSize; X <= ForwardLoadChunkSize; X++)
		{
			for (int32_t Y = -ForwardLoadChunkSize; Y <= ForwardLoadChunkSize; Y++)
			{
				for (int32_t Z = -ForwardLoadChunkSize; Z <= ForwardLoadChunkSize; Z++)
				{
					ivec3 CurrentOffset = { X, Y, Z };
					if (length(vec3(CurrentOffset)) > ForwardLoadChunkSize + 1e-6)
					{
						continue;
					}
					bool bAcceptable = false;
					float Importance = 0.0f;
					//Constantly load nearest
					if (X >= -1 && X <= 1 && Y >= -1 && Y <= 1 && Z >= -1 && Z <= 1)
					{
						bAcceptable = true;
						Importance = 1.0e6f;
					}
					else
					{
						vec3 CurrentOffsetFloat = { X, Y, Z };
						float DistanceThreshold = ForwardLoadChunkSize;
						float ChunkDistance = length(CurrentOffsetFloat);
						if (ChunkDistance < DistanceThreshold)
						{
							//Acceptable Chunk!
							bAcceptable = true;
							Importance = 1.0f / ChunkDistance;
						}
					}
					if (bAcceptable)
					{
						ImportancePriorityQueue.push({ Importance, CurrentOffset });
					}
				}
			}
		}
		return ImportancePriorityQueue;
	}
	//Bake
	inline static TNearestMap<FImportanceChunkQueue> BakeVisibilityByView(const FVoxelSceneConfig& VoxelSceneConfig, uint32_t Samples = 64u, bool bUseMulithreading = false)
	{
		FTimer Timer;
		TNearestMap<FImportanceChunkQueue> Result;
		std::vector<vec3> Directions = FVoxelMathHelper::GetFibonacciSphere<float>(Samples);

		if (!bUseMulithreading)
		{
			for (const auto& CurrentDirection : Directions)
			{
				FImportanceChunkQueue CurrentVisibility = GetDesiredShowChunkLocationByView(CurrentDirection, VoxelSceneConfig);
				Result.Insert(CurrentDirection, std::move(CurrentVisibility));
			}
		}
		else
		{
			boost::thread_group ThreadGroup;
			boost::mutex Lock;

			for (const auto& CurrentDirection : Directions)
			{
				ThreadGroup.create_thread([&Result, &CurrentDirection, &VoxelSceneConfig, &Lock]()
					{
						FImportanceChunkQueue CurrentVisibility = GetDesiredShowChunkLocationByView(CurrentDirection, VoxelSceneConfig);

						boost::lock_guard<boost::mutex> Lock_(Lock);
						Result.Insert(CurrentDirection, std::move(CurrentVisibility));
					});
			}
			ThreadGroup.join_all();
		}
		printf("Baked Time: %.2lfs\n", Timer.Step());
		return Result;
	}

	inline static uint32_t TruncateFrameStamp(uint64_t FrameStamp)
	{
		//return FrameStamp % (0xFFFFFFFFllu);
		return FrameStamp % (0x100000000llu);
	}
};