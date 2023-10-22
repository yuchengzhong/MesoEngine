// Meso Engine 2024
#pragma once
#include <lvk/LVK.h>
#include <lvk/HelpersImGui.h>
#include <shared/UtilsFPS.h>
//
#include "Shader/ShaderFullScreen.h"
#include "VoxelCamera.h"
#include "Helper/FileHelper.h"
#include "Helper/TimerSet.h"
#include "Voxel/VoxelSceneConfig.h"
#include "Voxel/VoxelStructure.h"

#include "Shader/GPUStructures.h"
struct VoxelInstanceInitialConfig
{
	bool bEnableValidationLayers;
	bool bPreferIntegratedGPU;
	uint32_t kNumBufferedFrames = 3;
	uint32_t kNumSamplesMSAA = 1;
	std::string FontPath = "";
	float CameraFOV = 60.0;
	bool bUseHDRForFinalBuffer = false;

	int WindowsWidth = 1280; //if 0, full screen
	int WindowsHeight = 720;
	bool bInitialFullScreen = true;

	bool bWindowsResizeable = true;
	bool bShowDemoWindow = true;

	uint32_t LosingFocusDelayMillisecond = 250;

	FVoxelSceneConfig VoxelSceneConfig;
};
class VoxelWindowsInstance
{
public:
	std::unique_ptr<lvk::IContext> LVKContext;
	GLFWwindow* LVKWindow = nullptr;

	int WindowsWidth = 0;
	int WindowsHeight = 0;

	bool bLVKEnableValidationLayers = false;
	bool bLVKPreferIntegratedGPU = false;
	uint32_t LVKNumBufferedFrames = 3;
	uint32_t LVKNumSamplesMSAA = 1;
	uint32_t LVKLosingFocusDelayMillisecond = 250;

	FVoxelSceneConfig VoxelSceneConfig;
	FVoxelCamera WindowsCamera;
	glm::vec2 WindowsMousePosition = glm::vec2(0.0f);
	bool bWindowsLeftMousePressed = false;
	bool bWindowsRightMousePressed = false;
	bool bWindowsMiddleMousePressed = false;
	bool bUseHDRForFinalBuffer = false;
	bool bWindowsResizeable = true;
	bool bWindowsFocusing = true;
	//UBOs
	std::vector<lvk::Holder<lvk::BufferHandle>> UBOCamera;

	FGPUUniformSceneConfig SceneConfig;
	lvk::Holder<lvk::BufferHandle> UBOSceneConfig;
	//Renderpass
	lvk::RenderPass RPSwapchain;
	//Pipeline
	lvk::Holder<lvk::RenderPipelineHandle> RPLSwapchain;
	//Textures
	lvk::Holder<lvk::TextureHandle> TEXOffscreenColor; //the last pass, should write to this texture
	//FB
	lvk::Framebuffer FBSwapchain; // swapchain

	ShaderFullScreenVS ShaderFullScreenVSInstance;
	ShaderFullScreenFS ShaderFullScreenFSInstance;

	//IMGUI
	std::unique_ptr<lvk::ImGuiRenderer> WindowsImgui;
	std::string Fonts = (FFileHelper::GetFontPath() + "/simhei.ttf");
	//Debug Timer
	FTimerSet DebugTimerSet;
	//Deconstruction
	virtual ~VoxelWindowsInstance();
	//This function must call before imgui setup
	virtual void SetImguiFont(const std::string& NewFontPath);

	//Initialize
	virtual void Initialize(const VoxelInstanceInitialConfig& InitialConfig);

	virtual void InitializeCameraAndScene(const VoxelInstanceInitialConfig& InitialConfig);
	virtual void InitializeContext();

	//Windows callback
	virtual void InitializeBegin();
	virtual void InitializeWindowsCallBacks();
	virtual void SubInitializeWindowsCallBacks();
	virtual void SubInitializeFramebufferSizeCallback();
	virtual void SubInitializeCursorPosCallback();
	virtual void SubInitializeMouseButtonCallback();
	virtual void SubInitializeScrollCallback();
	virtual void SubInitializeKeyCallback();

	virtual void CreateWindowsFrameBuffer();
	virtual void InitializeRenderPass();

	virtual void InitializeFrameBuffer();

	virtual void InitializePipeline();

	virtual void InitializeImgui();
	virtual void InitializeFinal();

	//Render
	double RenderPreviousTime = 0.0;
	double RenderCurrentTime = 0.0;
	double RenderDeltaTime = 0.0;
	uint32_t RenderGlobalFrameIndex = 0;
	uint32_t RenderFrameIndex = 0;

	FramesPerSecondCounter FPSCounter;
	virtual void InitializeRender();

	virtual void RunInstance();

	virtual void ProcessRenderImgui();
	//IMG
	bool bShowDemoWindow = true;
	virtual void SubprocessRenderImguiInitialize();
	virtual void SubprocessRenderImguiDemoWindow();
	virtual void SubprocessRenderImguiHints();
	virtual void SubprocessRenderImguiFPS();

	virtual void UpdateCamera();
	virtual void UpdatePhysics();
	virtual void UpdateFrameIndex();

	virtual void RenderStart();
	virtual void BindViewportScissor(lvk::ICommandBuffer& Buffer);
	virtual void Render();
	virtual void RenderEnd();
	//
	void DummyWhenCameraChunkUpdate()
	{
		WhenCameraChunkUpdate();
	}
	virtual void WhenCameraChunkUpdate()
	{
		//Donothing
	}
	//
	void DummyWhenCameraUpdate()
	{
		WhenCameraUpdate();
	}
	virtual void WhenCameraUpdate()
	{
		//Donothing
	}
};