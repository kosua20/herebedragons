#pragma once
#include "input/ControllableCamera.hpp"
#include "resources/Resources.hpp"
#include "Object.hpp"
#include "Pipeline.hpp"

#include "common.hpp"
#include <chrono>

class Renderer
{
public:

	Renderer();

	~Renderer();

	void upload(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat swapchainForma);

	void draw(WGPUQueue queue, WGPUCommandEncoder encoder, WGPUTextureView backbuffer);
	
	void update(const double deltaTime);
	
	void resize(WGPUDevice device, int width, int height);
	
	void clean();
	
private:

	glm::vec2 _size = glm::vec2(0.0f,0.0f);
	double _time = 0.0;
	
	// Scene.
	ControllableCamera _camera;
	std::vector<ShadedObject> _objects;
	Skybox _skybox;

	// GPU data
	Pipeline _objectPipeline;
	Pipeline _skyboxPipeline;
	Pipeline _shadowPipeline;

	WGPUBuffer _objectsUniformBuffer{0};
	WGPUBuffer _frameUniformBuffer{0};

	WGPUBindGroupLayout _uniformGroupLayout{0};
	WGPUBindGroupLayout _staticGroupLayout{0};
	WGPUBindGroup _uniformGroup{0};
	WGPUBindGroup _staticGroup{0};

	WGPUTexture _zbufferTexture{0};
	WGPUTextureView _zbufferTextureView{0};
	WGPUTexture _shadowMapTexture{0};
	WGPUTextureView _shadowMapTextureView{0};

	WGPUSampler _linearSampler{0};
	WGPUSampler _shadowSampler{0};

	size_t _objectUniformStride = 0;

	// Light
	glm::mat4 _lightViewproj;
	glm::vec4 _worldLightDir;
	glm::mat4 _lightProj;

};

