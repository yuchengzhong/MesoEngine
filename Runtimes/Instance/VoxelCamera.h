#pragma once
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#define _USE_MATH_DEFINES
#include <math.h>
#include <functional>
#include <shared/Camera.h>
#include "Voxel/VoxelStructure.h"
#include "Voxel/VoxelSceneConfig.h"
#include "Shader/GPUStructures.h"

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::ivec3;


class FVoxelCamera
{
public:
	CameraPositioner_FirstPerson CameraPositioner = CameraPositioner_FirstPerson(vec3(5.0f, 2.0f, 2.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));//cameraPosition_,cameraOrientation_,up_
	Camera FullCamera = Camera(CameraPositioner);

	float Fov = float(45.0f * (M_PI / 180.0f));
	float Near = 0.1f;
	float Far = 1000.0f;
	ivec3 CameraChunkLocation = {0,0,0};
	vec3 CameraForward = { 0,0,0 };
	void InitializeVoxelCamera(float FovAngle_ = 45.0f, float Near_ = 0.5f, float Far_ = 1000.0f);

	mat4 GetProjectionMatrix(float ViewWidth = 1920.0f, float ViewHeight = 1280.0f) const;
	mat4 GetViewMatrix() const;
	vec3 GetForwardVector() const;
	FGPUUniformCamera GetCameraUniform(float ViewWidth = 1920.0f, float ViewHeight = 1280.0f);

	//Camera callback
	using CameraChunkUpdateCallbackType = std::function<void()>;
	CameraChunkUpdateCallbackType CameraChunkUpdateCallback;
	void SetCameraChunkUpdateCallback(CameraChunkUpdateCallbackType CameraChunkUpdateCallback_)
	{
		CameraChunkUpdateCallback = std::move(CameraChunkUpdateCallback_);
	}

	//Including View
	using CameraUpdateCallbackType = std::function<void()>;
	CameraUpdateCallbackType CameraUpdateCallback;
	void SetCameraUpdateCallback(CameraUpdateCallbackType CameraUpdateCallback_) //Including View
	{
		CameraUpdateCallback = std::move(CameraUpdateCallback_);
	}

	void UpdateCamera(const FVoxelSceneConfig& CurrentSceneConfig);
};
#undef _USE_MATH_DEFINES