#pragma once
#include "resources/Resources.hpp"

#include "common.hpp"

class Object {
public:

	virtual void load();
	virtual void upload(WGPUDevice device, WGPUQueue queue);
	virtual void clean();

	// CPU data.
	Mesh geometry;
	// GPU data.
	WGPUBuffer vertexBuffer{0};
	WGPUBuffer indexBuffer{0};
	WGPUBindGroup textureGroup{0};
	WGPUBindGroupLayout textureGroupLayout{0};

	// Infos
	std::string name;
	glm::mat4 model{1.f};
	uint32_t vertexBufferSize{0};
	uint32_t indexBufferSize{0};
	uint32_t indexCount{0};
};

class ShadedObject : public Object {
public:

	void load() override;
	void upload(WGPUDevice device, WGPUQueue queue)  override;
	void clean()  override;

	// CPU data.
	Image albedo;
	Image normal;
	// GPU data.
	WGPUTexture albedoTexture{0};
	WGPUTextureView albedoView{0};
	WGPUTexture normalTexture{0};
	WGPUTextureView normalView{0};
	//Infos
	float shininess;
};

class Skybox : public Object {
public:

	void load() override;
	void upload(WGPUDevice device, WGPUQueue queue)  override;
	void clean()  override;

	// CPU data
	std::array<Image, 6> faces;
	// GPU data
	WGPUTexture cubemap{0};
	WGPUTextureView cubemapView{0};
};
