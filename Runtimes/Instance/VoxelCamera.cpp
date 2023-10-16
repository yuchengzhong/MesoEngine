#include "VoxelCamera.h"

void FVoxelCamera::InitializeVoxelCamera(float FovAngle_, float Near_, float Far_)
{
	Fov = float(FovAngle_ * (M_PI / 180.0f));
	Near = Near_;
	Far = Far_;
}

mat4 FVoxelCamera::GetProjectionMatrix(float ViewWidth, float ViewHeight) const
{
	const float AspectRatio = (float)ViewWidth / (float)ViewHeight;
	mat4 Projection = glm::perspective(Fov, AspectRatio, Near, Far);
	return Projection;
}

mat4 FVoxelCamera::GetViewMatrix() const
{
	return FullCamera.getViewMatrix();
}

vec3 FVoxelCamera::GetForwardVector() const
{
	const mat4 ViewMatrix = CameraPositioner.getViewMatrix();
	const vec3 Forward = -vec3(ViewMatrix[0][2], ViewMatrix[1][2], ViewMatrix[2][2]);
	return Forward;
}

FGPUUniformCamera FVoxelCamera::GetCameraUniform(float ViewWidth, float ViewHeight)
{
	return FGPUUniformCamera
	{
		.Projection = GetProjectionMatrix(ViewWidth, ViewHeight),
		.View = GetViewMatrix(),
		.CameraChunkLocation = CameraChunkLocation
	};
}

void FVoxelCamera::UpdateCamera(const FVoxelSceneConfig& CurrentSceneConfig)
{
	vec3 UnfractedCameraPosition = CameraPositioner.getPosition();
	auto Positions = FVoxelMathHelper::ConvertToChunkLocation(UnfractedCameraPosition, CurrentSceneConfig.GetChunkSize());
	const auto& NewCameraChunkLocationOffset = std::get<1>(Positions);
	if (NewCameraChunkLocationOffset != ivec3{0,0,0})
	{
		CameraChunkUpdateCallback();
		CameraChunkLocation += NewCameraChunkLocationOffset;
	}
	vec3 CurrentForward = GetForwardVector();
	if (CameraForward != CurrentForward || NewCameraChunkLocationOffset != ivec3{ 0,0,0 })
	{
		CameraForward = CurrentForward;
		CameraUpdateCallback();
	}
	CameraPositioner.setPosition(std::get<0>(Positions));
}
