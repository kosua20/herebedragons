#pragma once

#include <vector>
#include <string>
#include <DirectXMath.h>

#include "GPU.hpp"
#include "DescriptorAllocator.hpp"
#include "Pipeline.hpp"

#define CB_COUNT 3
#define FRAME_COUNT 3

struct MouseState {
	float scroll = 0.f;
	int currentX = 0;
	int currentY = 0;
	int clickedX = 0;
	int clickedY = 0;
	bool pressed = false;
	bool held = false;
};

class Renderer
{
public:

	Renderer( HWND windows );

	~Renderer();

	void resize( unsigned int w, unsigned int h );

	void update(const MouseState& mouse, double time, double deltaTime);
	
	bool draw();

private:

	void rebuildSwapchainDepthAndViews();

	void flush();

	struct Object {

		ID3D12Resource* vertexBuffer{nullptr};
		ID3D12Resource* indexBuffer{nullptr};
		D3D12_VERTEX_BUFFER_VIEW vertexView;
		D3D12_INDEX_BUFFER_VIEW indexView;

		unsigned int vbCount{ 0 };
		unsigned int ibCount{ 0 };
		DirectX::XMMATRIX model;

		void load(const std::string& name, ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::vector<ID3D12Resource*>& resourcePool );
	};

	struct ShadedObject : public Object {

		ID3D12Resource* diffuseTexture{ nullptr };
		ID3D12Resource* normalTexture{nullptr};
		D3D12_CPU_DESCRIPTOR_HANDLE resourceTable{ 0 };
		D3D12_GPU_DESCRIPTOR_HANDLE resourceTableGpu{ 0 };
		
		float shininess;

		void load(const std::string& name, ID3D12Device* device, DescriptorAllocator& allocator, ID3D12GraphicsCommandList* commandList, std::vector<ID3D12Resource*>& resourcePool );
	};


	struct Skybox : public Object {

		ID3D12Resource* cubemapTexture{ nullptr };
		D3D12_CPU_DESCRIPTOR_HANDLE resourceTable{ 0 };
		D3D12_GPU_DESCRIPTOR_HANDLE resourceTableGpu{ 0 };

		void load(const std::string& name, ID3D12Device* device, DescriptorAllocator& allocator, ID3D12GraphicsCommandList* commandList, std::vector<ID3D12Resource*>& resourcePool );
	};

	// Device and context
	ID3D12Device* _device{nullptr};
	ID3D12CommandQueue* _commandQueue{ nullptr };
	DescriptorAllocator _descriptorAllocator;

	// Backbuffer and views
	IDXGISwapChain3* _swapchain{ nullptr };
	ID3D12Resource* _backbuffers[FRAME_COUNT];
	D3D12_CPU_DESCRIPTOR_HANDLE _backbufferRTs[ FRAME_COUNT ];
	UINT _currentBackbuffer{ 0 };

	// Per frame resources
	ID3D12CommandAllocator* _commandAllocators[ FRAME_COUNT ];
	UINT64 _resourcesLastFrames[ FRAME_COUNT ];
	UINT _currentResources{ 0 };

	ID3D12GraphicsCommandList* _commandList{ nullptr };
	ID3D12Resource* _depthBuffer{ nullptr };
	D3D12_CPU_DESCRIPTOR_HANDLE _depthBufferRT{ 0 };

	// Synchronization
	ID3D12Fence* _fence{nullptr};
	UINT64 _nextFrame{0};
	HANDLE _fenceEvent{0};

	//Shadow map resource and views
	ID3D12Resource* _shadowMapDepth{nullptr};
	D3D12_CPU_DESCRIPTOR_HANDLE _shadowMapDepthRT{0};
	ID3D12Resource* _shadowMap{nullptr};
	D3D12_CPU_DESCRIPTOR_HANDLE _shadowMapRT{0};
	ID3D12Resource* _shadowMapBlur{nullptr};

	//// Global constant buffer
	ID3D12Resource* _constantBuffers[FRAME_COUNT];
	
	//// Shaders and layout for lit/unlit/shadow
	Pipeline _lit;
	Pipeline _unlit;
	Pipeline _shadow;
	Pipeline _blur;

	D3D12_CPU_DESCRIPTOR_HANDLE _texturesTables;
	D3D12_GPU_DESCRIPTOR_HANDLE _texturesDrawTableGpu;
	D3D12_GPU_DESCRIPTOR_HANDLE _texturesBlurTableGpu;
	
	D3D12_CPU_DESCRIPTOR_HANDLE _samplersTable;
	D3D12_GPU_DESCRIPTOR_HANDLE _samplersTableGpu;

	std::vector<ShadedObject> _objects;
	Skybox _skybox;

	struct Camera {
		DirectX::XMMATRIX matrix;
		float radius;
		float horizontalAngle;
		float verticalAngle;

		void update();
	};
	Camera _camera;
	
	DirectX::XMMATRIX _proj;
	DirectX::XMMATRIX _lightView;
	DirectX::XMMATRIX _lightProj;
	DirectX::XMVECTOR _worldLightDir;

	unsigned int _w{ 800 };
	unsigned int _h{ 600 };
	HWND _window{ 0 };
};