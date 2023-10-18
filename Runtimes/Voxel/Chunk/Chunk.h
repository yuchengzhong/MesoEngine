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

#include <functional>
#include <climits>
using glm::ivec3;
using glm::vec3;
using glm::ivec4;
using glm::u8vec3;
using glm::u8vec4;

struct FBlock;


struct FTraceInfomation
{
	uint64_t ChunkFrameStamp;
	uint32_t Priority;
};

struct FGPUChunk
{
	ivec3 ChunkLocation = { INT_MAX, INT_MAX, INT_MAX };
};
struct FChunkBase 
{
public:
	ivec3 ChunkLocation = { INT_MAX, INT_MAX, INT_MAX };
public:
	//FChunkBase() : ChunkLocation({ INT_MAX, INT_MAX, INT_MAX }) {}
	bool bIsValid() const
	{
		return ChunkLocation != ivec3{ INT_MAX, INT_MAX, INT_MAX };
	}
};

struct FChunk : public FChunkBase
{
	std::vector<FBlock> Blocks;
};

struct FEmptyChunk : public FChunkBase
{
	// Empty Chunk
};