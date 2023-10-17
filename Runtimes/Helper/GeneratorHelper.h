// Meso Engine 2024
#pragma once
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include "VoxelMathHelper.h"
#include "FileHelper.h"
#include "Voxel/Chunk/Chunk.h"
using glm::vec3;
using glm::vec4;
using glm::ivec3;
using glm::dvec3;
using glm::ivec4;
using glm::u8vec3;
using glm::u8vec4;

struct FGeneratorHelper
{
    template<typename T>
    inline static glm::tvec4<T, glm::defaultp> noised(glm::tvec3<T, glm::defaultp> x)
    {
        // https://iquilezles.org/articles/gradientnoise
        glm::tvec3<T, glm::defaultp> p = floor(x);
        glm::tvec3<T, glm::defaultp> w = fract(x);

        glm::tvec3<T, glm::defaultp> u = w * w * w * (w * (w * static_cast<T>(6.0) - static_cast<T>(15.0)) + static_cast<T>(10.0));
        glm::tvec3<T, glm::defaultp> du = static_cast<T>(30.0) * w * w * (w * (w - static_cast<T>(2.0)) + static_cast<T>(1.0));

        T a = FVoxelMathHelper::Hash(p + glm::tvec3<T, glm::defaultp>(0, 0, 0));
        T b = FVoxelMathHelper::Hash(p + glm::tvec3<T, glm::defaultp>(1, 0, 0));
        T c = FVoxelMathHelper::Hash(p + glm::tvec3<T, glm::defaultp>(0, 1, 0));
        T d = FVoxelMathHelper::Hash(p + glm::tvec3<T, glm::defaultp>(1, 1, 0));
        T e = FVoxelMathHelper::Hash(p + glm::tvec3<T, glm::defaultp>(0, 0, 1));
        T f = FVoxelMathHelper::Hash(p + glm::tvec3<T, glm::defaultp>(1, 0, 1));
        T g = FVoxelMathHelper::Hash(p + glm::tvec3<T, glm::defaultp>(0, 1, 1));
        T h = FVoxelMathHelper::Hash(p + glm::tvec3<T, glm::defaultp>(1, 1, 1));

        T k0 = a;
        T k1 = b - a;
        T k2 = c - a;
        T k3 = e - a;
        T k4 = a - b - c + d;
        T k5 = a - c - e + g;
        T k6 = a - b - e + f;
        T k7 = -a + b + c - d + e - f - g + h;

        glm::tvec4<T, glm::defaultp> result = glm::tvec4<T, glm::defaultp>(
            -static_cast<T>(1.0) + static_cast<T>(2.0) * (k0 + k1 * u.x + k2 * u.y + k3 * u.z + k4 * u.x * u.y + k5 * u.y * u.z + k6 * u.z * u.x + k7 * u.x * u.y * u.z),
            static_cast<T>(2.0) * du * glm::tvec3<T, glm::defaultp>(k1 + k4 * u.y + k6 * u.z + k7 * u.y * u.z,
                k2 + k5 * u.z + k4 * u.x + k7 * u.z * u.x,
                k3 + k6 * u.x + k5 * u.y + k7 * u.x * u.y));
        //.yzwx
        return glm::tvec4<T, glm::defaultp>{result.y, result.z, result.w, result.x};
    }

    template<typename T>
    inline static T displacement(glm::tvec3<T, glm::defaultp> p)
    {
        // more cool tricks -> https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch01.html
        glm::tvec3<T, glm::defaultp> pp = p;
        T mgn = static_cast<T>(0.5);
        T d = static_cast<T>(0.0);
        T s = static_cast<T>(1.0);
        for (int i = 0; i < 5; i++)
        {
            glm::tvec4<T, glm::defaultp> rnd = noised(p + static_cast<T>(10.0));
            d += rnd.w * mgn;

            p *= static_cast<T>(2.0);
            p += glm::tvec3<T, glm::defaultp>{rnd.x, rnd.y, rnd.z} *static_cast<T>(0.2)* s;
            if (i == 2)
            {
                s *= static_cast<T>(-1.0);
            }
            mgn *= static_cast<T>(0.5);
        }

        p = pp * pow(static_cast<T>(2.0), 5);
        for (int i = 0; i < 4; i++)
        {
            glm::tvec4<T, glm::defaultp> rnd = noised(p);
            d += rnd.w * mgn;

            p *= static_cast<T>(2.0);
            mgn *= static_cast<T>(0.5);
        }
        return d;
    }
    //Todo: can be optimize by octree
    //If using neural, can train a "min sdf" layered network for fast culling
    inline static FChunk TestGenerator(ivec3 StartLocation, float BlockSize, unsigned char ChunkResolution, uint32_t MipmapLevel)
    {
        FChunk Result;
        Result.ChunkLocation = StartLocation;
        using uchar = unsigned char;
        for (uint32_t X = 0; X < ChunkResolution; X++)
        {
            for (uint32_t Y = 0; Y < ChunkResolution; Y++)
            {
                for (uint32_t Z = 0; Z < ChunkResolution; Z++)
                {
                    dvec3 ChunkStartLocation = (dvec3)StartLocation * (double)BlockSize * (double)ChunkResolution;
                    dvec3 BlockCenterLocation = ChunkStartLocation + dvec3{ X,Y,Z } *(double)BlockSize;

                    double d = (BlockCenterLocation.y * .5 + (displacement(BlockCenterLocation * .1)) * 10.3) * .4;
                    if (d < 0.0)
                    {
                        Result.Blocks.push_back(
                            {
                                .ChunkIndex = 0,//Maybe not neccesary, can do in gather
                                .BlockLocation = {(uchar)X,(uchar)Y,(uchar)Z},
                                .VolumeIndex = 0,
                            }
                        );
                    }
                }
            }
        }
        return Result;// Result;
    }
	inline static FChunk TestGenerator2(ivec3 StartLocation, float BlockSize, unsigned char ChunkResolution, uint32_t MipmapLevel)
	{
        FChunk Result;
        Result.ChunkLocation = StartLocation;
		for (uint32_t X = 0; X < ChunkResolution; X++)
		{
			for (uint32_t Y = 0; Y < ChunkResolution; Y++)
			{
				for (uint32_t Z = 0; Z < ChunkResolution; Z++)
				{
					dvec3 ChunkStartLocation = (dvec3)StartLocation * (double)BlockSize * (double)ChunkResolution;
					dvec3 BlockCenterLocation = ChunkStartLocation + dvec3{ X,Y,Z } *(double)BlockSize;

                    double d = noised(BlockCenterLocation).x;
				}
			}
		}
        return {};// Result;
	}
};