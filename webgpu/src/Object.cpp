#include "Object.hpp"
#include "GPU.hpp"
#include "resources/Resources.hpp"
#include "resources/MeshUtilities.hpp"

void Object::load(){
	const std::string meshPath = "resources/meshes/" + name + ".obj";
	MeshUtilities::loadObj(meshPath, geometry, MeshUtilities::Indexed);
	MeshUtilities::centerAndUnitMesh(geometry);
	MeshUtilities::computeTangentsAndBinormals(geometry);
}

void Object::upload(WGPUDevice device, WGPUQueue queue){
	GPU::uploadMesh(geometry, device, queue, vertexBuffer, indexBuffer);
	vertexBufferSize = sizeof(Vertex)   * geometry.vertices.size();
	indexBufferSize  = sizeof(uint32_t) * geometry.indices.size();
	indexCount = geometry.indices.size();
}

void Object::clean(){
	wgpuBufferDestroy(indexBuffer);
	wgpuBufferDestroy(vertexBuffer);

	wgpuBufferRelease(indexBuffer);
	wgpuBufferRelease(vertexBuffer);
	if(textureGroup){
		wgpuBindGroupRelease(textureGroup);
		wgpuBindGroupLayoutRelease(textureGroupLayout);
	}
}

void ShadedObject::load(){
	const std::string albedoPath = "resources/textures/" + name + "_texture_color.png";
	const std::string normalPath = "resources/textures/" + name + "_texture_normal.png";
	Resources::loadImage(albedoPath, albedo, true);
	Resources::loadImage(normalPath, normal, true);
	Object::load();
}
void ShadedObject::upload(WGPUDevice device, WGPUQueue queue){
	Object::upload(device, queue);
	GPU::uploadImage(albedo, device, queue, albedoTexture, albedoView);
	GPU::uploadImage(normal, device, queue, normalTexture, normalView);

	// Rebuild the layout... and bind group.
	WGPUBindGroupLayoutEntry layoutEntries[2];
	WGPUBindGroupEntry entries[2];
	for(unsigned int i = 0; i < 2; ++i){
		GPU::initBindGroupLayoutEntry(layoutEntries[i]);
		GPU::initBindGroupEntry(entries[i]);

		layoutEntries[i].binding = i;
		layoutEntries[i].visibility = WGPUShaderStage_Fragment;
		layoutEntries[i].texture.sampleType = WGPUTextureSampleType_Float;
		layoutEntries[i].texture.viewDimension = WGPUTextureViewDimension_2D;
		entries[i].binding = i;
	}
	entries[0].textureView = albedoView;
	entries[1].textureView = normalView;

	WGPUBindGroupLayoutDescriptor layoutDesc{};
	layoutDesc.nextInChain = nullptr;
	layoutDesc.entryCount = 2;
	layoutDesc.entries = &layoutEntries[0];
	textureGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &layoutDesc);

	WGPUBindGroupDescriptor groupDesc{};
	groupDesc.nextInChain = nullptr;
	groupDesc.layout = textureGroupLayout;
	groupDesc.entryCount = 2;
	groupDesc.entries = &entries[0];
	textureGroup = wgpuDeviceCreateBindGroup(device, &groupDesc);
}

void ShadedObject::clean(){
	wgpuTextureViewRelease(albedoView);
	wgpuTextureViewRelease(normalView);
	wgpuTextureDestroy(albedoTexture);
	wgpuTextureDestroy(normalTexture);
	wgpuTextureRelease(albedoTexture);
	wgpuTextureRelease(normalTexture);
	Object::clean();
}

void Skybox::load(){
	const std::string prefixPath = "resources/textures/" + name;
	// Same order as Vulkan, phew.
	const std::string suffixes[] = {
		"_r", "_l", "_u", "_d", "_b","_f",
	};
	for(unsigned int i = 0; i < 6; ++i){
		const std::string facePath = prefixPath + suffixes[i] + ".png";
		Resources::loadImage(facePath, faces[i], false);
	}
	Object::load();
}
void Skybox::upload(WGPUDevice device, WGPUQueue queue){
	GPU::uploadCubemap(faces, device, queue, cubemap, cubemapView);
	Object::upload(device, queue);

	WGPUBindGroupLayoutEntry layoutEntries[1];
	WGPUBindGroupEntry entries[1];
	for(unsigned int i = 0; i < 1; ++i){
		GPU::initBindGroupLayoutEntry(layoutEntries[i]);
		GPU::initBindGroupEntry(entries[i]);

		layoutEntries[i].binding = i;
		layoutEntries[i].visibility = WGPUShaderStage_Fragment;
		layoutEntries[i].texture.sampleType = WGPUTextureSampleType_Float;
		layoutEntries[i].texture.viewDimension = WGPUTextureViewDimension_Cube;
		entries[i].binding = i;
	}
	entries[0].textureView = cubemapView;

	WGPUBindGroupLayoutDescriptor layoutDesc{};
	layoutDesc.nextInChain = nullptr;
	layoutDesc.entryCount = 1;
	layoutDesc.entries = &layoutEntries[0];
	textureGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &layoutDesc);

	WGPUBindGroupDescriptor groupDesc{};
	groupDesc.nextInChain = nullptr;
	groupDesc.layout = textureGroupLayout;
	groupDesc.entryCount = 1;
	groupDesc.entries = &entries[0];
	textureGroup = wgpuDeviceCreateBindGroup(device, &groupDesc);
}
void Skybox::clean(){
	wgpuTextureViewRelease(cubemapView);
	wgpuTextureDestroy(cubemap);
	wgpuTextureRelease(cubemap);
	Object::clean();
}
