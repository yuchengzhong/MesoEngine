#pragma once
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <tuple>
using glm::vec3;
using glm::vec2;
using glm::ivec3;
using glm::ivec4;
using glm::u8vec3;
using glm::u8vec4;

#define _USE_MATH_DEFINES
struct FVoxelMathHelper
{
	template<typename T>
	inline static std::tuple<glm::tvec3<T, glm::defaultp>, ivec3> ConvertToChunkLocation(glm::tvec3<T, glm::defaultp> Position, float ChunkSize)
	{
		glm::tvec3<T, glm::defaultp> ChunkLocation = floor(Position / ChunkSize);//Todo: everytime positioner update, update this and the position of the poristioner, make it loop(like) in a small room.
		glm::tvec3<T, glm::defaultp> FractedPosition = Position - glm::tvec3<T, glm::defaultp>(ChunkLocation) * ChunkSize;
		return { FractedPosition , ivec3(ChunkLocation) };
	}

	template<typename T>
	inline static T Fract(T Value) 
	{
		return Value - std::floor(Value);
	}
	template<typename T>
	inline static T Hash(glm::tvec3<T, glm::defaultp> Position) 
	{
		return Fract(sin(dot(Position, glm::tvec3<T, glm::defaultp>(127.1, 311.7, 74.7))) * static_cast<T>(43758.5453123));
	}

	template<typename T>
	inline static T Hash(glm::tvec2<T, glm::defaultp> Position)
	{
		return Fract(sin(dot(Position, glm::tvec2<T, glm::defaultp>(127.1, 311.7))) * static_cast<T>(43758.5453123));
	}

	template<typename T>
	inline static T Hash(T Position)
	{
		return fract(sin(Position) * static_cast<T>(43758.5453123));
	}


	template<typename T>
	inline static std::vector<glm::tvec3<T, glm::defaultp>> GetFibonacciSphere(uint32_t Samples = 100u, bool Normalize = true)
	{
		std::vector<glm::tvec3<T, glm::defaultp>> Points;
		const T Phi = M_PI * (std::sqrt(5.0) - 1.0);

		for (int i = 0; i < Samples; ++i)
		{
			T Y = 1 - (i / static_cast<T>(Samples - 1)) * 2;
			T Radius = std::sqrt(1 - Y * Y);
			T Theta = Phi * i;
			T X = std::cos(Theta) * Radius;
			T Z = std::sin(Theta) * Radius;
			if (Normalize)
			{
				Points.push_back(normalize(glm::tvec3<T, glm::defaultp>(X, Y, Z)));
			}
			else
			{
				Points.push_back(glm::tvec3<T, glm::defaultp>(X, Y, Z));
			}
		}
		return Points;
	}

	/*
	inline static vec3 GetRelativeBlockLocation(const FGPUBlockInstanceData& Block, const FVoxelCamera& Camera)
	{
		ivec3 RelativeChunkLocationA = Block.ChunkLocation - Camera.CameraChunkLocation;
		ivec3 RelativeLocationA = {
									BlockResolution * RelativeChunkLocationA.x + A.BlockLocation.x,
									BlockResolution * RelativeChunkLocationA.y + A.BlockLocation.y,
									BlockResolution * RelativeChunkLocationA.z + A.BlockLocation.z };
	}*/
};
#undef _USE_MATH_DEFINES