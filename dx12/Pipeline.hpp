#pragma once
#include <d3d12.h>
#include <string>

class Pipeline {

public:

	struct Settings {

		enum Resource {
			NONE = 0,
			ENGINE_TABLE = 1 << 0,
			OBJECT_TABLE = 1 << 1,
			ENGINE_CBV = 1 << 2,
			OBJECT_CBV = 1 << 3,
			ALL = ENGINE_TABLE | OBJECT_TABLE | ENGINE_CBV | OBJECT_CBV
		};

		Resource resources{ NONE };
		unsigned int resourcesCount{ 0 };
		bool pixel{ false };
		bool fullLayout{ false };
	};
	
	void configureGraphics(const std::string& name, const Settings& settings, ID3D12Device* device);

	void configureCompute(const std::string& name, ID3D12Device* device);

	void clean()
	{
		pipeline->Release();
		signature->Release();
	}

	ID3D12RootSignature* signature{ nullptr }; 
	ID3D12PipelineState* pipeline{ nullptr };

};

inline Pipeline::Settings::Resource operator|(Pipeline::Settings::Resource a, Pipeline::Settings::Resource b) {
	return static_cast<Pipeline::Settings::Resource>(std::underlying_type_t<Pipeline::Settings::Resource>(a) | std::underlying_type_t<Pipeline::Settings::Resource>(b));
}

inline bool operator&(Pipeline::Settings::Resource a, Pipeline::Settings::Resource b) {
	return static_cast<bool>(std::underlying_type_t<Pipeline::Settings::Resource>(a) & std::underlying_type_t<Pipeline::Settings::Resource>(b));
}