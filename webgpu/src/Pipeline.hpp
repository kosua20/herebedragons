#pragma once

#include "common.hpp"


class Pipeline {

public:
	struct Settings {
		enum class Attribute {
			NONE = 0,
			POSITION =  1 << 0,
			NORMAL = 1 << 1,
			TANGENT = 1 << 2,
			BITANGENT = 1 << 3,
			UV = 1 << 4,
			ALL = POSITION|NORMAL|TANGENT|BITANGENT|UV
		};

		std::vector<WGPUTextureViewDimension> textures;
		Settings::Attribute attributes = Attribute::NONE;
		WGPUCullMode cullMode = WGPUCullMode_None;
		std::string module;
		std::string vertexEntry;
		std::string fragmentEntry;
		WGPUTextureFormat colorFormat = WGPUTextureFormat_Undefined;
		WGPUTextureFormat depthFormat = WGPUTextureFormat_Undefined;
		WGPUBindGroupLayout uniformLayout = nullptr;
		WGPUBindGroupLayout staticLayout = nullptr;
		bool depthBias = false;
	};


	void upload(const Settings& settings, WGPUDevice device);

	void clean();

	WGPURenderPipeline pipeline(){ return _pipeline;}

private:

	WGPUShaderModule _shaderModule = nullptr;
	WGPUPipelineLayout _pipelineLayout = nullptr;
	WGPURenderPipeline _pipeline = nullptr;
	WGPUBindGroupLayout _textureGroupLayout = nullptr;
	
};

inline Pipeline::Settings::Attribute operator|(Pipeline::Settings::Attribute a, Pipeline::Settings::Attribute b){
	return static_cast<Pipeline::Settings::Attribute>(std::underlying_type_t<Pipeline::Settings::Attribute>(a) | std::underlying_type_t<Pipeline::Settings::Attribute>(b));
}

inline bool operator&(Pipeline::Settings::Attribute a, Pipeline::Settings::Attribute b){
	return static_cast<bool>(std::underlying_type_t<Pipeline::Settings::Attribute>(a) & std::underlying_type_t<Pipeline::Settings::Attribute>(b));
}
