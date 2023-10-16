#pragma once
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <tuple>
#include <GLFW/glfw3.h>

#include <lvk/LVK.h>
#include <lvk/HelpersImGui.h>
#include "Shape.h"
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

struct FCube : public FShapeBase
{
public:
    std::tuple<std::vector<FGPUSimpleVertexData>, std::vector<uint16_t>, uint32_t> GetGPUMeshes(const float Scale = 1.0f) override
    {
        const std::vector<FGPUSimpleVertexData> VertexData =
        {
            // top
            {{-Scale, -Scale, +Scale}, {0.0, 0.0, 1.0}}, // 0
            {{+Scale, -Scale, +Scale}, {0.0, 0.0, 1.0}}, // 1
            {{+Scale, +Scale, +Scale}, {0.0, 0.0, 1.0}}, // 2
            {{-Scale, +Scale, +Scale}, {0.0, 0.0, 1.0}}, // 3
            // bottom
            {{-Scale, -Scale, -Scale}, {0.0, 0.0, -1.0}}, // 4
            {{-Scale, +Scale, -Scale}, {0.0, 0.0, -1.0}}, // 5
            {{+Scale, +Scale, -Scale}, {0.0, 0.0, -1.0}}, // 6
            {{+Scale, -Scale, -Scale}, {0.0, 0.0, -1.0}}, // 7
            // left
            {{+Scale, +Scale, -Scale}, {0.0, 1.0, 0.0}}, // 8
            {{-Scale, +Scale, -Scale}, {0.0, 1.0, 0.0}}, // 9
            {{-Scale, +Scale, +Scale}, {0.0, 1.0, 0.0}}, // 10
            {{+Scale, +Scale, +Scale}, {0.0, 1.0, 0.0}}, // 11
            // right
            {{-Scale, -Scale, -Scale}, {0.0, -1.0, 0.0}}, // 12
            {{+Scale, -Scale, -Scale}, {0.0, -1.0, 0.0}}, // 13
            {{+Scale, -Scale, +Scale}, {0.0, -1.0, 0.0}}, // 14
            {{-Scale, -Scale, +Scale}, {0.0, -1.0, 0.0}}, // 15
            // front
            {{+Scale, -Scale, -Scale}, {1.0, 0.0, 0.0}}, // 16
            {{+Scale, +Scale, -Scale}, {1.0, 0.0, 0.0}}, // 17
            {{+Scale, +Scale, +Scale}, {1.0, 0.0, 0.0}}, // 18
            {{+Scale, -Scale, +Scale}, {1.0, 0.0, 0.0}}, // 19
            // back
            {{-Scale, +Scale, -Scale}, {-1.0, 0.0, 0.0}}, // 20
            {{-Scale, -Scale, -Scale}, {-1.0, 0.0, 0.0}}, // 21
            {{-Scale, -Scale, +Scale}, {-1.0, 0.0, 0.0}}, // 22
            {{-Scale, +Scale, +Scale}, {-1.0, 0.0, 0.0}}, // 23
        };

        const std::vector<uint16_t> IndexData =
        {
            0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,
            8,  9,  10, 10, 11, 8,  12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20
        };
        return { VertexData , IndexData ,  uint32_t(IndexData.size()) * uint32_t(sizeof(uint16_t)) };
    }
};

using FCubeHolder = FShapeHolderBase<FCube>;