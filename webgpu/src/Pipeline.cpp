#include "Pipeline.hpp"
#include "GPU.hpp"
#include "resources/Resources.hpp"
#include "resources/MeshUtilities.hpp"

void shaderCompilationCallback(WGPUCompilationInfoRequestStatus status, struct WGPUCompilationInfo const * compilationInfo, void * userdata)
{
	if(status == WGPUCompilationInfoRequestStatus_Success)
		return;

	std::cout << "Error (" << status << ") while compiling shader module \"" << ((const char*)userdata) << "\": " << std::endl;

	static const char* errorType[] = {"Error", "Warning", "Info" };

	for(unsigned int i = 0; i < compilationInfo->messageCount; ++i){
		const WGPUCompilationMessage& message = compilationInfo->messages[i];
		const std::string messageStr(message.message, message.length);
		std::cout << "* " << errorType[message.type] << " at (" << message.lineNum << "," << message.linePos << "): " << messageStr << std::endl;
	}
	if(compilationInfo->messageCount == 0){
		std::cout << "* Unknown error." << std::endl;
	}
}


void Pipeline::upload(const Settings& settings, WGPUDevice device){

	const std::string shaderContent = Resources::loadStringFromExternalFile("resources/shaders/" + settings.module + ".wgsl");

	// Create module.
	WGPUShaderModuleWGSLDescriptor shaderDesc{};
	shaderDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
	shaderDesc.chain.next = nullptr;
	shaderDesc.code = shaderContent.c_str();
	WGPUShaderModuleDescriptor moduleDesc{};
	moduleDesc.nextInChain = &shaderDesc.chain;
	_shaderModule = wgpuDeviceCreateShaderModule(device, &moduleDesc);
	wgpuShaderModuleGetCompilationInfo(_shaderModule, shaderCompilationCallback, (void*)settings.module.c_str());

	// Create pipeline layout.
	{
		const bool hasTextures = !settings.textures.empty();
		if(hasTextures){
			// Textures (assumed to be 2D or cube)
			const unsigned int textureCount = settings.textures.size();
			std::vector<WGPUBindGroupLayoutEntry> entries;
			entries.resize(textureCount);
			for(unsigned int i = 0; i < textureCount; ++i){
				GPU::initBindGroupLayoutEntry(entries[i]);
				entries[i].binding = i;
				entries[i].visibility = WGPUShaderStage_Fragment;
				entries[i].texture.sampleType = WGPUTextureSampleType_Float;
				entries[i].texture.viewDimension = settings.textures[i];
			}
			WGPUBindGroupLayoutDescriptor groupLayoutDesc{};
			groupLayoutDesc.nextInChain = nullptr;
			groupLayoutDesc.entryCount = textureCount;
			groupLayoutDesc.entries = &entries[0];
			_textureGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &groupLayoutDesc);
		}

		// Three groups at most: uniforms, textures, samplers.
		// Uniforms are always there, texture count is variable and samplers are there iff at least one texture needed.
		std::vector<WGPUBindGroupLayout> groupLayouts;
		groupLayouts.push_back( settings.uniformLayout);
		if(hasTextures){
			groupLayouts.push_back(_textureGroupLayout);
			groupLayouts.push_back(settings.staticLayout);
		}

		WGPUPipelineLayoutDescriptor layoutDesc{};
		layoutDesc.nextInChain = nullptr;
		layoutDesc.bindGroupLayoutCount = groupLayouts.size();
		layoutDesc.bindGroupLayouts = groupLayouts.data();
		_pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);
	}

	// Create pipeline.
	{
		WGPURenderPipelineDescriptor pipelineDesc{};
		pipelineDesc.nextInChain = nullptr;
		pipelineDesc.layout = _pipelineLayout;

		// Primitive settings
		pipelineDesc.primitive.nextInChain = nullptr;
		pipelineDesc.primitive.cullMode = settings.cullMode;
		pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
		pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
		pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;

		// Attributes
		std::vector<WGPUVertexAttribute> attributes;
		if(settings.attributes & Settings::Attribute::POSITION){
			WGPUVertexAttribute& attr = attributes.emplace_back();
			attr.format = WGPUVertexFormat_Float32x3;
			attr.offset = offsetof(Vertex, pos);
			attr.shaderLocation = 0;
		}
		if(settings.attributes & Settings::Attribute::NORMAL){
			WGPUVertexAttribute& attr = attributes.emplace_back();
			attr.format = WGPUVertexFormat_Float32x3;
			attr.offset = offsetof(Vertex, normal);
			attr.shaderLocation = 1;
		}
		if(settings.attributes & Settings::Attribute::TANGENT){
			WGPUVertexAttribute& attr = attributes.emplace_back();
			attr.format = WGPUVertexFormat_Float32x3;
			attr.offset = offsetof(Vertex, tangent);
			attr.shaderLocation = 2;
		}
		if(settings.attributes & Settings::Attribute::BITANGENT){
			WGPUVertexAttribute& attr = attributes.emplace_back();
			attr.format = WGPUVertexFormat_Float32x3;
			attr.offset = offsetof(Vertex, binormal);
			attr.shaderLocation = 3;
		}
		if(settings.attributes & Settings::Attribute::UV){
			WGPUVertexAttribute& attr = attributes.emplace_back();
			attr.format = WGPUVertexFormat_Float32x2;
			attr.offset = offsetof(Vertex, texCoord);
			attr.shaderLocation = 4;
		}

		WGPUVertexBufferLayout vertBuffDesc{};
		vertBuffDesc.arrayStride = sizeof(Vertex);
		vertBuffDesc.attributeCount = attributes.size();
		vertBuffDesc.attributes = &attributes[0];
		vertBuffDesc.stepMode = WGPUVertexStepMode_Vertex;

		// Vertex stage
		pipelineDesc.vertex.nextInChain = nullptr;
		pipelineDesc.vertex.module = _shaderModule;
		pipelineDesc.vertex.entryPoint = settings.vertexEntry.c_str();
		pipelineDesc.vertex.bufferCount = 1;
		pipelineDesc.vertex.buffers = &vertBuffDesc;
		pipelineDesc.vertex.constants = nullptr;
		pipelineDesc.vertex.constantCount = 0;

		// Output color
		WGPUColorTargetState targetDesc{};
		targetDesc.nextInChain = nullptr;
		targetDesc.writeMask = WGPUColorWriteMask_All;
		targetDesc.blend = nullptr; // no blend
		targetDesc.format = settings.colorFormat;

		// Fragment stage.
		WGPUFragmentState fragDesc{};
		if(!settings.fragmentEntry.empty()){
			fragDesc.nextInChain = nullptr;
			fragDesc.module = _shaderModule;
			fragDesc.entryPoint = settings.fragmentEntry.c_str();
			fragDesc.constantCount = 0;
			fragDesc.constants = nullptr;
			fragDesc.targetCount = 1;
			fragDesc.targets = &targetDesc;
			pipelineDesc.fragment = &fragDesc;
		} else {
			pipelineDesc.fragment = nullptr;
		}

		// Depth (if needed)
		WGPUDepthStencilState depthStencil{}; // Keep alive until pipeline creation.
		if(settings.depthFormat != WGPUTextureFormat_Undefined){
			depthStencil.nextInChain = nullptr;
			depthStencil.format = settings.depthFormat;
			depthStencil.depthCompare = WGPUCompareFunction_LessEqual;
			depthStencil.depthWriteEnabled = true;
			if(settings.depthBias){
				depthStencil.depthBias = 1;
				depthStencil.depthBiasClamp = 0;
				depthStencil.depthBiasSlopeScale = 1.2f;
			} else {
				depthStencil.depthBias = 0;
				depthStencil.depthBiasClamp = 0;
				depthStencil.depthBiasSlopeScale = 0;
			}
			depthStencil.stencilReadMask = 0;
			depthStencil.stencilWriteMask = 0;
			depthStencil.stencilFront.compare = WGPUCompareFunction_Always;
			depthStencil.stencilFront.passOp = WGPUStencilOperation_Keep;
			depthStencil.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
			depthStencil.stencilFront.failOp = WGPUStencilOperation_Keep;
			depthStencil.stencilBack.compare = WGPUCompareFunction_Always;
			depthStencil.stencilBack.passOp = WGPUStencilOperation_Keep;
			depthStencil.stencilBack.depthFailOp = WGPUStencilOperation_Keep;
			depthStencil.stencilBack.failOp = WGPUStencilOperation_Keep;
			pipelineDesc.depthStencil = &depthStencil;
		} else {
			pipelineDesc.depthStencil = nullptr;
		}

		// Multisampling.
		pipelineDesc.multisample.nextInChain = nullptr;
		pipelineDesc.multisample.count = 1;
		pipelineDesc.multisample.mask = ~0x0;
		pipelineDesc.multisample.alphaToCoverageEnabled = false;

		_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
	}

}

void Pipeline::clean(){
	wgpuRenderPipelineRelease(_pipeline);
	wgpuPipelineLayoutRelease(_pipelineLayout);
	if(_textureGroupLayout)
		wgpuBindGroupLayoutRelease(_textureGroupLayout);
	wgpuShaderModuleRelease(_shaderModule);
}
