#include "Renderer.hpp"
#include "GPU.hpp"
#include "resources/Resources.hpp"
#include "resources/MeshUtilities.hpp"

#define DRAGON_ID 0
#define MONKEY_ID 1
#define PLANE_ID 2

#define SHADOW_MAP_SIZE 1024

const WGPUTextureFormat kDepthBufferFormat = WGPUTextureFormat_Depth32Float;

struct FrameInfos {
	glm::mat4 proj;
	glm::mat4 lightVP;
	glm::vec3 lightDirViewspace;
	float pad[1];
};

struct ModelInfos {
	glm::mat4 MV;
	glm::mat4 MVinverse;
	glm::mat4 M;
	float shininess;
	float pad[3];
};

Renderer::~Renderer(){
}

Renderer::Renderer() {

	// Light settings.
	_lightProj = glm::ortho(-5.0, 5.0, -5.0, 5.0, 0.1, 10.0);
	_lightProj[1][1] *= -1;
	_worldLightDir = glm::normalize(glm::vec4(1.0f,1.0f,1.0f,0.0f));
	glm::mat4 lightView = glm::lookAt(2.0f*glm::vec3(_worldLightDir), glm::vec3(0.0f), glm::vec3(0.0,1.0,0.0));
	_lightViewproj = _lightProj * lightView;

	// Objects
	_objects.resize(3);
	ShadedObject& dragon = _objects[DRAGON_ID];
	ShadedObject& monkey = _objects[MONKEY_ID];
	ShadedObject& plane  = _objects[PLANE_ID];

	dragon.name = "dragon";
	dragon.shininess = 64.f;
	dragon.model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f,0.0f,-0.5f)), glm::vec3(1.2f));

	monkey.name = "suzanne";
	monkey.shininess = 8.f;
	monkey.model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.5, 0.0, 0.5)), glm::vec3(0.65f));

	plane.name = "plane";
	plane.shininess = 32.f;
	plane.model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0,-0.8,0.0)), glm::vec3(2.75f));

	_skybox.name = "cubemap";
	_skybox.model = glm::scale(glm::mat4(1.0f), glm::vec3(15.0f));

	for(Object& object : _objects){
		object.load();
	}
	_skybox.load();
}

void Renderer::upload(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat swapchainFormat){

	// Compute uniform buffers strides.
	WGPUSupportedLimits supportedLimits;
	wgpuDeviceGetLimits(device, &supportedLimits);
	WGPULimits limits = supportedLimits.limits;
	const size_t alignmentConstraint = 256;//(std::max)(256u, limits.minUniformBufferOffsetAlignment ); bug on macOS?

	const size_t objectUBOSizeRatio = (sizeof(ModelInfos) + alignmentConstraint - 1) / alignmentConstraint;
	const size_t objectUniformSize = objectUBOSizeRatio * alignmentConstraint;
	const size_t frameUniformSize = sizeof(FrameInfos);
	_objectUniformStride = objectUniformSize; // total size including stride.

	// Upload objects.
	for(Object& object : _objects){
		object.upload(device, queue);
	}
	_skybox.upload(device, queue);

	// Create sampler.
	{
		WGPUSamplerDescriptor linearDesc{};
		linearDesc.nextInChain = nullptr;
		linearDesc.addressModeU = WGPUAddressMode_Repeat;
		linearDesc.addressModeV = WGPUAddressMode_Repeat;
		linearDesc.addressModeW = WGPUAddressMode_Repeat;
		linearDesc.magFilter = WGPUFilterMode_Linear;
		linearDesc.minFilter = WGPUFilterMode_Linear;
		linearDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
		linearDesc.lodMinClamp = 0.0f;
		linearDesc.lodMaxClamp = FLT_MAX;
		linearDesc.maxAnisotropy = 1;
		linearDesc.compare = WGPUCompareFunction_Undefined;
		_linearSampler = wgpuDeviceCreateSampler(device, &linearDesc);

		WGPUSamplerDescriptor compareDesc{};
		compareDesc.nextInChain = nullptr;
		compareDesc.addressModeU = WGPUAddressMode_ClampToEdge;
		compareDesc.addressModeV = WGPUAddressMode_ClampToEdge;
		compareDesc.addressModeW = WGPUAddressMode_ClampToEdge;
		compareDesc.magFilter = WGPUFilterMode_Linear;
		compareDesc.minFilter = WGPUFilterMode_Linear;
		compareDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
		compareDesc.lodMinClamp = 0.0f;
		compareDesc.lodMaxClamp = 1.0f;
		compareDesc.maxAnisotropy = 1;
		compareDesc.compare = WGPUCompareFunction_Less;
		_shadowSampler = wgpuDeviceCreateSampler(device, &compareDesc);
		
	}

	// Create shadow map
	{
		_shadowMapTexture = GPU::createTexture(device, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 1, WGPUTextureViewDimension_2D, kDepthBufferFormat, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding, _shadowMapTextureView);
	}

	// Create uniform buffers.
	{
		const size_t objectCount = _objects.size() + 1 /* skybox */;
		_objectsUniformBuffer = GPU::createBuffer(device, objectUniformSize * objectCount, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
		_frameUniformBuffer = GPU::createBuffer(device, frameUniformSize, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
	}

	// Create common bind group layouts and the corresponding bindgroups.
	// First, uniforms
	{
		constexpr unsigned int kUniformCount = 2;
		WGPUBindGroupLayoutEntry layoutEntries[kUniformCount];
		WGPUBindGroupEntry entries[kUniformCount];
		for(unsigned int i = 0; i < kUniformCount; ++i){
			GPU::initBindGroupLayoutEntry(layoutEntries[i]);
			GPU::initBindGroupEntry(entries[i]);
		}

		layoutEntries[0].binding = 0;
		layoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
		layoutEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
		layoutEntries[0].buffer.hasDynamicOffset = false;
		layoutEntries[0].buffer.minBindingSize = frameUniformSize;

		layoutEntries[1].binding = 1;
		layoutEntries[1].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
		layoutEntries[1].buffer.type = WGPUBufferBindingType_Uniform;
		layoutEntries[1].buffer.hasDynamicOffset = true;
		layoutEntries[1].buffer.minBindingSize = objectUniformSize;

		entries[0].buffer = _frameUniformBuffer;
		entries[0].size = frameUniformSize;
		entries[0].binding = 0;

		entries[1].buffer = _objectsUniformBuffer;
		entries[1].size = objectUniformSize;
		entries[1].binding = 1;

		WGPUBindGroupLayoutDescriptor groupLayoutDesc{};
		groupLayoutDesc.nextInChain = nullptr;
		groupLayoutDesc.entryCount = kUniformCount;
		groupLayoutDesc.entries = &layoutEntries[0];
		_uniformGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &groupLayoutDesc);

		WGPUBindGroupDescriptor uniformGroupDesc{};
		uniformGroupDesc.nextInChain = nullptr;
		uniformGroupDesc.layout = _uniformGroupLayout;
		uniformGroupDesc.entryCount = kUniformCount;
		uniformGroupDesc.entries = &entries[0];
		_uniformGroup = wgpuDeviceCreateBindGroup(device, &uniformGroupDesc);
	}
	// Samplers.
	{
		constexpr unsigned int kStaticCount = 3;
		WGPUBindGroupLayoutEntry layoutEntries[kStaticCount];
		WGPUBindGroupEntry entries[kStaticCount];
		for(unsigned int i = 0; i < kStaticCount; ++i){
			GPU::initBindGroupLayoutEntry(layoutEntries[i]);
			GPU::initBindGroupEntry(entries[i]);
		}

		layoutEntries[0].binding = 0;
		layoutEntries[0].visibility = WGPUShaderStage_Fragment;
		layoutEntries[0].sampler.type = WGPUSamplerBindingType_Filtering;

		layoutEntries[1].binding = 1;
		layoutEntries[1].visibility = WGPUShaderStage_Fragment;
		layoutEntries[1].sampler.type = WGPUSamplerBindingType_Comparison;

		layoutEntries[2].binding = 2;
		layoutEntries[2].visibility = WGPUShaderStage_Fragment;
		layoutEntries[2].texture.viewDimension = WGPUTextureViewDimension_2D;
		layoutEntries[2].texture.sampleType = WGPUTextureSampleType_Depth;

		entries[0].binding = 0;
		entries[0].sampler = _linearSampler;

		entries[1].binding = 1;
		entries[1].sampler = _shadowSampler;

		entries[2].binding = 2;
		entries[2].textureView = _shadowMapTextureView;

		WGPUBindGroupLayoutDescriptor groupLayoutDesc{};
		groupLayoutDesc.nextInChain = nullptr;
		groupLayoutDesc.entryCount = kStaticCount;
		groupLayoutDesc.entries = &layoutEntries[0];
		_staticGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &groupLayoutDesc);

		WGPUBindGroupDescriptor samplerGroupDesc{};
		samplerGroupDesc.nextInChain = nullptr;
		samplerGroupDesc.layout = _staticGroupLayout;
		samplerGroupDesc.entryCount = kStaticCount;
		samplerGroupDesc.entries = &entries[0];
		_staticGroup = wgpuDeviceCreateBindGroup(device, &samplerGroupDesc);

	}
	
	// Create pipelines.
	{
		Pipeline::Settings basicPipeline;
		basicPipeline.textures = {WGPUTextureViewDimension_2D, WGPUTextureViewDimension_2D};
		basicPipeline.uniformLayout = _uniformGroupLayout;
		basicPipeline.staticLayout = _staticGroupLayout;
		basicPipeline.attributes = Pipeline::Settings::Attribute::ALL;
		basicPipeline.module = "object";
		basicPipeline.vertexEntry = "mainVertex";
		basicPipeline.fragmentEntry = "mainFragment";
		basicPipeline.depthFormat = kDepthBufferFormat;
		basicPipeline.colorFormat = swapchainFormat;
		basicPipeline.cullMode = WGPUCullMode_Back;
		_objectPipeline.upload(basicPipeline, device);

		Pipeline::Settings skyboxPipeline;
		skyboxPipeline.textures = { WGPUTextureViewDimension_Cube };
		skyboxPipeline.uniformLayout = _uniformGroupLayout;
		skyboxPipeline.staticLayout = _staticGroupLayout;
		skyboxPipeline.attributes = Pipeline::Settings::Attribute::POSITION | Pipeline::Settings::Attribute::UV;
		skyboxPipeline.module = "skybox";
		skyboxPipeline.vertexEntry = "mainVertex";
		skyboxPipeline.fragmentEntry = "mainFragment";
		skyboxPipeline.depthFormat = kDepthBufferFormat;
		skyboxPipeline.colorFormat = swapchainFormat;
		skyboxPipeline.cullMode = WGPUCullMode_Front;
		_skyboxPipeline.upload(skyboxPipeline, device);

		Pipeline::Settings shadowPipeline;
		shadowPipeline.textures = {};
		shadowPipeline.uniformLayout = _uniformGroupLayout;
		shadowPipeline.staticLayout = _staticGroupLayout;
		shadowPipeline.attributes = Pipeline::Settings::Attribute::POSITION;
		shadowPipeline.module = "object";
		shadowPipeline.vertexEntry = "mainVertexShadow";
		shadowPipeline.fragmentEntry = ""; // No fragment shader
		shadowPipeline.depthFormat = kDepthBufferFormat;
		shadowPipeline.colorFormat = WGPUTextureFormat_Undefined;
		shadowPipeline.cullMode = WGPUCullMode_Back;
		shadowPipeline.depthBias = true;
		_shadowPipeline.upload(shadowPipeline, device);
	}
}

void Renderer::draw(WGPUQueue queue, WGPUCommandEncoder encoder, WGPUTextureView backbuffer){

	// Update all uniforms
	{
		FrameInfos frameInfos;
		frameInfos.proj = _camera.projection();
		frameInfos.lightVP = _lightViewproj;
		frameInfos.lightDirViewspace = glm::vec3(_camera.view() * _worldLightDir);
		wgpuQueueWriteBuffer(queue, _frameUniformBuffer, 0, &frameInfos, sizeof(FrameInfos));

		unsigned int objectIndex = 0;
		for(const ShadedObject& object : _objects){
			ModelInfos modelInfos;
			modelInfos.M = object.model;
			modelInfos.MV = _camera.view() * object.model;
			modelInfos.MVinverse = glm::transpose(glm::inverse(modelInfos.MV));
			modelInfos.shininess = object.shininess;
			wgpuQueueWriteBuffer(queue, _objectsUniformBuffer, _objectUniformStride * objectIndex, &modelInfos, sizeof(ModelInfos));
			++objectIndex;
		}
		// Append skybox.
		{
			ModelInfos modelInfos;
			modelInfos.M = _skybox.model;
			modelInfos.MV = _camera.view() * _skybox.model;
			modelInfos.MVinverse = glm::transpose(glm::inverse(modelInfos.MV));
			modelInfos.shininess = 0.f;
			wgpuQueueWriteBuffer(queue, _objectsUniformBuffer, _objectUniformStride * objectIndex, &modelInfos, sizeof(ModelInfos));
		}
	}

	// ---- Shadow pass
	{
		// Draw to zbuffer only.
		WGPURenderPassDepthStencilAttachment depthAttachment{};
		depthAttachment.view = _shadowMapTextureView;
		depthAttachment.depthClearValue = 1.f;
		depthAttachment.depthLoadOp = WGPULoadOp_Clear;
		depthAttachment.depthStoreOp = WGPUStoreOp_Store;
		depthAttachment.depthReadOnly = false;
		depthAttachment.stencilLoadOp = WGPULoadOp_Undefined;
		depthAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
		depthAttachment.stencilClearValue = 0;
		depthAttachment.stencilReadOnly = true;

		WGPURenderPassDescriptor passDesc{};
		passDesc.nextInChain = nullptr;
		passDesc.colorAttachmentCount = 0;
		passDesc.colorAttachments = nullptr;
		passDesc.depthStencilAttachment = &depthAttachment;
		WGPURenderPassEncoder shadowPass = wgpuCommandEncoderBeginRenderPass(encoder, &passDesc);

		wgpuRenderPassEncoderSetViewport(shadowPass, 0.f, 0.f, float(SHADOW_MAP_SIZE), float(SHADOW_MAP_SIZE), 0.0f, 1.0f);
		wgpuRenderPassEncoderSetPipeline(shadowPass, _shadowPipeline.pipeline());

		// Render objects in shadow map.
		unsigned int objectIndex = 0;
		for(const ShadedObject& obj : _objects){
			unsigned int offset = objectIndex * _objectUniformStride;
			wgpuRenderPassEncoderSetBindGroup(shadowPass, 0, _uniformGroup, 1, &offset); // uniforms
			wgpuRenderPassEncoderSetVertexBuffer(shadowPass, 0, obj.vertexBuffer, 0,  obj.vertexBufferSize);
			wgpuRenderPassEncoderSetIndexBuffer(shadowPass, obj.indexBuffer, WGPUIndexFormat_Uint32, 0, obj.indexBufferSize);
			wgpuRenderPassEncoderDrawIndexed(shadowPass, obj.indexCount, 1, 0, 0, 0);
			++objectIndex;
		}
		wgpuRenderPassEncoderEnd(shadowPass);
	}
	
	// ---- Main pass
	{
		// Draw to backbuffer.
		WGPURenderPassColorAttachment colorAttachment{};
		colorAttachment.nextInChain = nullptr;
		colorAttachment.resolveTarget = nullptr;
		colorAttachment.view = backbuffer;
		colorAttachment.loadOp = WGPULoadOp_Clear;
		colorAttachment.storeOp = WGPUStoreOp_Store;
		colorAttachment.clearValue = {1.0f, 0.2f, 0.5f, 1.f};
		// And our zbuffer
		WGPURenderPassDepthStencilAttachment depthAttachment{};
		depthAttachment.view = _zbufferTextureView;
		depthAttachment.depthClearValue = 1.f;
		depthAttachment.depthLoadOp = WGPULoadOp_Clear;
		depthAttachment.depthStoreOp = WGPUStoreOp_Store;
		depthAttachment.depthReadOnly = false;
		depthAttachment.stencilLoadOp = WGPULoadOp_Undefined;
		depthAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
		depthAttachment.stencilClearValue = 0;
		depthAttachment.stencilReadOnly = true;

		WGPURenderPassDescriptor finalPassDesc{};
		finalPassDesc.nextInChain = nullptr;
		finalPassDesc.colorAttachmentCount = 1;
		finalPassDesc.colorAttachments = &colorAttachment;
		finalPassDesc.depthStencilAttachment = &depthAttachment;
		WGPURenderPassEncoder finalPass = wgpuCommandEncoderBeginRenderPass(encoder, &finalPassDesc);
		wgpuRenderPassEncoderSetViewport(finalPass, 0.f, 0.f, _size[0], _size[1], 0.0f, 1.0f);

		// Render objects.
		unsigned int objectIndex = 0;
		{
			wgpuRenderPassEncoderSetPipeline(finalPass, _objectPipeline.pipeline());
			wgpuRenderPassEncoderSetBindGroup(finalPass, 2, _staticGroup, 0, nullptr);

			for(const ShadedObject& obj : _objects){

				unsigned int offset = objectIndex * _objectUniformStride;
				wgpuRenderPassEncoderSetBindGroup(finalPass, 0, _uniformGroup, 1, &offset); // uniforms
				wgpuRenderPassEncoderSetBindGroup(finalPass, 1, obj.textureGroup, 0, nullptr); // textures
				wgpuRenderPassEncoderSetVertexBuffer(finalPass, 0, obj.vertexBuffer, 0,  obj.vertexBufferSize);
				wgpuRenderPassEncoderSetIndexBuffer(finalPass, obj.indexBuffer, WGPUIndexFormat_Uint32, 0, obj.indexBufferSize);
				wgpuRenderPassEncoderDrawIndexed(finalPass, obj.indexCount, 1, 0, 0, 0);
				++objectIndex;
			}
		}
		// And the skybox.
		{
			wgpuRenderPassEncoderSetPipeline(finalPass, _skyboxPipeline.pipeline());
			wgpuRenderPassEncoderSetBindGroup(finalPass, 2, _staticGroup, 0, nullptr);

			unsigned int offset = objectIndex * _objectUniformStride;
			wgpuRenderPassEncoderSetBindGroup(finalPass, 0, _uniformGroup, 1, &offset); // uniforms
			wgpuRenderPassEncoderSetBindGroup(finalPass, 1, _skybox.textureGroup, 0, nullptr); // textures (per object)
			wgpuRenderPassEncoderSetVertexBuffer(finalPass, 0, _skybox.vertexBuffer, 0,  _skybox.vertexBufferSize);
			wgpuRenderPassEncoderSetIndexBuffer(finalPass, _skybox.indexBuffer, WGPUIndexFormat_Uint32, 0, _skybox.indexBufferSize);
			wgpuRenderPassEncoderDrawIndexed(finalPass, _skybox.indexCount, 1, 0, 0, 0);
		}

		wgpuRenderPassEncoderEnd(finalPass);

	}
}

void Renderer::update(double deltaTime) {
	_time += deltaTime;
	_camera.update();
	_camera.physics(deltaTime);
	
	_worldLightDir = glm::normalize(glm::vec4(1.0,0.5*sin(_time)+0.6, 1.0,0.0));
	glm::mat4 lightView = glm::lookAt(2.0f*glm::vec3(_worldLightDir), glm::vec3(0.0f), glm::vec3(0.0,1.0,0.0));
	_lightViewproj = _lightProj * lightView;

	_objects[MONKEY_ID].model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.5,0.0,0.5)), float(fmod(_time, 2*M_PI)), glm::vec3(0.0f,1.0f,0.0f)) , glm::vec3(0.65));
}

void Renderer::resize(WGPUDevice device, int width, int height){
	if(width == _size[0] && height == _size[1]){
		return;
	}
	_camera.ratio(float(width)/float(height));
	_size[0] = width; _size[1] = height;

	if(_zbufferTextureView){
		wgpuTextureViewRelease(_zbufferTextureView);
	}
	if(_zbufferTexture){
		wgpuTextureDestroy(_zbufferTexture);
		wgpuTextureRelease(_zbufferTexture);
	}

	// Resize depth buffer.
	_zbufferTexture = GPU::createTexture(device, width, height, 1, WGPUTextureViewDimension_2D, kDepthBufferFormat, WGPUTextureUsage_RenderAttachment, _zbufferTextureView);

}

void Renderer::clean(){
	for(Object& object : _objects){
		object.clean();
	}
	_skybox.clean();

	_objectPipeline.clean();
	_shadowPipeline.clean();
	_skyboxPipeline.clean();

	wgpuBindGroupRelease(_uniformGroup);
	wgpuBindGroupRelease(_staticGroup);
	wgpuBindGroupLayoutRelease(_uniformGroupLayout);
	wgpuBindGroupLayoutRelease(_staticGroupLayout);

	wgpuBufferDestroy(_objectsUniformBuffer);
	wgpuBufferRelease(_objectsUniformBuffer);
	wgpuBufferDestroy(_frameUniformBuffer);
	wgpuBufferRelease(_frameUniformBuffer);
	wgpuSamplerRelease(_linearSampler);
	wgpuSamplerRelease(_shadowSampler);

	wgpuTextureViewRelease(_zbufferTextureView);
	wgpuTextureDestroy(_zbufferTexture);
	wgpuTextureRelease(_zbufferTexture);
}

