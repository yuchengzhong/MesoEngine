// Meso Engine 2024
#pragma once
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include "Helper/VoxelMathHelper.h"
using glm::ivec3;
using glm::ivec4;
using glm::u8vec3;
using glm::u8vec4;


enum class EChunkOverrideMode : uint8_t
{
	FindLess = 1 << 0, // fastest
	FindMin = 1 << 1, // mild
	OverrideMin = 1 << 2, //slowest, worst
};


struct FVoxelSceneConfig
{
	unsigned char BlockResolution = 8; //This won't change basically
	float BlockSize = 1.0f;
	unsigned char ChunkResolution = 16;
	uint32_t MaxBlockCount = 65536 * 16;
	uint32_t MaxVolumeCount = 65536 * 16;
	uint32_t MaxChunkCount = 8192 * 2;
	uint32_t MaxEmptyChunkCount = 8192 * 4;

	uint32_t MaxChunkCheckTimes = 128;
	uint32_t MaxEmptyChunkCheckTimes = 128;
	uint32_t MaxBlockCheckTimes = 4;

	uint32_t BakeVisibilityViewNum = 256;
	uint32_t ViewForwardLoadChunkSize = 24;
	uint32_t ViewBackwardLoadChunkSize = 6;
	uint32_t MaxSyncedLoadChunkCount = 0;
	uint32_t MaxUnsyncedLoadChunkCount = 256; //will not exceed physical cpu cores
	uint32_t ChunkTaskPerCore = 8; //Chunk batch
	EChunkOverrideMode ChunkOverrideMode = EChunkOverrideMode::FindMin;// 
	float ViewChunkAngle = 120.0f;//should = fov

	//Chunk config
	uint32_t ChunkOccupancyDepth = 4;
	uint32_t ChunkInnerVoxelCullDepthThreshold = 1;
	float GetChunkSize() const
	{
		return ChunkResolution * BlockSize;
	}
};