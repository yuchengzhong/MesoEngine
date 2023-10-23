// Meso Engine 2024
#pragma once
#include <LVK.h>
#include "ShaderBase.h"
#include "GPUStructures.h"

class ShaderWireFrameVS :public ShaderBase
{
public:
    lvk::ShaderStage GetShaderStage() const override
    {
        return lvk::Stage_Vert;
    }
    std::string GetShaderDebugInfo() const override
    {
        return std::string("Shader Module: fullscreen (vert)");
    }
    std::string GetShaderString() const override
    {
        return FGPUSimpleInstanceData::GetInstanceLayoutShader(0) + FGPUUniformCamera::GetStructureShader() + FGPUUniformSceneConfig::GetStructureShader() +
R"(
#define SATURATION 0.5
layout (location=0) out vec3 OutColor;

layout(push_constant) uniform constants
{
	CameraInfo Camera;
	SceneInfo Scene;
} pc;

void main() 
{
    mat4 Projection = pc.Camera.Projection;
    mat4 View = pc.Camera.View;

    ivec3 ChunkOffset = InstanceChunkLocation - pc.Camera.CameraChunkLocation;
    vec3 RealVertexPosition = VertexPosition * InstanceScale + vec3(ChunkOffset) * pc.Scene.ChunkSize;
    gl_Position = Projection * View * vec4(RealVertexPosition, 1.0);
    OutColor = InstanceMarker > 0.5 ? vec3(SATURATION, SATURATION, 1.0) : vec3(SATURATION, 1.0, SATURATION);

    float DistanceInverse = 1.0 / max(1.0, (length(vec3(ChunkOffset)) - 12.0));
    OutColor = mix(vec3(1.0,SATURATION,SATURATION), OutColor, DistanceInverse);
}
)";
    }
};

class ShaderWireFrameFS :public ShaderBase
{
public:
    lvk::ShaderStage GetShaderStage() const override
    {
        return lvk::Stage_Frag;
    }
    std::string GetShaderDebugInfo() const override
    {
        return std::string("Shader Module: fullscreen (frag)");
    }
    std::string GetShaderString() const override
    {
        return R"(
layout (location=0) in vec3 InColor;
layout (location=0) out vec4 out_FragColor;

void main() 
{
  out_FragColor = vec4(InColor, 1.0);
};
)";
    }
};