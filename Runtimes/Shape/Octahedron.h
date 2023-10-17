// Meso Engine 2024
#pragma once
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <tuple>
#include "Shape.h"
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

class FOctahedron : public FShapeBase
{
public:
    std::tuple<std::vector<FGPUSimpleVertexData>, std::vector<uint16_t>, uint32_t> GetGPUMeshes(const float Scale = 1.0f) override
    {
        const std::vector<FGPUSimpleVertexData> VertexData =
        {
            // 6 in total
            {{0.0f, 0.0f, +Scale}, {0.0f, 0.0f, 1.0f}}, // 0
            {{0.0f, +Scale, 0.0f}, {0.0f, 1.0f, 0.0f}}, // 1
            {{+Scale, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // 2
            {{0.0f, -Scale, 0.0f}, {0.0f, -1.0f, 0.0f}}, // 3
            {{-Scale, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}, // 4
            {{0.0f, 0.0f, -Scale}, {0.0f, 0.0f, -1.0f}}, // 5
        };

        const std::vector<uint16_t> IndexData =
        {
            // Top half
            0, 1, 2,  // Top front-right triangle
            0, 2, 3,  // Top back-right triangle
            0, 3, 4,  // Top back-left triangle
            0, 4, 1,  // Top front-left triangle

            // Bottom half
            5, 2, 1,  // Bottom front-right triangle
            5, 3, 2,  // Bottom back-right triangle
            5, 4, 3,  // Bottom back-left triangle
            5, 1, 4   // Bottom front-left triangle
        };
        return { VertexData , IndexData ,  uint32_t(IndexData.size()) * uint32_t(sizeof(uint16_t)) };
    }
};

using FOctahedronHolder = FShapeHolderBase<FOctahedron>;