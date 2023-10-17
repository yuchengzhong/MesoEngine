// Meso Engine 2024
#pragma once
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include "Helper/VoxelMathHelper.h"
using glm::ivec3;
using glm::ivec4;
using glm::u8vec3;
using glm::u8vec4;

struct FVoxelSceneConfig
{
	unsigned char BlockResolution = 8; //This won't change basically
	float BlockSize = 1.0f;
	unsigned char ChunkResolution = 16;
	uint32_t MaxBlockCount = 65536;
	uint32_t MaxVolumeCount = 65536;
	uint32_t MaxChunkCount = 4096;
	uint32_t MaxEmptyChunkCount = 8192 * 4;

	uint32_t BakeVisibilityViewNum = 64;
	uint32_t ViewForwardLoadChunkSize = 16;
	uint32_t ViewBackwardLoadChunkSize = 4;
	uint32_t MaxSyncedLoadChunkCount = 0;
	uint32_t MaxUnsyncedLoadChunkCount = 64;
	float ViewChunkAngle = 120.0f;//should = fov

	float GetChunkSize() const
	{
		return ChunkResolution * BlockSize;
	}
};