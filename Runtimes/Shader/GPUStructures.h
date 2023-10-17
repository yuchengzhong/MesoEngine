// Meso Engine 2024
#pragma once
#include <glm/ext.hpp>
#include <glm/glm.hpp>

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::ivec3;

struct FGPUUniformSceneConfig
{
	float BlockSize = 0.0f;
	uint32_t BlockResolution = 8;
	float ChunkSize = 0.0f;
	uint32_t ChunkResolution = 0;

	static std::string GetStructureShader()
	{
		return
			R"(
layout(std430, buffer_reference) readonly buffer SceneInfo
{
	float BlockSize;
	uint BlockResolution;
	float ChunkSize;
	uint ChunkResolution;
};
)";
	}
};


struct FGPUUniformCamera
{
	mat4 Projection = {};
	mat4 View = {};
	ivec3 CameraChunkLocation = {};

	static std::string GetStructureShader()
	{
		return
			R"(
layout(std430, buffer_reference) readonly buffer CameraInfo
{
	mat4 Projection;
	mat4 View;
	ivec3 CameraChunkLocation;
};
)";
	}
};



struct FGPUSimpleVertexData
{
    vec3 Position = {};
    vec3 Normal = { 0.0f, 0.0f, 1.0f }; // Int_2_10_10_10_REV
    static lvk::VertexInput GetDescriptor()
    {
        return
        {
            .attributes =
            {
                {.location = 0, .format = lvk::VertexFormat::Float3, .offset = offsetof(FGPUSimpleVertexData, Position)},
                {.location = 1, .format = lvk::VertexFormat::Float3, .offset = offsetof(FGPUSimpleVertexData, Normal)},
            },
            .inputBindings = { {.stride = sizeof(FGPUSimpleVertexData)} },
        };
    }
    static std::string GetLayoutShader(uint32_t Offset = 0)//Layout size: 2
    {
        const std::string Layout = "layout (location=";
        return "\n" +
            Layout + std::to_string(Offset + 0) + ") in vec3 VertexPosition;\n" +
            Layout + std::to_string(Offset + 1) + ") in vec3 VertexNormal;\n" +
            "\n";
    }
};

struct FGPUSimpleInstanceData
{
    vec3 Position = {};
    ivec3 ChunkLocation = { 0,0,0 };
    float Scale = 1.0f;
    vec4 Rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
    float Marker = 0.0f;

    static lvk::VertexInput GetInstanceDescriptor()
    {
        return
        {
            .attributes =
            {
                {.location = 0, .binding = 0, .format = lvk::VertexFormat::Float3, .offset = offsetof(FGPUSimpleVertexData, Position)},
                {.location = 1, .binding = 0, .format = lvk::VertexFormat::Float3, .offset = offsetof(FGPUSimpleVertexData, Normal)},
                {.location = 2, .binding = 1, .format = lvk::VertexFormat::Float3, .offset = offsetof(FGPUSimpleInstanceData, Position)},
                {.location = 3, .binding = 1, .format = lvk::VertexFormat::Int3, .offset = offsetof(FGPUSimpleInstanceData, ChunkLocation)},
                {.location = 4, .binding = 1, .format = lvk::VertexFormat::Float1, .offset = offsetof(FGPUSimpleInstanceData, Scale)},
                {.location = 5, .binding = 1, .format = lvk::VertexFormat::Float4, .offset = offsetof(FGPUSimpleInstanceData, Rotation)},
                {.location = 6, .binding = 1, .format = lvk::VertexFormat::Float1, .offset = offsetof(FGPUSimpleInstanceData, Marker)},
            },
            .inputBindings = { {.stride = sizeof(FGPUSimpleVertexData), .rate = 0}, {.stride = sizeof(FGPUSimpleInstanceData), .rate = 1} },
        };
    }
    static std::string GetLayoutShader(uint32_t Offset = 0)//Layout size: 5
    {
        const std::string Layout = "layout (location=";
        return "\n" +
            Layout + std::to_string(Offset + 0) + ") in vec3 InstancePosition;\n" +
            Layout + std::to_string(Offset + 1) + ") in ivec3 InstanceChunkLocation;\n" +
            Layout + std::to_string(Offset + 2) + ") in float InstanceScale;\n" +
            Layout + std::to_string(Offset + 3) + ") in vec4 InstanceRotation;\n" +
            Layout + std::to_string(Offset + 4) + ") in float InstanceMarker;\n" +
            "\n";
    }
    static std::string GetInstanceLayoutShader(uint32_t Offset = 0)//Layout size: 5
    {
        return FGPUSimpleVertexData::GetLayoutShader(Offset) + GetLayoutShader(2 + Offset);
    }
};

struct FGPUChunkData
{
    ivec3 ChunkLocation = { INT_MAX, INT_MAX, INT_MAX };
};