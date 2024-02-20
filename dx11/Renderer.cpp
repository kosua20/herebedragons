#include "Renderer.hpp"
#include "helpers/Resources.hpp"

#include "helpers/Log.hpp"

#ifndef M_PI
#define M_PI       3.14159265358979323846   // pi
#endif

using namespace DirectX;

#define SHADOW_MAP_SIZE 1024

// Constant buffers.

struct BasicVsCB {
	XMFLOAT4X4 mvp;
};

struct LitVsCB {
	XMFLOAT4X4 MV;
	XMFLOAT4X4 invMV;
	XMFLOAT4X4 lightMVP;
};

struct LitPsCB {
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

void loadTexture(const std::string& path, ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D*& texture, ID3D11ShaderResourceView*& view) {
	Image image;
	Resources::loadImage(path, image, true);
	
	ID3D11Texture2D* dxTexture{nullptr};

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = image.w;
 	desc.Height = image.h;
 	desc.MipLevels = 0;
 	desc.ArraySize = 1;
 	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
 	desc.SampleDesc.Count = 1;
 	desc.SampleDesc.Quality = 0;
 	desc.Usage = D3D11_USAGE_DEFAULT;
 	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
 	desc.CPUAccessFlags = 0;
 	desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	DX_RET(device->CreateTexture2D(&desc, nullptr, &texture));
	DX_RET(device->CreateShaderResourceView(texture, nullptr, &view));

	// Fill data
	context->UpdateSubresource( texture, 0, nullptr, image.data.data(), image.w * 4, image.w * image.h * 4 );
	// Generate mips
	context->GenerateMips( view );
}

void loadShaders(const std::string& path, bool _fullLayout, ID3D11Device* device, ID3D11InputLayout*& layout, ID3D11VertexShader*& vertex, ID3D11PixelShader** pixel) {
	
	size_t vertexShaderSize = 0;
	char* vertexShaderData = Resources::loadRawDataFromExternalFile(path + "VertexShader.cso", vertexShaderSize);
	DX_RET(device->CreateVertexShader(vertexShaderData, vertexShaderSize, nullptr, &vertex));
	
	if( pixel )
	{
		size_t pixelShaderSize = 0;
		char* pixelShaderData = Resources::loadRawDataFromExternalFile( path + "PixelShader.cso", pixelShaderSize );
		DX_RET( device->CreatePixelShader( pixelShaderData, pixelShaderSize, nullptr, pixel ) );
		delete[] pixelShaderData;
	}

	{
		D3D11_INPUT_ELEMENT_DESC attribs[5];

		attribs[0].SemanticName = "POSITION";
		attribs[0].SemanticIndex = 0;
		attribs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attribs[0].InputSlot = 0;
		attribs[0].AlignedByteOffset = offsetof(Vertex, pos);
		attribs[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attribs[0].InstanceDataStepRate = 0;

		if( _fullLayout ){
			attribs[ 1 ].SemanticName = "NORMAL";
			attribs[ 1 ].SemanticIndex = 0;
			attribs[ 1 ].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			attribs[ 1 ].InputSlot = 0;
			attribs[ 1 ].AlignedByteOffset = offsetof( Vertex, normal );
			attribs[ 1 ].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			attribs[ 1 ].InstanceDataStepRate = 0;

			attribs[ 2 ].SemanticName = "TANGENT";
			attribs[ 2 ].SemanticIndex = 0;
			attribs[ 2 ].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			attribs[ 2 ].InputSlot = 0;
			attribs[ 2 ].AlignedByteOffset = offsetof( Vertex, tangent );
			attribs[ 2 ].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			attribs[ 2 ].InstanceDataStepRate = 0;

			attribs[ 3 ].SemanticName = "BINORMAL";
			attribs[ 3 ].SemanticIndex = 0;
			attribs[ 3 ].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			attribs[ 3 ].InputSlot = 0;
			attribs[ 3 ].AlignedByteOffset = offsetof( Vertex, binormal );
			attribs[ 3 ].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			attribs[ 3 ].InstanceDataStepRate = 0;

			attribs[ 4 ].SemanticName = "TEXCOORD";
			attribs[ 4 ].SemanticIndex = 0;
			attribs[ 4 ].Format = DXGI_FORMAT_R32G32_FLOAT;
			attribs[ 4 ].InputSlot = 0;
			attribs[ 4 ].AlignedByteOffset = offsetof( Vertex, texCoord );
			attribs[ 4 ].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			attribs[ 4 ].InstanceDataStepRate = 0;
		}

		DX_RET(device->CreateInputLayout(attribs, _fullLayout ? 5 : 1, vertexShaderData, vertexShaderSize, &layout));
	}

	delete[] vertexShaderData;
}

void Renderer::Object::load(const std::string& name, ID3D11Device* device ) {
	Mesh mesh;
	Resources::loadMesh("resources/models/" + name + ".obj", mesh);
	vbCount = (unsigned int)mesh.vertices.size();
	ibCount = (unsigned int)mesh.indices.size();

	// create buffers
	{
		D3D11_BUFFER_DESC desc{};
		desc.ByteWidth = sizeof(Vertex) * vbCount;
  		desc.Usage = D3D11_USAGE_IMMUTABLE;
  		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  		desc.CPUAccessFlags = 0;
  		desc.MiscFlags = 0;
  		desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA data{};
		data.pSysMem = (void*)mesh.vertices.data();
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;

		DX_RET(device->CreateBuffer(&desc, &data, &vertexBuffer));
	}

	{
		D3D11_BUFFER_DESC desc{};
		desc.ByteWidth = sizeof(unsigned int) * ibCount;
  		desc.Usage = D3D11_USAGE_IMMUTABLE;
  		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  		desc.CPUAccessFlags = 0;
  		desc.MiscFlags = 0;
  		desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA data{};
		data.pSysMem = mesh.indices.data();
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;

		DX_RET(device->CreateBuffer(&desc, &data, &indexBuffer));
	}
}

void Renderer::ShadedObject::load(const std::string& name, ID3D11Device* device, ID3D11DeviceContext* context ) {
	Object::load(name, device);
	loadTexture("resources/textures/" + name + ".png", device, context, diffuseTexture, diffuse);
	loadTexture("resources/textures/" + name + "_normal.png", device, context, normalTexture, normal);
	
}

void Renderer::Skybox::load(const std::string& name, ID3D11Device* device, ID3D11DeviceContext* context ) {
	Object::load("cube", device);

	std::vector<Image> faces(6);
	const std::string suffixes[] = { "_r", "_l", "_u", "_d", "_b", "_f"};
	for (int i = 0; i < 6; ++i) {
		Resources::loadImage("resources/textures/" + name + "/" + name + suffixes[i] + ".png", faces[i], false);
	}

	{
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = faces[0].w;
	 	desc.Height = faces[0].h;
	 	desc.MipLevels = 0;
	 	desc.ArraySize = 6;
	 	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	 	desc.SampleDesc.Count = 1;
	 	desc.SampleDesc.Quality = 0;
	 	desc.Usage = D3D11_USAGE_DEFAULT;
	 	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	 	desc.CPUAccessFlags = 0;
	 	desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;

		DX_RET(device->CreateTexture2D(&desc, nullptr, &cubemapTexture));
		DX_RET(device->CreateShaderResourceView(cubemapTexture, nullptr, &cubemap));

		// Fill data
		D3D11_TEXTURE2D_DESC cubeDesc{};
		cubemapTexture->GetDesc( &cubeDesc );
		for( int i = 0; i < 6; ++i )
		{
			const Image& face = faces[ i ];
			UINT subResource = D3D11CalcSubresource( 0, i, cubeDesc.MipLevels );
			context->UpdateSubresource( cubemapTexture, subResource, nullptr, face.data.data(), face.w * 4, face.w * face.h * 4 );
		}
		// Generate mips
		context->GenerateMips( cubemap );
	}
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

	// Swapchain
	{
		DXGI_SWAP_CHAIN_DESC swapDesc{};
		swapDesc.BufferDesc.Width = _w;
		swapDesc.BufferDesc.Height = _h;
		swapDesc.BufferDesc.RefreshRate = {1, 60};
		swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapDesc.SampleDesc.Count = 1;
		swapDesc.SampleDesc.Quality = 0;
		swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapDesc.BufferCount = 2;
		swapDesc.OutputWindow = _window;
		swapDesc.Windowed = true;
		swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapDesc.Flags = 0;
		UINT creationFlags = 0;
#ifdef _DEBUG
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		ID3D11DeviceContext* contextV0;
		DX_RET(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags, nullptr, 0 /*D3D11_CREATE_DEVICE_DEBUG*/, D3D11_SDK_VERSION, &swapDesc, &_swapchain, &_device, nullptr, &contextV0));
		
		DX_RET(contextV0->QueryInterface<ID3D11DeviceContext1>(&_context));
		contextV0->Release(); // QueryInterface adds a reference to the object via the new interface.

		rebuildSwapchainDepthAndViews();
	}

	{
		// Shadowmap
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = SHADOW_MAP_SIZE;
	 	desc.Height = SHADOW_MAP_SIZE;
	 	desc.MipLevels = 1;
	 	desc.ArraySize = 1;
	 	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	 	desc.SampleDesc.Count = 1;
	 	desc.SampleDesc.Quality = 0;
	 	desc.Usage = D3D11_USAGE_DEFAULT;
	 	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	 	desc.CPUAccessFlags = 0;
	 	desc.MiscFlags = 0;

		DX_RET(_device->CreateTexture2D(&desc, nullptr, &_shadowMapDepth));

		D3D11_SHADER_RESOURCE_VIEW_DESC vDesc{};
		vDesc.Format = DXGI_FORMAT_R32_FLOAT;
		vDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		vDesc.Texture2D.MostDetailedMip = 0;
		vDesc.Texture2D.MipLevels = 1;
		DX_RET( _device->CreateShaderResourceView(_shadowMapDepth, &vDesc, &_shadowMap));

		D3D11_DEPTH_STENCIL_VIEW_DESC  dDesc{};
		dDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dDesc.Flags = 0;
		dDesc.Texture2D.MipSlice = 0;
		DX_RET( _device->CreateDepthStencilView(_shadowMapDepth, &dDesc, &_shadowMapDepthRT));

	}

	{
		// Shared constant buffers
		// Estimate size, taking into account constants alignment constraints.
		unsigned int cbSizeInBytes = alignOnCbOffset(sizeof(PerFrameCB));
		cbSizeInBytes += LIT_OBJ_COUNT * (alignOnCbOffset(sizeof(BasicVsCB)) + alignOnCbOffset(sizeof(LitVsCB)) + alignOnCbOffset(sizeof(LitPsCB)));
		cbSizeInBytes += UNLIT_OBJ_COUNT * alignOnCbOffset(sizeof(BasicVsCB));

		D3D11_BUFFER_DESC desc{};
		desc.ByteWidth = cbSizeInBytes;
  		desc.Usage = D3D11_USAGE_DYNAMIC;
  		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  		desc.MiscFlags = 0;
  		desc.StructureByteStride = 0;

		for (int i = 0; i < CB_COUNT; ++i) {
			DX_RET(_device->CreateBuffer(&desc, nullptr, &_constantBuffers[i]));
		}
	}

	{
		// General state
		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
  		blendDesc.RenderTarget[0].BlendEnable = false;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = 0b1111;
		DX_RET(_device->CreateBlendState(&blendDesc, &_blendState));	

		D3D11_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		depthDesc.StencilEnable = FALSE;
		DX_RET(_device->CreateDepthStencilState(&depthDesc, &_depthState));	

		D3D11_RASTERIZER_DESC rasterDesc{};
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		rasterDesc.CullMode = D3D11_CULL_BACK;
		rasterDesc.FrontCounterClockwise = TRUE;
		rasterDesc.DepthBias = 0;
		rasterDesc.DepthBiasClamp = 0.f;
		rasterDesc.SlopeScaledDepthBias = 0.f;
		rasterDesc.DepthClipEnable = TRUE;
		rasterDesc.ScissorEnable = FALSE;
		rasterDesc.MultisampleEnable = FALSE;
		rasterDesc.AntialiasedLineEnable = FALSE;
		DX_RET(_device->CreateRasterizerState(&rasterDesc, &_rasterState));	
	}

	// Samplers
	{
		D3D11_SAMPLER_DESC desc{};
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.MipLODBias = 0;
		desc.MaxAnisotropy = 0;
		desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 0.F;
		desc.MinLOD = 0.f;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		DX_RET(_device->CreateSamplerState(&desc, &_shadowSampler));

		desc.Filter = D3D11_FILTER_ANISOTROPIC;
		desc.MaxAnisotropy = 8;
		DX_RET(_device->CreateSamplerState(&desc, &_anisoSampler));
	}


	// Load shaders
	loadShaders("resources/shaders/Lit", true, _device, _litLayout, _litVertexShader, &_litPixelShader);
	loadShaders("resources/shaders/Unlit", false, _device, _unlitLayout, _unlitVertexShader, &_unlitPixelShader);
	loadShaders("resources/shaders/Shadow", false, _device, _shadowLayout, _shadowVertexShader, nullptr);

	_objects.resize( 3 );

	_objects[0].load("dragon", _device, _context);
	_objects[0].model = XMMatrixScaling(1.2f, 1.2f, 1.2f) * XMMatrixTranslation(-0.5f, 0.0f, -0.5f);
	_objects[0].shininess = 64.f;

	_objects[1].load("suzanne", _device, _context );
	_objects[1].model =  XMMatrixScaling(0.65f, 0.65f, 0.65f) * XMMatrixTranslation(0.5f, 0.0f, 0.5f);
	_objects[1].shininess = 8.f;

	_objects[2].load("plane", _device, _context );
	_objects[2].model = XMMatrixScaling(2.75f, 2.75f, 2.75f) * XMMatrixTranslation(0.0f, -0.8f, 0.0f);
	_objects[2].shininess = 32.f;

	_skybox.load("cubemap", _device, _context );
	_skybox.model =  XMMatrixScaling(15.0f, 15.0f, 15.0f);

	// Light init
	_worldLightDir = XMVector3Normalize(XMVectorSet(1.0f, 0.6f, 1.0f, 0.0f));
	_lightView = XMMatrixLookAtRH(5.f * _worldLightDir, XMVectorZero(), XMVectorSet(0.f, 1.0f, 0.f, 0.f));
	_lightProj = XMMatrixOrthographicRH(10.f, 10.f, 0.1f, 10.f); // This will never change
	
	
}

Renderer::~Renderer()
{
	_context->ClearState();
	
	for(ShadedObject& obj : _objects){
		obj.diffuse->Release();
		obj.normal->Release();
		obj.diffuseTexture->Release();
		obj.normalTexture->Release();
		obj.indexBuffer->Release();
		obj.vertexBuffer->Release();
	}

	_skybox.cubemap->Release();
	_skybox.cubemapTexture->Release();
	_skybox.indexBuffer->Release();
	_skybox.vertexBuffer->Release();

	// Views
	_shadowMapDepthRT->Release();
	_shadowMap->Release();
	_swapchainColorRT->Release();
	_swapchainDepthRT->Release();

	// Textures
	_swapchainDepth->Release();
	_swapchainColor->Release();
	_shadowMapDepth->Release();

	// Buffers
	for (int i = 0; i < CB_COUNT; ++i) {
		_constantBuffers[i]->Release();
	}

	// Layouts
	_litLayout->Release();
	_unlitLayout->Release();
	_shadowLayout->Release();

	// Shaders
	_litVertexShader->Release();
	_litPixelShader->Release();
	_unlitVertexShader->Release();
	_unlitPixelShader->Release();
	_shadowVertexShader->Release();

	// State
	_anisoSampler->Release();
	_shadowSampler->Release();
	_rasterState->Release();
	_depthState->Release();
	_blendState->Release();

	// Global objects
	_context->Release();
	_swapchain->Release();
	_device->Release();
}

void Renderer::rebuildSwapchainDepthAndViews(){
	DX_RET(_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&_swapchainColor ));
	DX_RET(_device->CreateRenderTargetView( _swapchainColor, NULL, &_swapchainColorRT));
	
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = _w;
	desc.Height = _h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	DX_RET( _device->CreateTexture2D(&desc, nullptr, &_swapchainDepth));

	D3D11_DEPTH_STENCIL_VIEW_DESC  dDesc{};
	dDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dDesc.Flags = 0;
	dDesc.Texture2D.MipSlice = 0;
	DX_RET( _device->CreateDepthStencilView(_swapchainDepth, &dDesc, &_swapchainDepthRT));

}


void Renderer::resize( unsigned int w, unsigned int h )
{
	_w = w;
	_h = h;

	_context->ClearState();

	_swapchainColorRT->Release();
	_swapchainColor->Release();

	_swapchainDepthRT->Release();
	_swapchainDepth->Release();

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
	_context->ClearState();

	// Clear rendertargets
	const float clearColor[] = { 0.f, 1.0f, 0.5f, 1.0f };
	_context->ClearRenderTargetView( _swapchainColorRT, clearColor );
	_context->ClearDepthStencilView( _shadowMapDepthRT, D3D11_CLEAR_DEPTH, 1.0f, 0 );
	_context->ClearDepthStencilView( _swapchainDepthRT, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	// Common state for both passes
	_context->RSSetState( _rasterState );
	_context->OMSetBlendState( _blendState, nullptr, 0xFFFFFFFF );
	_context->OMSetDepthStencilState( _depthState, 0 );
	_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Viewports
	D3D11_VIEWPORT mainViewport{ 0, 0, _w, _h, 0.f, 1.f };
	D3D11_VIEWPORT shadowViewport{ 0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0.f, 1.f };

	const unsigned int sizeOfVertex = sizeof(Vertex);
	const unsigned int offsetOfVertex = 0;

	// Constant buffer setup
	ID3D11Buffer* constantBuffer = _constantBuffers[_cbIndex];
	_cbIndex = (_cbIndex + 1) % CB_COUNT;

	// Prepare all data for each shaded object
	const UINT frameCbSize = alignOnCbOffset(sizeof(PerFrameCB)) / CB_CONSTANT_SIZE_IN_BYTES;
	const UINT shadowVsSize = alignOnCbOffset(sizeof(BasicVsCB)) / CB_CONSTANT_SIZE_IN_BYTES;
	const UINT unlitVsSize = alignOnCbOffset(sizeof(BasicVsCB)) / CB_CONSTANT_SIZE_IN_BYTES;
	const UINT litVsSize = alignOnCbOffset(sizeof(LitVsCB)) / CB_CONSTANT_SIZE_IN_BYTES;
	const UINT litPsSize = alignOnCbOffset(sizeof(LitPsCB)) / CB_CONSTANT_SIZE_IN_BYTES;

	UINT frameCbOffset = 0;
	UINT shadowVsOffsets[LIT_OBJ_COUNT];
	UINT litVsOffsets[LIT_OBJ_COUNT];
	UINT litPsOffsets[LIT_OBJ_COUNT];
	UINT unlitVsOffsets[UNLIT_OBJ_COUNT];

	// Update constant buffer data for this frame.
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource{};
		DX_RET(_context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
		unsigned char* data = (unsigned char*)mappedResource.pData;
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

			shadowVsOffsets[i] = currentOffsetInConstants;
			std::memcpy(data + CB_CONSTANT_SIZE_IN_BYTES * currentOffsetInConstants, &shadow, sizeof(shadow));
			currentOffsetInConstants += shadowVsSize;
			
			LitVsCB vsLit;
			XMMATRIX MV = object.model * _camera.matrix;
			XMMATRIX invMV = XMMatrixInverse(nullptr, MV);
			XMMATRIX lightMVP = object.model * lightVP;
			XMStoreFloat4x4(&vsLit.lightMVP, XMMatrixTranspose(lightMVP));
			XMStoreFloat4x4(&vsLit.MV, XMMatrixTranspose(MV));
			XMStoreFloat4x4(&vsLit.invMV, invMV); // Double transpose

			litVsOffsets[i] = currentOffsetInConstants;
			std::memcpy(data + CB_CONSTANT_SIZE_IN_BYTES * currentOffsetInConstants, &vsLit, sizeof(vsLit));
			currentOffsetInConstants += litVsSize;


			LitPsCB psLit;
			psLit.shininess = object.shininess;

			litPsOffsets[i] = currentOffsetInConstants;
			std::memcpy(data + CB_CONSTANT_SIZE_IN_BYTES * currentOffsetInConstants, &psLit, sizeof(psLit));
			currentOffsetInConstants += litPsSize;
			++i;
		}
		{
			BasicVsCB unlit;
			XMMATRIX VNoTranslation = _camera.matrix;
			VNoTranslation.r[ 3 ] = XMVectorSet( 0.f, 0.0f, 0.f, 1.f );
			XMMATRIX MVP = _skybox.model * VNoTranslation * _proj;
			XMStoreFloat4x4(&unlit.mvp, XMMatrixTranspose(MVP));

			unlitVsOffsets[0] = currentOffsetInConstants;
			std::memcpy(data + CB_CONSTANT_SIZE_IN_BYTES * currentOffsetInConstants, &unlit, sizeof(unlit));
			currentOffsetInConstants += unlitVsSize;
		}
		_context->Unmap(constantBuffer, 0);
	}
	

	// Shadow pass
	{
		_context->RSSetViewports( 1, &shadowViewport );
		_context->OMSetRenderTargets(0, nullptr, _shadowMapDepthRT);

		_context->IASetInputLayout(_shadowLayout);
		_context->VSSetShader(_shadowVertexShader, nullptr, 0);
		_context->PSSetShader(nullptr, nullptr, 0);

		int i = 0;
		for (const ShadedObject& obj : _objects) {
			_context->VSSetConstantBuffers1(0, 1, &constantBuffer, &shadowVsOffsets[i], &shadowVsSize);
			_context->IASetVertexBuffers(0, 1, &obj.vertexBuffer, &sizeOfVertex, &offsetOfVertex);
			_context->IASetIndexBuffer(obj.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
			_context->DrawIndexed(obj.ibCount, 0, 0);
			++i;
		}
	}
	
	// Main pass
	{
		_context->RSSetViewports( 1, &mainViewport );
		_context->OMSetRenderTargets(1, &_swapchainColorRT, _swapchainDepthRT);

		// Objects
		_context->IASetInputLayout(_litLayout);
		_context->VSSetShader(_litVertexShader, nullptr, 0);
		_context->PSSetShader(_litPixelShader, nullptr, 0);
		// Common resources
		_context->PSSetShaderResources(2, 1, &_shadowMap);
		_context->PSSetSamplers(0, 1, &_anisoSampler);
		_context->PSSetSamplers(1, 1, &_shadowSampler);
		_context->VSSetConstantBuffers1(0, 1, &constantBuffer, &frameCbOffset, &frameCbSize);
		_context->PSSetConstantBuffers1(0, 1, &constantBuffer, &frameCbOffset, &frameCbSize);
		int i = 0;
		for (const ShadedObject& obj : _objects) {
			_context->VSSetConstantBuffers1(1, 1, &constantBuffer, &litVsOffsets[i], &litVsSize);
			_context->PSSetConstantBuffers1(1, 1, &constantBuffer, &litPsOffsets[i], &litPsSize);
			_context->PSSetShaderResources(0, 1, &obj.diffuse);
			_context->PSSetShaderResources(1, 1, &obj.normal);

			_context->IASetVertexBuffers( 0, 1, &obj.vertexBuffer, &sizeOfVertex, &offsetOfVertex );
			_context->IASetIndexBuffer(obj.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
			_context->DrawIndexed(obj.ibCount, 0, 0);
			++i;
		}

		// Skybox
		_context->IASetInputLayout(_unlitLayout);
		_context->VSSetShader(_unlitVertexShader, nullptr, 0);
		_context->PSSetShader(_unlitPixelShader, nullptr, 0);
		// Common resources
		_context->PSSetSamplers(0, 1, &_anisoSampler);
		_context->VSSetConstantBuffers1(0, 1, &constantBuffer, &unlitVsOffsets[0], &unlitVsSize);
		_context->PSSetShaderResources(0, 1, &_skybox.cubemap);

		_context->IASetVertexBuffers(0, 1, &_skybox.vertexBuffer, &sizeOfVertex, &offsetOfVertex);
		_context->IASetIndexBuffer(_skybox.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		_context->DrawIndexed(_skybox.ibCount, 0, 0);

	}

	HRESULT status = _swapchain->Present( 1, 0 );
	// Quit if occluded, to fix application sometimes remaining active after quitting
	return status != S_OK;
}

