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
#include "Voxel/Occupancy/BinaryOccupancyVolume.h"
#include "Helper/Comparator.h"

#include <LVK.h>

#include <functional>
#include <climits>
using glm::ivec3;
using glm::vec3;
using glm::ivec4;
using glm::u8vec3;
using glm::u8vec4;

struct FGPUChunk
{
	ivec3 ChunkLocation = { INT_MAX, INT_MAX, INT_MAX };
	uint32_t ChunkFrameStamp = 0;

	static std::string GetStructureShader()
	{
		return
			R"(
struct GPUChunk 
{
	ivec3 ChunkLocation;
	uint ChunkFrameStamp;
};
)";
	}
};
struct FChunkBase 
{
public:
	ivec3 ChunkLocation = { INT_MAX, INT_MAX, INT_MAX };
	uint32_t ChunkFrameStamp = 0;
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
	//std::set<ivec3, FIVec3Comparator> OccupancyVolume;
	std::vector<FBinaryOccupancyVolume> OccupancyVolumeErodeMipmaps;
	
	void AddBlock(const FBlock& NewBlock)
	{
		//OccupancyVolume.insert(NewBlock.BlockLocation);
		Blocks.push_back(NewBlock);
	}
	void AddBlock(FBlock&& NewBlock)
	{
		//OccupancyVolume.insert(NewBlock.BlockLocation);
		Blocks.push_back(std::move(NewBlock));
	}
	void CalculateOccupancyErodeMipmaps(const uint32_t Resolution = 16, const uint32_t MaxDepth = 4)
	{
		OccupancyVolumeErodeMipmaps.clear();
		FBinaryOccupancyVolume Mip0(Resolution);
		for (uint32_t i = 0; i < Blocks.size(); i++)
		{
			Mip0.Set(true, ivec3(Blocks[i].BlockLocation));
		}
		OccupancyVolumeErodeMipmaps.push_back(std::move(Mip0));
		for (uint32_t d = 1; d < MaxDepth; d++)
		{
			const FBinaryOccupancyVolume& LastMipmap = OccupancyVolumeErodeMipmaps.back();
			FBinaryOccupancyVolume CurrentMip(Resolution);
			for (uint32_t i = 0; i < Blocks.size(); i++)
			{
				const ivec3& CurrentLocation = ivec3(Blocks[i].BlockLocation);
				FBinaryOccupancyVolume::FOccupancyValue Solid = FOccupancyHelper::ErodeSingleVoxel(LastMipmap, CurrentLocation);
				CurrentMip.Set(Solid, CurrentLocation);
			}
			OccupancyVolumeErodeMipmaps.push_back(std::move(CurrentMip));
		}
	}

	bool bShouldVoxelOccupancyCull(ivec3 BlockLocation, uint32_t ThresholdDepth = 2) const
	{
		const FBinaryOccupancyVolume& CurrentOccupancyMipmap = OccupancyVolumeErodeMipmaps[ThresholdDepth];
		return CurrentOccupancyMipmap.GetWithBoundaryCondition(BlockLocation, true);
	}
};

struct FEmptyChunk : public FChunkBase
{
	// Empty Chunk
};