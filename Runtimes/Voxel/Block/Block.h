// Meso Engine 2024
#pragma once
#include <vector>
#include <set>
#include <map>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include "Shape/Shape.h"
#include "Voxel/VoxelSceneConfig.h"
#include <LVK.h>

struct FBlock
{
	uint32_t ChunkIndex = INT_MAX; //This refer to GPU chunk ssbo
    u8vec3 BlockLocation = {255u,255u,255u}; //Generator
	uint32_t VolumeIndex = INT_MAX; //This refer to GPU Volume virtual index
};

struct FGPUBlock
{
	uint32_t ChunkIndex = INT_MAX; //This refer to GPU chunk ssbo
    u8vec4 BlockLocation = {255u,255u,255u,255u};
	uint32_t BlockFrameStamp = 0;
	//uint32_t VolumeIndex; //This refer to GPU Volume virtual index

    static lvk::VertexInput GetInstanceDescriptor()
    {
        return
        {
            .attributes =
            {
                {.location = 0, .binding = 0, .format = lvk::VertexFormat::Float3, .offset = offsetof(FGPUSimpleVertexData, Position)},
                {.location = 1, .binding = 0, .format = lvk::VertexFormat::Float3, .offset = offsetof(FGPUSimpleVertexData, Normal)},
                {.location = 2, .binding = 1, .format = lvk::VertexFormat::UInt1, .offset = offsetof(FGPUBlock, ChunkIndex)},
                {.location = 3, .binding = 1, .format = lvk::VertexFormat::UInt1, .offset = offsetof(FGPUBlock, BlockLocation)},
                {.location = 4, .binding = 1, .format = lvk::VertexFormat::UInt1, .offset = offsetof(FGPUBlock, BlockFrameStamp)},
            },
            .inputBindings = { {.stride = sizeof(FGPUSimpleVertexData), .rate = 0}, {.stride = sizeof(FGPUBlock), .rate = 1} },
        };
    }
    static std::string GetLayoutShader(uint32_t Offset = 0)//Layout size: 3
    {
        const std::string Layout = "layout (location=";
        return "\n" +
            Layout + std::to_string(Offset + 0) + ") in uint InstanceChunkIndex;\n" +
            Layout + std::to_string(Offset + 1) + ") in uint InstanceBlockLocation;\n" +
            Layout + std::to_string(Offset + 2) + ") in uint InstanceBlockFrameStamp;\n" +
            "\n";
    }
};
