// Meso Engine 2024
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

/*
         3-------2
        / \      /|
       /   \    / |
      /     \  /  |
     /       \/   |
    4-------+0----1
    |      /|    /
    |    /  |   /
    |  /    |  /
    |/      | /
    5-------6
*/
struct FTriplePlanarCube : public FShapeBase
{
public:
    std::tuple<std::vector<FGPUSimpleVertexData>, std::vector<uint16_t>, uint32_t> GetGPUMeshes(const float Scale = 1.0f) override
    {
        const std::vector<uint16_t> IndexData =
        {
            0,  
            1, 2,
            3, 4,
            5, 6,
            1,
        };
        return { {} , IndexData ,  uint32_t(IndexData.size()) };
    }
};

using FTriplePlanarCubeHolder = FShapeHolderBase<FTriplePlanarCube, true>;