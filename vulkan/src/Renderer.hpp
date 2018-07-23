#pragma once
#include "Object.hpp"
#include "Skybox.hpp"
#include "ShadowPass.hpp"
#include "Swapchain.hpp"

#include "VulkanUtilities.hpp"
#include "input/ControllableCamera.hpp"

#include "common.hpp"
#include <chrono>



class Renderer
{
public:

	Renderer(Swapchain & swapchain, const int width, const int height);

	~Renderer();

	void encode(const VkQueue & graphicsQueue, const uint32_t imageIndex, VkCommandBuffer & finalCommmandBuffer, VkRenderPassBeginInfo & finalPassInfos, const VkSemaphore & startSemaphore, const VkSemaphore & endSemaphore, const VkFence & submissionFence);
	
	void update(const double deltaTime);
	
	void resize(VkRenderPass & finalRenderPass, const int width, const int height);
	
	void clean();
	
private:
	
	void createPipelines(const VkRenderPass & finalRenderPass);
	void updateUniforms(const uint32_t index);
	
	glm::vec2 _size = glm::vec2(0.0f,0.0f);
	double _time = 0.0;
	
	// Scene.
	std::vector<Object> _objects;
	Skybox _skybox;
	ControllableCamera _camera;
	// Light
	glm::mat4 _lightViewproj;
	glm::vec4 _worldLightDir;
	glm::mat4 _lightProj;
	
	// Vulkan
	VkDevice _device;
	VkDescriptorPool _descriptorPool;
	VkSampler _textureSampler;
	
	// Pipelines
	ShadowPass _shadowPass;
	VkPipelineLayout _objectPipelineLayout;
	VkPipeline _objectPipeline;
	VkPipelineLayout _skyboxPipelineLayout;
	VkPipeline _skyboxPipeline;
	
	// Per frame data.
	std::vector<VkBuffer> _uniformBuffers;
	std::vector<VkDeviceMemory> _uniformBuffersMemory;
	
	
};

