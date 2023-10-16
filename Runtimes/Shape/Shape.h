#pragma once
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <cmath>
#include <tuple>

#include <GLFW/glfw3.h>

#include <lvk/LVK.h>
#include <lvk/HelpersImGui.h>

#include "Shader/GPUStructures.h"
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::ivec3;
class FShapeBase
{
public:
    //Triangle, Index, IndexSize
    inline virtual std::tuple<std::vector<FGPUSimpleVertexData>, std::vector<uint16_t>, uint32_t> GetGPUMeshes(const float Scale = 1.0f) = 0;
    inline virtual uint32_t GetIndexSize()
    {
        return uint32_t(std::get<1>(GetGPUMeshes()).size()) * sizeof(uint16_t);
    };
};


template<typename T>
class FShapeHolderBase
{
public:
    lvk::Holder<lvk::BufferHandle> VertexBuffer, IndexBuffer;
    lvk::VertexInput DescVertex;
    uint32_t ShapeIndexCount = 0;
    void Initialize(lvk::IContext* LVKContext)
    {
        static_assert(std::is_base_of_v<FShapeBase, T>, "T must be derived from FShapeBase");
        T DummyShape;
        auto Meshes = DummyShape.GetGPUMeshes(1.0);
        const auto& VertexData = std::get<0>(Meshes);
        const auto& IndexData = std::get<1>(Meshes);
        ShapeIndexCount = std::get<2>(Meshes);
        VertexBuffer = LVKContext->createBuffer(
            {
                .usage = lvk::BufferUsageBits_Vertex,
                .storage = lvk::StorageType_Device,
                .size = sizeof(FGPUSimpleVertexData) * VertexData.size(),
                .data = VertexData.data(),
                .debugName = "Buffer: vertex"
            },
            nullptr);
        IndexBuffer = LVKContext->createBuffer(
            {
                .usage = lvk::BufferUsageBits_Index,
                .storage = lvk::StorageType_Device,
                .size = sizeof(uint32_t) * IndexData.size(),
                .data = IndexData.data(),
                .debugName = "Buffer: index"
            },
            nullptr);

        DescVertex = FGPUSimpleVertexData::GetDescriptor();
    }
    uint32_t GetIndexSize() const
    {
        return ShapeIndexCount;
    }
    const lvk::VertexInput& GetVertexDescriptor() const
    {
        return DescVertex;
    }
};