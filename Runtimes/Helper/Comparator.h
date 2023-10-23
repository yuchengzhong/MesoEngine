// Meso Engine 2024
#pragma once
#include "Voxel/Block/Block.h"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

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

struct FBlockComparator
{
	bool operator()(const FBlock& A, const FBlock& B) const
	{
		if (A.ChunkIndex != B.ChunkIndex) return A.ChunkIndex < B.ChunkIndex;
		if (A.BlockLocation.x != B.BlockLocation.x) return A.BlockLocation.x < B.BlockLocation.x;
		if (A.BlockLocation.y != B.BlockLocation.y) return A.BlockLocation.y < B.BlockLocation.y;
		if (A.BlockLocation.z != B.BlockLocation.z) return A.BlockLocation.z < B.BlockLocation.z;
		return A.VolumeIndex < B.VolumeIndex;
	}
};