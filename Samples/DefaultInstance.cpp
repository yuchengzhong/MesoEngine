#include "Instance/VoxelWindowsInstance.h"

#if defined(NDEBUG)
constexpr bool kEnableValidationLayers = false;
#else
constexpr bool kEnableValidationLayers = true;
#endif
constexpr bool kPreferIntegratedGPU = false;
constexpr uint32_t kNumSamplesMSAA = 1;
constexpr uint32_t kNumBufferedFrames = 3;

class DefaultVoxelWindowsInstance : public VoxelWindowsInstance
{
public:
};

int main(int argc, char* argv[])
{
    DefaultVoxelWindowsInstance Instance;
    Instance.Initialize({ 
        .bEnableValidationLayers = kEnableValidationLayers, 
        .bPreferIntegratedGPU = kPreferIntegratedGPU, 
        .kNumBufferedFrames = kNumBufferedFrames, 
        .kNumSamplesMSAA = kNumSamplesMSAA 
        });

    // Main loop
    Instance.RunInstance();
    return 0;
}