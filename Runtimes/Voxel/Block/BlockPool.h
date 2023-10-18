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
#include "Voxel/Chunk/Chunk.h"
#include "Shader/GPUStructures.h"
#include "Helper/TimerSet.h"
#include "Helper/VoxelMathHelper.h"
#include "Shader/ShaderWireFrame.h"
#include "Thread/ThreadSafeMap.h"
#include "Thread/AtomicVector.h"
#include "Thread/ThreadSafeMemoryPool.h"

#include <functional>
#include <climits>

//Now is single thread, TODO: make this multi thread
class BlockPool
{
public:
	std::vector<FGPUChunkData> GPUChunkData;
};