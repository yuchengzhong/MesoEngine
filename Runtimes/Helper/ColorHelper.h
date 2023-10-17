// Meso Engine 2024
#pragma once
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <tuple>

#include <imgui.h>
using glm::vec3;
using glm::vec2;
using glm::ivec3;
using glm::ivec4;
using glm::u8vec3;
using glm::u8vec4;

struct FColorHelper
{
	inline static ImVec4 GetBackgroundGreen(float Brightness = 0.25f, float Alpha = 0.75f)
	{
		return ImVec4(0.75f * Brightness, 1.0f * Brightness, 0.75f * Brightness, Alpha);
	}
	inline static ImVec4 GetBackgroundBlue(float Brightness = 0.25f, float Alpha = 0.75f)
	{
		return ImVec4(0.75f * Brightness, 0.75f * Brightness, 1.0f * Brightness, Alpha);
	}
	inline static ImVec4 GetBackgroundRed(float Brightness = 0.25f, float Alpha = 0.75f)
	{
		return ImVec4(1.0f * Brightness, 0.75f * Brightness, 0.75f * Brightness, Alpha);
	}
};