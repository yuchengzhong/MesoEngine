// Meso Engine 2024
#pragma once
#include "Shape/Cube.h"

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include <lvk/LVK.h>
#include <lvk/HelpersImGui.h>

#include "VoxelSceneConfig.h"
#include "Helper/VoxelMathHelper.h"
//Chunk -> Block   ->   Voxel
//		         Volume
using glm::ivec3;
using glm::ivec4;
using glm::u8vec3;
using glm::u8vec4;

//Remember:
//ChunkLocation start from the corner
struct FGPUBlockInstanceData
{
	ivec3 ChunkLocation; //ChunkLocation and ChunkIndex
	uint32_t ChunkIndex;
	u8vec3 BlockLocation; //BlockLocation and ???
};

class FVolume
{
	std::vector<uint32_t> Occupancy;
	void Initialize(const FVoxelSceneConfig& VoxelSceneConfig)
	{
		uint32_t VoxelCount = ((uint32_t)VoxelSceneConfig.BlockResolution) * VoxelSceneConfig.BlockResolution * VoxelSceneConfig.BlockResolution;
		uint32_t MaskCount = VoxelCount / 32;
		Occupancy.resize(MaskCount);
	}
};

class FVolumePoolHolder
{
public:
	uint32_t MaxVolumeCount = 0;
	std::vector<FVolume> VolumeData;
	void Initialize(lvk::IContext* LVKContext, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		VolumeData.resize(VoxelSceneConfig.MaxVolumeCount);
	}
};

//Sketch
//Chunk Pool
//Block Pool
class FBlockPoolHolder
{
public:
	uint32_t MaxBlockCount = 0;
	std::vector<FGPUBlockInstanceData> InstanceData;
	lvk::Holder<lvk::BufferHandle> VoxelInstanceBuffer;
	//State
	bool bDirty = false;
	void Initialize(lvk::IContext* LVKContext, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		MaxBlockCount = VoxelSceneConfig.MaxBlockCount;
		//InstanceData.resize(MaxBlockCount);

		VoxelInstanceBuffer = LVKContext->createBuffer(
			{
				.usage = lvk::BufferUsageBits_Vertex,
				.storage = lvk::StorageType_Device,
				.size = sizeof(FGPUBlockInstanceData) * MaxBlockCount,
				.data = nullptr, //InstanceData.data(), not **upload** yet
				.debugName = "Buffer: Block Instance"
			},
			nullptr);//To update, use **upload**
	}
	/*
	void Update(const std::vector<FGPUBlockInstanceData>& NewInstanceData, const FVoxelCamera& Camera, const FVoxelSceneConfig& VoxelSceneConfig)
	{
		InstanceData = NewInstanceData;
		uint32_t BlockResolution = (uint32_t)VoxelSceneConfig.BlockResolution;
		std::sort(InstanceData.begin(), InstanceData.end(), [&Camera, BlockResolution](const FGPUBlockInstanceData& A, const FGPUBlockInstanceData& B)
		{
			ivec3 RelativeChunkLocationA = A.ChunkLocation - Camera.CameraChunkLocation;
			ivec3 RelativeChunkLocationB = B.ChunkLocation - Camera.CameraChunkLocation;
			ivec3 RelativeLocationA = { 
										BlockResolution * RelativeChunkLocationA.x + A.BlockLocation.x, 
										BlockResolution * RelativeChunkLocationA.y + A.BlockLocation.y, 
										BlockResolution * RelativeChunkLocationA.z + A.BlockLocation.z};
			ivec3 RelativeLocationB = { 
										BlockResolution * RelativeChunkLocationB.x + B.BlockLocation.x, 
										BlockResolution * RelativeChunkLocationB.y + B.BlockLocation.y, 
										BlockResolution * RelativeChunkLocationB.z + B.BlockLocation.z};
			return a.age < b.age;  // sort in ascending order of age
		});
	}*/
};