#include "Renderer.hpp"
#include "GPU.hpp"

#include "helpers/Resources.hpp"

#include "helpers/Log.hpp"

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")

#ifndef M_PI
#define M_PI       3.14159265358979323846   // pi
#endif

using namespace DirectX;

#define SHADOW_MAP_SIZE 1024

// Constant buffers.

struct BasicVsCB {
	XMFLOAT4X4 mvp;
};

struct LitCB {
	XMFLOAT4X4 MV;
	XMFLOAT4X4 invMV;
	XMFLOAT4X4 lightMVP;
	float shininess;
};

struct PerFrameCB {
	XMFLOAT4X4 P;
	XMFLOAT4 lightDirViewSpace;
};

#define LIT_OBJ_COUNT 3
#define UNLIT_OBJ_COUNT 1

#define CB_MIN_OFFSET_IN_CONSTANTS 16
#define CB_CONSTANT_SIZE_IN_BYTES (4 * 4)
#define CB_MIN_OFFSET_IN_BYTES (CB_MIN_OFFSET_IN_CONSTANTS * CB_CONSTANT_SIZE_IN_BYTES)

unsigned int alignOn(unsigned int size, unsigned int offset) {
	return ((size + offset - 1) / offset) * offset;
}

unsigned int alignOnCbOffset(unsigned int sizeInBytes) {
	return ((sizeInBytes + CB_MIN_OFFSET_IN_BYTES - 1) / CB_MIN_OFFSET_IN_BYTES) * CB_MIN_OFFSET_IN_BYTES;
}

void loadTexture(const std::string& path, ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DescriptorAllocator& allocator, std::vector<ID3D12Resource*>& resourcePool, ID3D12Resource*& texture, D3D12_CPU_DESCRIPTOR_HANDLE view) {
	Image image;
	Resources::loadImage(path, image, true);

	texture = GPU::createTexture(device, DXGI_FORMAT_R8G8B8A8_UNORM, image.w, image.h, 1, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	GPU::uploadImageToSubresource(device, commandList, allocator, resourcePool, texture, image, 0);

	// Generate SRV covering full texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0;
	device->CreateShaderResourceView(texture, &srvDesc, view);
}


void Renderer::Object::load(const std::string& name, ID3D12Device* device, ID3D12GraphicsCommandList * commandList, std::vector<ID3D12Resource*>& resourcePool ) {
	Mesh mesh;
	Resources::loadMesh("resources/models/" + name + ".obj", mesh);
	vbCount = (unsigned int)mesh.vertices.size();
	ibCount = (unsigned int)mesh.indices.size();

	// Create buffers
	{
		const size_t size = vbCount * sizeof( Vertex );
		char* data = (char*)mesh.vertices.data();
		vertexBuffer = GPU::createAndUploadBuffer(device, commandList, resourcePool, size, data);
		GPU::transitionResource(commandList, vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		
		vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexView.SizeInBytes = (UINT)size;
		vertexView.StrideInBytes = sizeof(Vertex);
	}

	{
		const size_t size = ibCount * sizeof( unsigned int );
		char* data = (char*)mesh.indices.data();
		indexBuffer = GPU::createAndUploadBuffer(device, commandList, resourcePool, size, data);
		GPU::transitionResource(commandList, indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		
		indexView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexView.SizeInBytes = (UINT)size;
		indexView.Format = DXGI_FORMAT_R32_UINT;
	}
}

void Renderer::ShadedObject::load(const std::string& name, ID3D12Device* device, DescriptorAllocator& allocator, ID3D12GraphicsCommandList* commandList, std::vector<ID3D12Resource*>& resourcePool ) {
	Object::load(name, device, commandList, resourcePool);
	
	resourceTable = allocator.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2);
	resourceTableGpu = allocator.convert( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, resourceTable);

	D3D12_CPU_DESCRIPTOR_HANDLE diffuse = resourceTable;
	D3D12_CPU_DESCRIPTOR_HANDLE normal = allocator.next(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, resourceTable);

	loadTexture("resources/textures/" + name + ".png", device, commandList, allocator, resourcePool, diffuseTexture, diffuse);
	loadTexture("resources/textures/" + name + "_normal.png", device, commandList, allocator, resourcePool, normalTexture, normal);
	
}

void Renderer::Skybox::load(const std::string& name, ID3D12Device* device, DescriptorAllocator& allocator, ID3D12GraphicsCommandList* commandList, std::vector<ID3D12Resource*>& resourcePool) {
	Object::load("cube", device, commandList, resourcePool );

	resourceTable = allocator.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2);
	resourceTableGpu = allocator.convert( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, resourceTable);

	D3D12_CPU_DESCRIPTOR_HANDLE view = resourceTable;

	std::vector<Image> faces(6);
	const std::string suffixes[] = { "_r", "_l", "_u", "_d", "_b", "_f"};
	for (int i = 0; i < 6; ++i) {
		Resources::loadImage("resources/textures/" + name + "/" + name + suffixes[i] + ".png", faces[i], false);
	}

	cubemapTexture = GPU::createTexture(device, DXGI_FORMAT_R8G8B8A8_UNORM, faces[0].w, faces[0].h, 6, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	
	UINT mipCount = cubemapTexture->GetDesc().MipLevels;
	for (UINT i = 0; i < 6; ++i) {
		UINT index = i * mipCount;
		GPU::uploadImageToSubresource(device, commandList, allocator, resourcePool, cubemapTexture, faces[i], index);
	}

	// Generate SRV covering full texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = mipCount;
	srvDesc.TextureCube.ResourceMinLODClamp = 0;
	device->CreateShaderResourceView(cubemapTexture, &srvDesc, view);
}

void Renderer::Camera::update() {
	float cosV = cosf(verticalAngle);
	float sinV = sinf(verticalAngle);
	float cosH = cosf(horizontalAngle);
	float sinH = sinf(horizontalAngle);
	XMVECTOR pos = XMVectorSet(radius * cosH * cosV, radius * sinV, radius * sinH * cosV, 1.f);
	XMVECTOR up = XMVectorSet(0.f, 1.0f, 0.f, 0.f);
	XMVECTOR at = XMVectorSet(0.f, 0.0f, 0.f, 1.f);
	matrix = XMMatrixLookAtRH(pos, at, up);
}

Renderer::Renderer( HWND window )
{
	_window = window;
	// Initial resolution
	RECT rect;
	if( GetWindowRect( window, &rect ) )
	{
		_w = ( std::max)( (LONG)1, std::abs( rect.right - rect.left ) );
		_h = ( std::max)( (LONG)1, std::abs( rect.bottom - rect.top ) );
	}

	_camera.horizontalAngle = 0.7f; 
	_camera.verticalAngle = 0.4f;
	_camera.radius = 2.0f; 

#ifdef _DEBUG
	ID3D12Debug3* dbgInterface{ nullptr };
	DX_RET( D3D12GetDebugInterface( IID_PPV_ARGS(&dbgInterface) ) );
	if (dbgInterface) {
		dbgInterface->EnableDebugLayer();
		//dbgInterface->SetEnableSynchronizedCommandQueueValidation(TRUE);
	}
#endif
	IDXGIFactory2* dxgiFactory{ nullptr };
	DX_RET( CreateDXGIFactory2( 0 ,  IID_PPV_ARGS( &dxgiFactory ) ));
	
	IDXGIAdapter1* dxgiAdapter = GPU::retrieveBestAdapter(dxgiFactory);
	if (!dxgiAdapter)
		return;

	// Device
	DX_RET( D3D12CreateDevice( dxgiAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS( &_device ) ) );
	
	// Command queue
	{
		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		DX_RET(_device->CreateCommandQueue( &desc, IID_PPV_ARGS( &_commandQueue ) ));
	}

	// Swapchain
	{
		DXGI_SWAP_CHAIN_DESC1 swapDesc{};
		swapDesc.Width = _w;
		swapDesc.Height = _h;
		swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapDesc.Stereo = FALSE;
		swapDesc.SampleDesc.Count = 1;
		swapDesc.SampleDesc.Quality = 0;
		swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapDesc.BufferCount = FRAME_COUNT;
		swapDesc.Scaling = DXGI_SCALING_STRETCH;
		swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		
		IDXGISwapChain1* swapchain;
		DX_RET( dxgiFactory->CreateSwapChainForHwnd( _commandQueue, _window, &swapDesc, nullptr, nullptr, &swapchain ));
		swapchain->QueryInterface< IDXGISwapChain3>( &_swapchain );
		swapchain->Release();

		_currentBackbuffer = _swapchain->GetCurrentBackBufferIndex();
	}

	// Create descriptor heaps
	_descriptorAllocator.init(_device);

	// Backbuffers
	{
		// Allocate descriptors for the backbuffers once and for all, we'll flush when resizing.
		for( int i = 0; i < FRAME_COUNT; ++i )
		{
			_backbufferRTs[ i ] = _descriptorAllocator.allocate( D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1 );
		}
		_depthBufferRT = _descriptorAllocator.allocate( D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1 );

		rebuildSwapchainDepthAndViews();
	}

	// Setup the fence to track which frames are complete on the GPU.
	{
		DX_RET( _device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &_fence ) ) );
		_nextFrame = 1;
		_fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
		if( !_fenceEvent )
			return;
	}

	// Per frame resources.
	{
		// Estimate size, taking into account constants alignment constraints.
		unsigned int cbSizeInBytes = alignOnCbOffset(sizeof(PerFrameCB));
		cbSizeInBytes += LIT_OBJ_COUNT * (alignOnCbOffset(sizeof(BasicVsCB)) + alignOnCbOffset(sizeof(LitCB)));
		cbSizeInBytes += UNLIT_OBJ_COUNT * alignOnCbOffset(sizeof(BasicVsCB));

		D3D12_HEAP_PROPERTIES heapDesc{};
		heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC rscDesc{};
		rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		rscDesc.Width = cbSizeInBytes;
		rscDesc.Height = 1;
		rscDesc.DepthOrArraySize = 1;
		rscDesc.MipLevels = 1;
		rscDesc.Format = DXGI_FORMAT_UNKNOWN;
		rscDesc.SampleDesc.Count = 1;
		rscDesc.SampleDesc.Quality = 0;
		rscDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		for( int i = 0; i < FRAME_COUNT; ++i )
		{
			DX_RET( _device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &_commandAllocators[ i ] ) ) );

			_resourcesLastFrames[ i ] = 0;

			DX_RET(_device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &rscDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_constantBuffers[i])));

			// No need for views, we'll pass an offset in the root signature
			// D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
			// desc.BufferLocation = _constantBuffers[i]->GetGPUVirtualAddress();
			// desc.SizeInBytes = ...
			// DX_RET(_device->CreateConstantBufferView(&desc, ...);

		}
	}				


	DX_RET( _device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocators[ 0 ], nullptr, IID_PPV_ARGS( &_commandList ) ) );
	// Command list is opened by default, used for upload.


	dxgiAdapter->Release();
	dxgiFactory->Release();

	// Shared resource tables
	{
		_texturesTable = _descriptorAllocator.allocate( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
		_texturesTableGpu = _descriptorAllocator.convert( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, _texturesTable);
		_samplersTable = _descriptorAllocator.allocate( D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2);
		_samplersTableGpu = _descriptorAllocator.convert( D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, _samplersTable);
	}

	// Shadowmap
	{
		_shadowMapDepth = GPU::createTexture(_device, DXGI_FORMAT_R32_TYPELESS, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 1, 1, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
		viewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		viewDesc.Flags = D3D12_DSV_FLAG_NONE;
		viewDesc.Texture2D.MipSlice = 0;

		_shadowMapDepthRT = _descriptorAllocator.allocate( D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);

		_device->CreateDepthStencilView( _shadowMapDepth, &viewDesc, _shadowMapDepthRT );

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0;

		// Directly place the shadow map in the shared texture resource table.
		D3D12_CPU_DESCRIPTOR_HANDLE shadowMap = _texturesTable;
		_device->CreateShaderResourceView( _shadowMapDepth, &srvDesc, shadowMap);
	}


	// Samplers
	{
		// Place two samplers in the shared sampler resource table.
		D3D12_CPU_DESCRIPTOR_HANDLE  linearSampler= _samplersTable;
		D3D12_CPU_DESCRIPTOR_HANDLE anisoSampler = _descriptorAllocator.next(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, linearSampler);
		
		D3D12_SAMPLER_DESC desc{};
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.MipLODBias = 0;
		desc.MaxAnisotropy = 0;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 0.F;
		desc.MinLOD = 0.f;
		desc.MaxLOD = D3D12_FLOAT32_MAX;
		_device->CreateSampler(&desc, linearSampler);

		desc.Filter = D3D12_FILTER_ANISOTROPIC;
		desc.MaxAnisotropy = 8;
		_device->CreateSampler(&desc, anisoSampler);
	}


	// Load shaders
	{
		Pipeline::Settings settings;
		settings.fullLayout = true;
		settings.pixel = true;
		settings.resources = Pipeline::Settings::ALL;
		settings.resourcesCount = 2;
		_lit.configureGraphics("resources/shaders/Lit", settings, _device);

		settings.fullLayout = false;
		settings.pixel = true;
		settings.resources = Pipeline::Settings::OBJECT_CBV | Pipeline::Settings::OBJECT_TABLE;
		settings.resourcesCount = 1;
		_unlit.configureGraphics("resources/shaders/Unlit", settings, _device);

		settings.fullLayout = false;
		settings.pixel = false;
		settings.resources = Pipeline::Settings::OBJECT_CBV;
		settings.resourcesCount = 0;
		_shadow.configureGraphics("resources/shaders/Shadow", settings, _device);

	}
	
	// Prepare for upload work on the command list.
	std::vector<ID3D12Resource*> tmpResourcesPool;
	_commandList->SetDescriptorHeaps(2, _descriptorAllocator.getGpuHeaps());

	_objects.resize( 3 );

	_objects[0].load("dragon", _device, _descriptorAllocator, _commandList, tmpResourcesPool );
	_objects[0].model = XMMatrixScaling(1.2f, 1.2f, 1.2f) * XMMatrixTranslation(-0.5f, 0.0f, -0.5f);
	_objects[0].shininess = 64.f;

	_objects[1].load("suzanne", _device, _descriptorAllocator, _commandList, tmpResourcesPool );
	_objects[1].model = XMMatrixScaling(0.65f, 0.65f, 0.65f) * XMMatrixTranslation(0.5f, 0.0f, 0.5f);
	_objects[1].shininess = 8.f;

	_objects[2].load("plane", _device, _descriptorAllocator, _commandList, tmpResourcesPool );
	_objects[2].model = XMMatrixScaling(2.75f, 2.75f, 2.75f) * XMMatrixTranslation(0.0f, -0.8f, 0.0f);
	_objects[2].shininess = 32.f;

	_skybox.load("cubemap", _device, _descriptorAllocator, _commandList, tmpResourcesPool );
	_skybox.model =  XMMatrixScaling(15.0f, 15.0f, 15.0f);

	// Light init
	_worldLightDir = XMVector3Normalize(XMVectorSet(1.0f, 0.6f, 1.0f, 0.0f));
	_lightView = XMMatrixLookAtRH(5.f * _worldLightDir, XMVectorZero(), XMVectorSet(0.f, 1.0f, 0.f, 0.f));
	_lightProj = XMMatrixOrthographicRH(10.f, 10.f, 0.1f, 10.f); // This will never change

	DX_RET(_commandList->Close());
	ID3D12CommandList* cList[] = { _commandList };
	_commandQueue->ExecuteCommandLists( 1, cList );

	flush();

	for( ID3D12Resource* tmpResource : tmpResourcesPool )
	{
		tmpResource->Release();
	}
}

Renderer::~Renderer()
{

	flush();

	// Cleanup everything.
	

	for( ShadedObject& obj : _objects )
	{
		obj.diffuseTexture->Release();
		obj.normalTexture->Release();
		obj.indexBuffer->Release();
		obj.vertexBuffer->Release();
	}

	_skybox.cubemapTexture->Release();
	_skybox.indexBuffer->Release();
	_skybox.vertexBuffer->Release();

	_lit.pipeline->Release();
	_lit.signature->Release();
	_unlit.pipeline->Release();
	_unlit.signature->Release();
	_shadow.pipeline->Release();
	_shadow.signature->Release();

	_shadowMapDepth->Release();
	_depthBuffer->Release();

	_fence->Release();

	_commandList->Release();
	for( int i = 0; i < FRAME_COUNT; ++i )
	{
		_constantBuffers[ i ]->Release();
		_commandAllocators[ i ]->Release();
		_backbuffers[ i ]->Release();
	}

	_descriptorAllocator.clear();

	_commandQueue->Release();
	_swapchain->Release();
	_device->Release();
}

void Renderer::rebuildSwapchainDepthAndViews(){

	// Swapchain views
	for(int i = 0; i < FRAME_COUNT; ++i){
		DX_RET(_swapchain->GetBuffer(i, IID_PPV_ARGS(&_backbuffers[i])));
		_device->CreateRenderTargetView( _backbuffers[i], nullptr, _backbufferRTs[i]);
		
	}

	// Depth buffer
	{
		_depthBuffer = GPU::createTexture(_device, DXGI_FORMAT_D32_FLOAT, _w, _h, 1, 1, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
		viewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		viewDesc.Flags = D3D12_DSV_FLAG_NONE;
		_device->CreateDepthStencilView( _depthBuffer, &viewDesc, _depthBufferRT );
	}

}


void Renderer::resize( unsigned int w, unsigned int h )
{
	_w = w;
	_h = h;

	flush();

	for(int i = 0; i < FRAME_COUNT; ++i){
		_backbuffers[i]->Release();
	}
	_depthBuffer->Release();

	DX_RET(_swapchain->ResizeBuffers(0, _w, _h, DXGI_FORMAT_UNKNOWN, 0));

	rebuildSwapchainDepthAndViews();
}

void Renderer::update(const MouseState& mouse, double time, double deltaTime) {

	const float scrollSpeed = 8.0f;
	const float angleSpeed = 1.f;
	const float deltaTimeF = (float)deltaTime;
	// Temporary rotation
	if (mouse.held) {
		float deltaX = (float)(mouse.currentX - mouse.clickedX);
		float deltaY = (float)(mouse.currentY - mouse.clickedY);

		_camera.horizontalAngle += deltaTimeF * deltaX * angleSpeed;
		_camera.verticalAngle += deltaTimeF * deltaY * angleSpeed;
		_camera.verticalAngle = (std::min)((std::max)(_camera.verticalAngle, -1.57f), 1.57f);
	}
	if (mouse.scroll != 0.f) {
		_camera.radius -= deltaTimeF * scrollSpeed * mouse.scroll;
		_camera.radius = (std::max)(_camera.radius, 0.1f);
	}
	_camera.update();

	_worldLightDir = XMVector3Normalize(XMVectorSet(1.0f, 0.5f*sinf((float)time)+0.6f, 1.0f,0.0f));

	_proj = XMMatrixPerspectiveFovRH( 60.f * ( float )M_PI / 180.f, float( _w ) / float( _h ), 0.1f, 1000.0f );

	_lightView = XMMatrixLookAtRH(5.f * _worldLightDir, XMVectorZero(), XMVectorSet(0.f, 1.0f, 0.f, 0.f));
	
	float monkeyAngle = float(fmod(time, 2 * M_PI));
	_objects[1].model = XMMatrixScaling(0.65f, 0.65f, 0.65f) * XMMatrixRotationAxis(XMVectorSet(0.f, 1.0f, 0.f, 0.f), monkeyAngle) * XMMatrixTranslation(0.5f, 0.0f, 0.5f); 
}

bool Renderer::draw()
{
	UINT64 lastFrameComplete = _fence->GetCompletedValue();
	
	_currentResources = ( _currentResources + 1) % FRAME_COUNT;
	// Make sure the resources we want to use are not in use anymore.
	const UINT64 resourcesLastFrame = _resourcesLastFrames[ _currentBackbuffer ];
	if( ( resourcesLastFrame > lastFrameComplete) && ( resourcesLastFrame != 0) )
	{
		// Otherwise bruteforce wait
		DX_RET(_fence->SetEventOnCompletion( resourcesLastFrame, _fenceEvent ));
		WaitForSingleObject( _fenceEvent, INFINITE );
	}

	// Update the constant buffer content.
	ID3D12Resource* constantBuffer = _constantBuffers[_currentResources];
	UINT frameCbOffset = 0;
	UINT unlitVsOffsets[UNLIT_OBJ_COUNT];
	UINT shadowVsOffsets[LIT_OBJ_COUNT];
	UINT litOffsets[LIT_OBJ_COUNT];

	{

		// Prepare all data for each shaded object
		const UINT frameCbSize = alignOnCbOffset(sizeof(PerFrameCB)) / CB_CONSTANT_SIZE_IN_BYTES;
		const UINT shadowVsSize = alignOnCbOffset(sizeof(BasicVsCB)) / CB_CONSTANT_SIZE_IN_BYTES;
		const UINT unlitVsSize = alignOnCbOffset(sizeof(BasicVsCB)) / CB_CONSTANT_SIZE_IN_BYTES;
		const UINT litSize = alignOnCbOffset(sizeof(LitCB)) / CB_CONSTANT_SIZE_IN_BYTES;

		BYTE* data{ nullptr };
		DX_RET(constantBuffer->Map(0, nullptr, (void**)&data));

		UINT currentOffsetInConstants = 0;
		int i = 0;

		// Per frame data.
		XMMATRIX lightVP = _lightView * _lightProj;
		PerFrameCB frame;
		XMStoreFloat4x4(&frame.P, XMMatrixTranspose(_proj));
		XMVECTOR lightDirViewSpace = XMVector4Transform(_worldLightDir, _camera.matrix);
		XMStoreFloat4(&frame.lightDirViewSpace, lightDirViewSpace);
		std::memcpy(data, &frame, sizeof(frame));
		currentOffsetInConstants += frameCbSize;

		// Shaded objects
		for (const ShadedObject& object : _objects) {
	
			BasicVsCB shadow;
			XMMATRIX MVP = object.model * lightVP;
			XMStoreFloat4x4(&shadow.mvp, XMMatrixTranspose(MVP));
		
			shadowVsOffsets[i] = currentOffsetInConstants * CB_CONSTANT_SIZE_IN_BYTES;
			std::memcpy(data + CB_CONSTANT_SIZE_IN_BYTES * currentOffsetInConstants, &shadow, sizeof(shadow));
			currentOffsetInConstants += shadowVsSize;
			
			LitCB lit;
			lit.shininess = object.shininess;
			XMMATRIX MV = object.model * _camera.matrix;
			XMMATRIX invMV = XMMatrixInverse(nullptr, MV);
			XMMATRIX lightMVP = object.model * lightVP;
			XMStoreFloat4x4(&lit.lightMVP, XMMatrixTranspose(lightMVP));
			XMStoreFloat4x4(&lit.MV, XMMatrixTranspose(MV));
			XMStoreFloat4x4(&lit.invMV, invMV); // Double transpose
		
			litOffsets[i] = currentOffsetInConstants * CB_CONSTANT_SIZE_IN_BYTES;
			std::memcpy(data + CB_CONSTANT_SIZE_IN_BYTES * currentOffsetInConstants, &lit, sizeof(lit));
			currentOffsetInConstants += litSize;
		
			++i;
		}
		{
			BasicVsCB unlit;
			XMMATRIX VNoTranslation = _camera.matrix;
			VNoTranslation.r[ 3 ] = XMVectorSet( 0.f, 0.0f, 0.f, 1.f );
			XMMATRIX MVP = _skybox.model * VNoTranslation * _proj;
			XMStoreFloat4x4(&unlit.mvp, XMMatrixTranspose(MVP));
		
			unlitVsOffsets[0] = currentOffsetInConstants * CB_CONSTANT_SIZE_IN_BYTES;
			std::memcpy(data + CB_CONSTANT_SIZE_IN_BYTES * currentOffsetInConstants, &unlit, sizeof(unlit));
			currentOffsetInConstants += unlitVsSize;
		}
		constantBuffer->Unmap(0, nullptr);
		
	}

	const D3D12_VIEWPORT mainViewport{ 0.f, 0.f, (float)_w, (float)_h, 0.f, 1.f };
	const D3D12_VIEWPORT shadowViewport{ 0.f, 0.f, (float)SHADOW_MAP_SIZE, (float)SHADOW_MAP_SIZE, 0.f, 1.f };
	const D3D12_RECT mainScissor{ 0, 0, (LONG)_w, (LONG)_h };
	const D3D12_RECT shadowScissor{ 0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE };
	
	// We can reuse this allocator.
	DX_RET( _commandAllocators[ _currentResources ]->Reset());
	// Open command list
	DX_RET( _commandList->Reset( _commandAllocators[ _currentResources ], nullptr ) );
	_commandList->SetDescriptorHeaps(2, _descriptorAllocator.getGpuHeaps());

	// Shadow map pass (assume in depth write state by default).
	{
		_commandList->ClearDepthStencilView(_shadowMapDepthRT, D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
		_commandList->RSSetViewports(1, &shadowViewport);
		_commandList->RSSetScissorRects(1, &shadowScissor);
		_commandList->OMSetRenderTargets(0, nullptr, FALSE, &_shadowMapDepthRT);
		
		_commandList->SetGraphicsRootSignature(_shadow.signature);
		_commandList->SetPipelineState(_shadow.pipeline);

		_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		int i = 0;
		for(const ShadedObject& obj : _objects) {
			// Root signature:  object CB
			_commandList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress() + shadowVsOffsets[i]);
			_commandList->IASetVertexBuffers(0, 1, &obj.vertexView);
			_commandList->IASetIndexBuffer(&obj.indexView);
			_commandList->DrawIndexedInstanced(obj.ibCount, 1, 0, 0, 0);
			++i;
		}
	}


	// Fetch the current backbuffer and transition it.
	_currentBackbuffer = _swapchain->GetCurrentBackBufferIndex();
	GPU::transitionResource(_commandList, _backbuffers[ _currentBackbuffer ], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	// We'll use the shadow map as resource in pixel shader
	GPU::transitionResource(_commandList, _shadowMapDepth, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Main pass
	{
		// Clear rendertargets
		const float clearColor[] = { 1.f, 0.5f, 0.0f, 1.0f };
		_commandList->ClearRenderTargetView( _backbufferRTs[ _currentBackbuffer ], clearColor, 0, nullptr );
		_commandList->ClearDepthStencilView(_depthBufferRT, D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
		_commandList->RSSetViewports(1, &mainViewport);
		_commandList->RSSetScissorRects(1, &mainScissor);
		_commandList->OMSetRenderTargets(1, &_backbufferRTs[ _currentBackbuffer ], FALSE, &_depthBufferRT);

		_commandList->SetGraphicsRootSignature(_lit.signature);
		_commandList->SetPipelineState(_lit.pipeline);
		_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Root signature: object CB, object RT, engine CB, engine RT, samplers RT
		_commandList->SetGraphicsRootConstantBufferView(2, constantBuffer->GetGPUVirtualAddress() + frameCbOffset);
		_commandList->SetGraphicsRootDescriptorTable(3, _texturesTableGpu);
		_commandList->SetGraphicsRootDescriptorTable(4, _samplersTableGpu);

		int i = 0;
		for(const ShadedObject& obj : _objects) {
			_commandList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress() + litOffsets[i]);
			_commandList->SetGraphicsRootDescriptorTable(1, obj.resourceTableGpu);
			_commandList->IASetVertexBuffers(0, 1, &obj.vertexView);
			_commandList->IASetIndexBuffer(&obj.indexView);
			_commandList->DrawIndexedInstanced(obj.ibCount, 1, 0, 0, 0);
			++i;
		}

		_commandList->SetGraphicsRootSignature(_unlit.signature);
		_commandList->SetPipelineState(_unlit.pipeline);
		_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// Root signature:  object CB, object RT, samplersRT
		_commandList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress() + unlitVsOffsets[0]);
		_commandList->SetGraphicsRootDescriptorTable(1, _skybox.resourceTableGpu);
		_commandList->SetGraphicsRootDescriptorTable(2, _samplersTableGpu);
		_commandList->IASetVertexBuffers(0, 1, &_skybox.vertexView);
		_commandList->IASetIndexBuffer(&_skybox.indexView);
		_commandList->DrawIndexedInstanced(_skybox.ibCount, 1, 0, 0, 0);
	}

	// Restore backbuffer and shadowmap to their default states. 
	GPU::transitionResource(_commandList, _backbuffers[ _currentBackbuffer ], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	GPU::transitionResource(_commandList, _shadowMapDepth, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	DX_RET( _commandList->Close() );

	ID3D12CommandList* commandLists[] = { _commandList };
	_commandQueue->ExecuteCommandLists( 1, commandLists );

	// Present current backbuffer.
	HRESULT status = _swapchain->Present( 1, 0 );

	// Increment the frame index on the GPU, so that we can check when the frame is finished.
	_resourcesLastFrames[ _currentResources ] = _nextFrame;
	DX_RET(_commandQueue->Signal( _fence, _nextFrame ));
	++_nextFrame;

	// Quit if occluded, to fix application sometimes remaining active after quitting
	return status != S_OK;
}

void Renderer::flush(){
	
	// Add a new 'frame' on the gpu and wait on it.
	UINT64 frameToFinish = _nextFrame;
	UINT64 lastFrameComplete = _fence->GetCompletedValue();
	_commandQueue->Signal(_fence, frameToFinish );
	++_nextFrame;

	if(lastFrameComplete < frameToFinish ){
		_fence->SetEventOnCompletion( frameToFinish, _fenceEvent);
		WaitForSingleObject(_fenceEvent, INFINITE);
	}
}
