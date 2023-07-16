#include "GPU.hpp"
#include "resources/MeshUtilities.hpp"
#include "resources/Resources.hpp"


WGPUComputePipeline GPU::_mipmapPipeline = nullptr;
WGPUShaderModule GPU::_mipmapModule = nullptr;
size_t GPU::_uboAlignment = 0;

void GPU::init(WGPUDevice device, size_t uboAlignment){
	_uboAlignment = uboAlignment;
	
	// Prepare compute pipeline.
	const std::string shaderContent = Resources::loadStringFromExternalFile("resources/shaders/mipmap.wgsl");

	// Create module.
	WGPUShaderModuleWGSLDescriptor shaderDesc{};
	shaderDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
	shaderDesc.chain.next = nullptr;
	shaderDesc.code = shaderContent.c_str();
	WGPUShaderModuleDescriptor moduleDesc{};
	moduleDesc.nextInChain = &shaderDesc.chain;
	_mipmapModule = wgpuDeviceCreateShaderModule(device, &moduleDesc);

	WGPUComputePipelineDescriptor pipeDesc{};
	pipeDesc.nextInChain = nullptr;
	pipeDesc.layout = nullptr;
	pipeDesc.compute.module = _mipmapModule;
	pipeDesc.compute.entryPoint = "mainCompute";
	pipeDesc.compute.nextInChain = nullptr;
	pipeDesc.compute.constantCount = 0;
	pipeDesc.compute.constants = nullptr;

	_mipmapPipeline = wgpuDeviceCreateComputePipeline(device, &pipeDesc);
}

void GPU::clean(){
	if(_mipmapPipeline){
		wgpuComputePipelineRelease(_mipmapPipeline);
	}
	if(_mipmapModule){
		wgpuShaderModuleRelease(_mipmapModule);
	}
}

void GPU::initBindGroupEntry(WGPUBindGroupEntry& entry){
	entry = {};
	entry.nextInChain = nullptr;
	entry.size = 0;
	entry.offset = 0;
	entry.buffer = nullptr;
	entry.textureView = nullptr;
	entry.sampler = nullptr;
	entry.binding = 0;
}

void GPU::initBindGroupLayoutEntry(WGPUBindGroupLayoutEntry& entry){
	entry = {};
	entry.nextInChain = nullptr;
	entry.buffer.nextInChain = nullptr;
	entry.texture.nextInChain = nullptr;
	entry.sampler.nextInChain = nullptr;
	entry.storageTexture.nextInChain = nullptr;
	entry.buffer.type = WGPUBufferBindingType_Undefined;
	entry.buffer.hasDynamicOffset = false;
	entry.texture.viewDimension = WGPUTextureViewDimension_Undefined;
	entry.texture.sampleType = WGPUTextureSampleType_Undefined;
	entry.texture.multisampled = false;
	entry.sampler.type = WGPUSamplerBindingType_Undefined;
	entry.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;
	entry.storageTexture.access = WGPUStorageTextureAccess_Undefined;
	entry.storageTexture.format = WGPUTextureFormat_Undefined;
}


WGPUBuffer GPU::createBuffer(WGPUDevice device, size_t size, WGPUBufferUsageFlags usage){
	WGPUBufferDescriptor buffDesc{};
	buffDesc.nextInChain = nullptr;
	buffDesc.mappedAtCreation = false;
	buffDesc.size = size;
	buffDesc.usage = usage;
	return wgpuDeviceCreateBuffer(device, &buffDesc);
}

WGPUTexture GPU::createTexture(WGPUDevice device, size_t w, size_t h, size_t m, WGPUTextureViewDimension dims, WGPUTextureFormat format, WGPUTextureUsageFlags usage, WGPUTextureView& view){

	const uint32_t l = dims == WGPUTextureViewDimension_Cube ? 6 : 1;
	const bool isDepth = format == WGPUTextureFormat_Depth24Plus || format == WGPUTextureFormat_Depth32Float
	|| format == WGPUTextureFormat_Depth24PlusStencil8 || format == WGPUTextureFormat_Depth32FloatStencil8 || format == WGPUTextureFormat_Depth16Unorm;

	WGPUTextureDescriptor desc{};
	desc.nextInChain = nullptr;
	desc.dimension = WGPUTextureDimension_2D;
	desc.format = format;
	desc.usage = usage;
	desc.size = {uint32_t(w), uint32_t(h), l};
	desc.mipLevelCount = m;
	desc.sampleCount = 1;
	desc.viewFormatCount = 1;
	desc.viewFormats = &format; // Used for optim?
	WGPUTexture texture = wgpuDeviceCreateTexture(device, &desc);

	WGPUTextureViewDescriptor viewDesc{};
	viewDesc.nextInChain = nullptr;
	viewDesc.dimension = dims;
	viewDesc.format = format;
	viewDesc.aspect = isDepth ? WGPUTextureAspect_DepthOnly : WGPUTextureAspect_All;
	viewDesc.baseArrayLayer = 0;
	viewDesc.arrayLayerCount = l;
	viewDesc.baseMipLevel = 0;
	viewDesc.mipLevelCount = m;
	view = wgpuTextureCreateView(texture, &viewDesc);
	return texture;
}

void GPU::uploadMesh(const Mesh& mesh, WGPUDevice device, WGPUQueue queue, WGPUBuffer& vertexBuffer, WGPUBuffer& indexBuffer){

	const size_t vSize = sizeof(Vertex) * mesh.vertices.size();
	const size_t iSize = sizeof(uint32_t) * mesh.indices.size();
	vertexBuffer = createBuffer(device, vSize, WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst);
	indexBuffer = createBuffer(device, iSize, WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst);

	if(!indexBuffer || !vertexBuffer){
		std::cerr << "Unable to create geometry buffers." << std::endl;
		return;
	}

	// Upload content to both.
	wgpuQueueWriteBuffer(queue, vertexBuffer, 0, mesh.vertices.data(), vSize);
	wgpuQueueWriteBuffer(queue, indexBuffer, 0, mesh.indices.data(), iSize);
}

void GPU::uploadImage(const Image& image, WGPUDevice device, WGPUQueue queue, WGPUTexture& texture, WGPUTextureView& textureView){

	// Compute the number of mipmaps (skip very small levels).
	const unsigned int mipCount = std::max((unsigned int)std::floor(std::log2(std::min(image.w, image.h))) - 2, 1u);

	texture = createTexture(device, image.w, image.h, mipCount, WGPUTextureViewDimension_2D, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst | WGPUTextureUsage_StorageBinding, textureView);

	if(!texture){
		std::cerr << "Unable to create texture." << std::endl;
		return;
	}

	// Populate first level.
	WGPUImageCopyTexture copyDesc{};
	copyDesc.nextInChain = nullptr;
	copyDesc.texture = texture;
	copyDesc.origin = {0,0,0};
	copyDesc.aspect = WGPUTextureAspect_All;
	copyDesc.mipLevel = 0;

	WGPUTextureDataLayout dataLayout{};
	dataLayout.nextInChain = nullptr;
	dataLayout.offset = 0;
	dataLayout.bytesPerRow = image.w * image.c;
	dataLayout.rowsPerImage = image.h;

	WGPUExtent3D writeSize{ image.w, image.h, 1 };

	const size_t byteSize = image.data.size();
	wgpuQueueWriteTexture(queue, &copyDesc, image.data.data(), byteSize, &dataLayout, &writeSize);

	// Generate other levels

	// Prepare per level views
	std::vector<WGPUTextureView> views(mipCount);
	WGPUTextureViewDescriptor viewDesc{};
	viewDesc.nextInChain = nullptr;
	viewDesc.format = WGPUTextureFormat_RGBA8Unorm;
	viewDesc.dimension = WGPUTextureViewDimension_2D;
	viewDesc.aspect = WGPUTextureAspect_All;
	viewDesc.baseArrayLayer = 0;
	viewDesc.arrayLayerCount = 1;
	viewDesc.mipLevelCount = 1; // Only one mip.
	for(unsigned int i = 0; i < mipCount; ++i){
		viewDesc.baseMipLevel = i;
		views[i] = wgpuTextureCreateView(texture, &viewDesc);
	}

	// Begin pass.
	WGPUCommandEncoderDescriptor desc{};
	desc.nextInChain = nullptr;
	desc.label = "Compute mipmap";
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &desc);

	WGPUComputePassDescriptor compDesc{};
	compDesc.nextInChain = nullptr;
	compDesc.timestampWriteCount = 0;
	compDesc.timestampWrites = nullptr;
	WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(encoder, &compDesc);

	// Set mipmap pipeline.
	const unsigned int groupSize = 8u;
	wgpuComputePassEncoderSetPipeline(pass, _mipmapPipeline);

	// Preinit bind group entries and descriptor.
	WGPUBindGroupEntry entries[2];
	for(unsigned int i = 0; i < 2; ++i)
		GPU::initBindGroupEntry(entries[i]);
	entries[0].binding = 0;
	entries[1].binding = 1;
	WGPUBindGroupDescriptor groupDesc{};
	groupDesc.layout = wgpuComputePipelineGetBindGroupLayout(_mipmapPipeline, 0);
	groupDesc.nextInChain = nullptr;
	groupDesc.entryCount = 2;
	groupDesc.entries = &entries[0];

	for(unsigned int i = 1; i < mipCount; ++i){
		// Create binding group with input and output textures.
		entries[0].textureView = views[i-1];
		entries[1].textureView = views[i];
		WGPUBindGroup group = wgpuDeviceCreateBindGroup(device, &groupDesc);
		wgpuComputePassEncoderSetBindGroup(pass, 0, group, 0, nullptr);
		// Size of the dispatch.
		const unsigned int w = image.w >> i;
		const unsigned int h = image.h >> i;
		const unsigned int groupSizeX = (w + groupSize - 1)/groupSize;
		const unsigned int groupSizeY = (h + groupSize - 1)/groupSize;
		wgpuComputePassEncoderDispatchWorkgroups(pass, groupSizeX, groupSizeY, 1);
		wgpuBindGroupRelease(group);
	}
	// End pass and submit
	wgpuComputePassEncoderEnd(pass);
	WGPUCommandBufferDescriptor commandBufferDesc{};
	commandBufferDesc.nextInChain = nullptr;
	commandBufferDesc.label = "Mipmap command buffer";
	WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);
	wgpuQueueSubmit(queue, 1, &commandBuffer);

	// Cleanup encoders and views.
	wgpuComputePassEncoderRelease(pass);
	wgpuCommandEncoderRelease(encoder);
	wgpuCommandBufferRelease(commandBuffer);
	for(WGPUTextureView view : views){
		wgpuTextureViewRelease(view);
	}
}

void GPU::uploadCubemap(const std::array<Image, 6>& images, WGPUDevice device, WGPUQueue queue, WGPUTexture& texture, WGPUTextureView& textureView){

	const unsigned int w = images[0].w;
	const unsigned int h = images[0].h;
	const unsigned int c = images[0].c;

	texture = createTexture(device, w, h, 1, WGPUTextureViewDimension_Cube, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst, textureView);
	if(!texture){
		std::cerr << "Unable to create cubemap." << std::endl;
		return;
	}

	// Upload each face to a layer.

	// Common settings
	WGPUTextureDataLayout dataLayout{};
	dataLayout.nextInChain = nullptr;
	dataLayout.offset = 0;
	dataLayout.bytesPerRow = w * c;
	dataLayout.rowsPerImage = h;
	WGPUExtent3D writeSize{ w, h, 1 };
	const size_t byteSize = images[0].data.size();

	for(unsigned int i = 0; i < 6; ++i){
		WGPUImageCopyTexture copyDesc{};
		copyDesc.nextInChain = nullptr;
		copyDesc.texture = texture;
		copyDesc.origin = {0,0,i};
		copyDesc.aspect = WGPUTextureAspect_All;
		copyDesc.mipLevel = 0;
		wgpuQueueWriteTexture(queue, &copyDesc, images[i].data.data(), byteSize, &dataLayout, &writeSize);
	}


}
