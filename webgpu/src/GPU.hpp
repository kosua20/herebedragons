#pragma once

#include "common.hpp"

struct Mesh;
struct Image;

class GPU {
public:

	static void init(WGPUDevice device, size_t uboAlignment);

	static void initBindGroupEntry(WGPUBindGroupEntry& entry);

	static void initBindGroupLayoutEntry(WGPUBindGroupLayoutEntry& entry);

	static WGPUBuffer createBuffer(WGPUDevice device, size_t size, WGPUBufferUsageFlags usage);

	static WGPUTexture createTexture(WGPUDevice device, size_t w, size_t h, size_t m, WGPUTextureViewDimension dims, WGPUTextureFormat format, WGPUTextureUsageFlags usage, WGPUTextureView& view);

	// Upload mesh
	static void uploadMesh(const Mesh& mesh, WGPUDevice device, WGPUQueue queue, WGPUBuffer& vertexBuffer, WGPUBuffer& indexBuffer);

	// Upload texture
	static void uploadImage(const Image& image, WGPUDevice device, WGPUQueue queue, WGPUTexture& texture, WGPUTextureView& textureView);

	static void uploadCubemap(const std::array<Image, 6>& images, WGPUDevice device, WGPUQueue queue, WGPUTexture& texture, WGPUTextureView& textureView);

	static void clean();

	static size_t getUBOAlignment(){ return _uboAlignment; }

private:

	static WGPUComputePipeline _mipmapPipeline;
	static WGPUShaderModule _mipmapModule;
	static size_t _uboAlignment;
};
