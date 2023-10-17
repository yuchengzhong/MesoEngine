// Meso Engine 2024
#pragma once
#include <vector>
#include <set>
#include <map>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include "Voxel/VoxelSceneConfig.h"
#include "Voxel/Chunk/Chunk.h"

struct FBlock
{
	uint32_t ChunkIndex; //This refer to GPU chunk ssbo
	u8vec3 BlockLocation; //Generator
	uint32_t VolumeIndex; //This refer to GPU Volume virtual index
};
