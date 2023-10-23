#include <boost/dynamic_bitset.hpp>
#include "Helper/VoxelMathHelper.h"


struct FBinaryOccupancyVolume
{
    using FOccupancyValue = bool;

	boost::dynamic_bitset<> OccupancyVolume;
    uint32_t Resolution = 0;
    FBinaryOccupancyVolume() = default;
    FBinaryOccupancyVolume(uint32_t Resolution_) : Resolution(Resolution_), OccupancyVolume(Resolution_* Resolution_* Resolution_) {}

    void Set(FOccupancyValue Value, ivec3 Location)
    {
        //uint32_t PhysicIndex = Location.x + Location.y * Resolution + Location.z * Resolution * Resolution;
        uint32_t PhysicIndex = FVoxelMathHelper::Convert3DTo1DClamped(Location, { Resolution ,Resolution ,Resolution });
        OccupancyVolume[PhysicIndex] = Value;
    }
    FOccupancyValue GetClamped(ivec3 Location) const
    {
        uint32_t PhysicIndex = FVoxelMathHelper::Convert3DTo1DClamped(Location, { Resolution ,Resolution ,Resolution });
        return OccupancyVolume[PhysicIndex];
    }
    FOccupancyValue Get(ivec3 Location) const
    {
        uint32_t PhysicIndex = FVoxelMathHelper::Convert3DTo1D(Location, { Resolution ,Resolution ,Resolution });
        return OccupancyVolume[PhysicIndex];
    }
    FOccupancyValue GetWithBoundaryCondition(ivec3 Location, FOccupancyValue BoundaryValue = 0) const
    {
        if (FVoxelMathHelper::bIsOutOfBound(Location, { Resolution ,Resolution ,Resolution }))
        {
            return BoundaryValue;
        }
        uint32_t PhysicIndex = FVoxelMathHelper::Convert3DTo1D(Location, { Resolution ,Resolution ,Resolution });
        return OccupancyVolume[PhysicIndex];
    }
};

// should move to helper/folder but it's messy now
struct FOccupancyHelper
{
    //Moore neighborhood
    inline static std::vector<ivec3> Get26Offsets()
    {
        return
        {
            {-1, -1, -1}, {-1, -1, 0}, {-1, -1, 1},
            {-1, 0, -1} , {-1, 0, 0} , {-1, 0, 1},
            {-1, 1, -1} , {-1, 1, 0} , {-1, 1, 1},

            {0, -1, -1}, {0, -1, 0}, {0, -1, 1},
            {0, 0, -1} , /*SELF*/    {0, 0, 1},
            {0, 1, -1} , {0, 1, 0} , {0, 1, 1},

            {1, -1, -1}, {1, -1, 0}, {1, -1, 1},
            {1, 0, -1} , {1, 0, 0} , {1, 0, 1},
            {1, 1, -1} , {1, 1, 0} , {1, 1, 1}
        };
    }

    //Von Neumann neighborhood
    inline static std::vector<ivec3> Get6Offsets()
    {
        return
        {
            {-1, 0, 0},
            {0, -1, 0},
            {0, 0, -1}, 
            {0, 0, 1},
            {0, 1, 0},
            {1, 0, 0}
        };
    }
    template<bool bUseComplex = true>
    inline static FBinaryOccupancyVolume::FOccupancyValue ErodeSingleVoxel(const FBinaryOccupancyVolume& BaseOccupancyVolume, ivec3 Location)
    {
        if (FVoxelMathHelper::bIsOutOfBoundThickness(Location, { BaseOccupancyVolume.Resolution,BaseOccupancyVolume.Resolution ,BaseOccupancyVolume.Resolution }, 1u))
        {
            return 0;
        }
        std::vector<ivec3> Offsets;
        constexpr uint32_t NeighbourNum = bUseComplex ? 26 : 6;
        if constexpr (bUseComplex)
        {
            Offsets = Get26Offsets();
        }
        else
        {
            Offsets = Get6Offsets();
        }
        FBinaryOccupancyVolume::FOccupancyValue ErodeResult = true;
        for (uint32_t i = 0; i < NeighbourNum; i++)
        {
            ErodeResult &= BaseOccupancyVolume.Get(Offsets[i] + Location);
        }
        return ErodeResult;
    }
};