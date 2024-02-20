#include "Renderer.hpp"
#include "helpers/Resources.hpp"

#include "helpers/Log.hpp"

#ifndef M_PI
#define M_PI       3.14159265358979323846   // pi
#endif

using namespace DirectX;

#define SHADOW_MAP_SIZE 1024

IDirect3DTexture9* loadTexture(const std::string& path, IDirect3DDevice9* device) {
	Image texture;
	Resources::loadImage(path, texture, true);
	texture.swizzle(0, 2);

	IDirect3DTexture9* dxTexture;
	DX_RET(device->CreateTexture(texture.w, texture.h, 0, D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &dxTexture, nullptr));
	D3DLOCKED_RECT lockedRect;
	DX_RET(dxTexture->LockRect(0, &lockedRect, nullptr, 0));
	unsigned int srcPitch = texture.c * texture.w;
	for (unsigned int y = 0; y < texture.h; ++y)
	{
		unsigned char* dst = (unsigned char*)(lockedRect.pBits) + y * lockedRect.Pitch;
		unsigned char* src = texture.data.data() + y * srcPitch;
		std::memcpy(dst, src, srcPitch);
	}
	DX_RET(dxTexture->UnlockRect(0));
	return dxTexture;
}

void loadShaders(const std::string& path, IDirect3DDevice9* device, IDirect3DVertexShader9*& vertex, IDirect3DPixelShader9*& pixel) {
	{
		size_t shaderSize = 0;
		char* shaderData = Resources::loadRawDataFromExternalFile(path + "VertexShader.cso", shaderSize);
		DX_RET(device->CreateVertexShader((DWORD*)shaderData, &vertex));
		delete[] shaderData;
	}
	{
		size_t shaderSize = 0;
		char* shaderData = Resources::loadRawDataFromExternalFile(path + "PixelShader.cso", shaderSize);
		DX_RET(device->CreatePixelShader((DWORD*)shaderData, &pixel));
		delete[] shaderData;
	}
}

void Renderer::Object::load(const std::string& name, IDirect3DDevice9* device) {
	Mesh mesh;
	Resources::loadMesh("resources/models/" + name + ".obj", mesh);
	vbCount = (unsigned int)mesh.vertices.size();
	ibCount = (unsigned int)mesh.indices.size();

	// create buffers
	{
		const unsigned int srcSize = vbCount * sizeof(Vertex);
		DX_RET(device->CreateVertexBuffer(srcSize, 0, 0, D3DPOOL_MANAGED, &vertexBuffer, nullptr));
		void* lockedData;
		DX_RET(vertexBuffer->Lock(0, srcSize, &lockedData, 0));
		std::memcpy(lockedData, mesh.vertices.data(), srcSize);
		DX_RET(vertexBuffer->Unlock());
	}
	{
		const unsigned int srcSize = ibCount * sizeof(unsigned int);
		DX_RET(device->CreateIndexBuffer(srcSize, 0, D3DFMT_INDEX32, D3DPOOL_MANAGED, &indexBuffer, nullptr));
		void* lockedData;
		DX_RET(indexBuffer->Lock(0, srcSize, &lockedData, 0));
		std::memcpy(lockedData, mesh.indices.data(), srcSize);
		DX_RET(indexBuffer->Unlock());
	}
}

void Renderer::ShadedObject::load(const std::string& name, IDirect3DDevice9* device) {
	Object::load(name, device);
	diffuse = loadTexture("resources/textures/" + name + ".png", device);
	normal = loadTexture("resources/textures/" + name + "_normal.png", device);
}

void Renderer::Skybox::load(const std::string& name, IDirect3DDevice9* device) {
	Object::load("cube", device);

	std::vector<Image> faces(6);

	
	const std::string suffixes[] = { "_r", "_l", "_u", "_d", "_b", "_f"};

	for (int i = 0; i < 6; ++i) {
		Resources::loadImage("resources/textures/" + name + "/" + name + suffixes[i] + ".png", faces[i], false);
		faces[i].swizzle(0, 2);
	}

	DX_RET(device->CreateCubeTexture(faces[0].w, 0, D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &cubemap, nullptr));
	
	for (int i = 0; i < 6; ++i) {
		D3DCUBEMAP_FACES faceId = D3DCUBEMAP_FACES(D3DCUBEMAP_FACE_POSITIVE_X + i);
		Image& face = faces[i];
		D3DLOCKED_RECT lockedRect;
		DX_RET(cubemap->LockRect(faceId, 0, &lockedRect, nullptr, 0));
		unsigned int srcPitch = face.c * face.w;
		for (unsigned int y = 0; y < face.h; ++y)
		{
			unsigned char* dst = (unsigned char*)(lockedRect.pBits) + y * lockedRect.Pitch;
			unsigned char* src = face.data.data() + y * srcPitch;
			std::memcpy(dst, src, srcPitch);
		}
		DX_RET(cubemap->UnlockRect(faceId, 0));
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
	_api = Direct3DCreate9( D3D_SDK_VERSION );
	
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

	{
		D3DPRESENT_PARAMETERS d3dpp;
		memset( &d3dpp, 0, sizeof( d3dpp ) );
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.hDeviceWindow = _window;
		d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
		d3dpp.BackBufferWidth = _w;
		d3dpp.BackBufferHeight = _h;
		d3dpp.EnableAutoDepthStencil = TRUE;
		d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
		DX_RET(_api->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, _window, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &_device ));
	}

	logInfo("Device %s\n", _device ? "OK" : "NO");
	if (!_device)
		return;

	setDefaultState();

	{
		D3DVERTEXELEMENT9 attribs[6];
		attribs[0].Method = D3DDECLMETHOD_DEFAULT;
		attribs[0].Offset = offsetof(Vertex, pos);
		attribs[0].Stream = 0;
		attribs[0].Type = D3DDECLTYPE_FLOAT3;
		attribs[0].Usage = D3DDECLUSAGE_POSITION;
		attribs[0].UsageIndex = 0;

		attribs[1].Method = D3DDECLMETHOD_DEFAULT;
		attribs[1].Offset = offsetof(Vertex, normal);
		attribs[1].Stream = 0;
		attribs[1].Type = D3DDECLTYPE_FLOAT3;
		attribs[1].Usage = D3DDECLUSAGE_NORMAL;
		attribs[1].UsageIndex = 0;

		attribs[2].Method = D3DDECLMETHOD_DEFAULT;
		attribs[2].Offset = offsetof(Vertex, tangent);
		attribs[2].Stream = 0;
		attribs[2].Type = D3DDECLTYPE_FLOAT3;
		attribs[2].Usage = D3DDECLUSAGE_TANGENT;
		attribs[2].UsageIndex = 0;

		attribs[3].Method = D3DDECLMETHOD_DEFAULT;
		attribs[3].Offset = offsetof(Vertex, binormal);
		attribs[3].Stream = 0;
		attribs[3].Type = D3DDECLTYPE_FLOAT3;
		attribs[3].Usage = D3DDECLUSAGE_BINORMAL;
		attribs[3].UsageIndex = 0;

		attribs[4].Method = D3DDECLMETHOD_DEFAULT;
		attribs[4].Offset = offsetof(Vertex, texCoord);
		attribs[4].Stream = 0;
		attribs[4].Type = D3DDECLTYPE_FLOAT2;
		attribs[4].Usage = D3DDECLUSAGE_TEXCOORD;
		attribs[4].UsageIndex = 0;

		attribs[5] = D3DDECL_END();
		DX_RET(_device->CreateVertexDeclaration(attribs, &_fullVertexDec));
	}
	{
		D3DVERTEXELEMENT9 attribs[2];
		attribs[0].Method = D3DDECLMETHOD_DEFAULT;
		attribs[0].Offset = offsetof(Vertex, pos);
		attribs[0].Stream = 0;
		attribs[0].Type = D3DDECLTYPE_FLOAT3;
		attribs[0].Usage = D3DDECLUSAGE_POSITION;
		attribs[0].UsageIndex = 0;

		attribs[1] = D3DDECL_END();
		DX_RET(_device->CreateVertexDeclaration(attribs, &_simpleVertexDec));
	}

	// Load shaders
	loadShaders("resources/shaders/Lit", _device, _litVertexShader, _litPixelShader);
	loadShaders("resources/shaders/Unlit", _device, _unlitVertexShader, _unlitPixelShader);
	loadShaders("resources/shaders/Shadow", _device, _shadowVertexShader, _shadowPixelShader);

	_objects.resize( 3 );

	_objects[0].load("dragon", _device);
	_objects[0].model = XMMatrixScaling(1.2f, 1.2f, 1.2f) * XMMatrixTranslation(-0.5f, 0.0f, -0.5f);
	_objects[0].shininess = 64.f;

	_objects[1].load("suzanne", _device);
	_objects[1].model =  XMMatrixScaling(0.65f, 0.65f, 0.65f) * XMMatrixTranslation(0.5f, 0.0f, 0.5f);
	_objects[1].shininess = 8.f;

	_objects[2].load("plane", _device);
	_objects[2].model = XMMatrixScaling(2.75f, 2.75f, 2.75f) * XMMatrixTranslation(0.0f, -0.8f, 0.0f);
	_objects[2].shininess = 32.f;

	_skybox.load("cubemap", _device);
	_skybox.model =  XMMatrixScaling(15.0f, 15.0f, 15.0f);

	// Light init
	_worldLightDir = XMVector3Normalize(XMVectorSet(1.0f, 0.6f, 1.0f, 0.0f));
	_lightView = XMMatrixLookAtRH(5.f * _worldLightDir, XMVectorZero(), XMVectorSet(0.f, 1.0f, 0.f, 0.f));
	_lightProj = XMMatrixOrthographicRH(10.f, 10.f, 0.1f, 10.f); // This will never change
	
	
}

Renderer::~Renderer()
{
	for( ShadedObject& object : _objects )
	{
		object.diffuse->Release();
		object.normal->Release();
		object.vertexBuffer->Release();
		object.indexBuffer->Release();
	}
	_skybox.cubemap->Release();
	_skybox.vertexBuffer->Release();
	_skybox.indexBuffer->Release();

	_litVertexShader->Release();
	_litPixelShader->Release();
	_unlitVertexShader->Release();
	_unlitPixelShader->Release();
	_simpleVertexDec->Release();
	_fullVertexDec->Release();
	_shadowMap->Release();
	_shadowMapDepth->Release();

	_device->Release();
	_api->Release();
}

void Renderer::resize( unsigned int w, unsigned int h )
{
	_w = w;
	_h = h;

	// Cleanup rendertarget
	_shadowMap->Release();
	_shadowMapDepth->Release();

	// Reset the device. Shaders and managed data don't have to be recreated.
	D3DPRESENT_PARAMETERS d3dpp;
	memset( &d3dpp, 0, sizeof( d3dpp ) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = _window;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth = _w;
	d3dpp.BackBufferHeight = _h;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	DX_RET(_device->Reset( &d3dpp ));

	setDefaultState();
}

void Renderer::setDefaultState()
{
	_device->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );
	_device->SetRenderState( D3DRS_LIGHTING, FALSE );

	_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
	_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
	_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	_device->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 8);

	_device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
	_device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
	_device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	_device->SetSamplerState(1, D3DSAMP_MAXANISOTROPY, 8);

	_device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	_device->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	_device->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	// Recreate rendertargets
	DX_RET(_device->CreateDepthStencilSurface(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &_shadowMapDepth, nullptr));
	DX_RET(_device->CreateTexture(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R16F, D3DPOOL_DEFAULT, &_shadowMap, nullptr));

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
	// Viewports
	D3DVIEWPORT9 mainViewport{ 0, 0, _w, _h, 0.f, 1.f };
	D3DVIEWPORT9 shadowViewport{ 0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0.f, 1.f };

	// Retrieve the backbuffer, prepare viewport
	IDirect3DSurface9* finalRT{ nullptr };
	IDirect3DSurface9* finalDepth{ nullptr };
	DX_RET(_device->GetRenderTarget(0, &finalRT));
	DX_RET(_device->GetDepthStencilSurface(&finalDepth));
	finalRT->Release();
	finalDepth->Release();

	// Retrieve the shadowmap RT
	IDirect3DSurface9* shadowMapRT{ nullptr };
	DX_RET(_shadowMap->GetSurfaceLevel(0, &shadowMapRT));
	shadowMapRT->Release();

	// Set shadowmap RT and depth buffer
	DX_RET(_device->SetRenderTarget(0, shadowMapRT));
	DX_RET(_device->SetDepthStencilSurface(_shadowMapDepth));
	// Set viewport and clear
	DX_RET(_device->SetViewport(&shadowViewport));
	DX_RET(_device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(255, 0, 0), 1.0f, 0));

	DX_RET(_device->BeginScene());

	DX_RET(_device->SetVertexDeclaration(_simpleVertexDec));
	DX_RET(_device->SetVertexShader(_shadowVertexShader));
	DX_RET(_device->SetPixelShader(_shadowPixelShader));

	XMMATRIX lightVP = _lightView * _lightProj;
	for (const ShadedObject& object : _objects)
	{
		// Set parameters and textures
		XMMATRIX MVP = object.model * lightVP;
		XMFLOAT4X4 MVPm;
		XMStoreFloat4x4(&MVPm, XMMatrixTranspose(MVP));
		DX_RET(_device->SetVertexShaderConstantF(0, &MVPm.m[0][0], 4));

		// Draw
		DX_RET(_device->SetStreamSource(0, object.vertexBuffer, 0, sizeof(Vertex)));
		DX_RET(_device->SetIndices(object.indexBuffer));
		DX_RET(_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, object.vbCount, 0, object.ibCount / 3));
	}
	DX_RET(_device->EndScene());

	// Set backbuffer RT and depth buffer
	DX_RET(_device->SetRenderTarget(0, finalRT));
	DX_RET(_device->SetDepthStencilSurface(finalDepth));
	// Set viewport and clear
	DX_RET(_device->SetViewport(&mainViewport));
	DX_RET( _device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 40, 100 ), 1.0f, 0 ));

	DX_RET(_device->BeginScene());
	
	DX_RET( _device->SetVertexDeclaration( _fullVertexDec ) );
	DX_RET( _device->SetVertexShader( _litVertexShader ) );
	DX_RET( _device->SetPixelShader( _litPixelShader ) );

	XMFLOAT4X4 Pm;
	XMStoreFloat4x4(&Pm, XMMatrixTranspose(_proj));
	DX_RET(_device->SetVertexShaderConstantF(0, &Pm.m[0][0], 4));

	XMFLOAT4 lightDirViewSpace;
	XMStoreFloat4(&lightDirViewSpace, XMVector4Transform(_worldLightDir, _camera.matrix));
	DX_RET(_device->SetPixelShaderConstantF(0, &lightDirViewSpace.x, 1));

	for( const ShadedObject& object : _objects )
	{

		// Set parameters and textures
		XMMATRIX MV = object.model * _camera.matrix;
		XMMATRIX invMV = XMMatrixInverse(nullptr, MV);
		XMMATRIX lightMVP = object.model * lightVP;

		XMFLOAT4X4 MVm;
		XMStoreFloat4x4(&MVm, XMMatrixTranspose(MV));
		XMFLOAT4X4 invMVm;
		XMStoreFloat4x4( &invMVm, invMV ); // Double transpose.
		XMFLOAT4X4 lightMVPm;
		XMStoreFloat4x4(&lightMVPm, XMMatrixTranspose(lightMVP));

		DX_RET(_device->SetVertexShaderConstantF( 4, &MVm.m[ 0 ][ 0 ], 4 ));
		DX_RET(_device->SetVertexShaderConstantF( 8, &invMVm.m[0][0], 4 ));
		DX_RET(_device->SetVertexShaderConstantF( 12, &lightMVPm.m[0][0], 4 ));

		DX_RET(_device->SetPixelShaderConstantF( 1, &object.shininess, 1 ));
		DX_RET(_device->SetTexture( 0, object.diffuse ));
		DX_RET(_device->SetTexture( 1, object.normal ));
		DX_RET(_device->SetTexture( 2, _shadowMap ));

		// Draw
		DX_RET( _device->SetStreamSource( 0, object.vertexBuffer, 0, sizeof( Vertex ) ) );
		DX_RET( _device->SetIndices( object.indexBuffer ) );
		DX_RET( _device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, object.vbCount, 0, object.ibCount / 3 ) );
	}

	DX_RET(_device->SetVertexDeclaration(_simpleVertexDec));
	DX_RET(_device->SetVertexShader(_unlitVertexShader));
	DX_RET(_device->SetPixelShader(_unlitPixelShader));
	{
		XMMATRIX VNoTranslation = _camera.matrix;
		VNoTranslation.r[ 3 ] = XMVectorSet( 0.f, 0.0f, 0.f, 1.f );
		XMMATRIX MVP = XMMatrixTranspose(_skybox.model * VNoTranslation * _proj);
		XMFLOAT4X4 MVPm;
		XMStoreFloat4x4(&MVPm, MVP);

		DX_RET( _device->SetVertexShaderConstantF( 0, &MVPm.m[ 0 ][ 0 ], 4 ) );
		DX_RET( _device->SetTexture( 0, _skybox.cubemap ) );

		DX_RET(_device->SetStreamSource(0, _skybox.vertexBuffer, 0, sizeof(Vertex)));
		DX_RET(_device->SetIndices(_skybox.indexBuffer));
		DX_RET(_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, _skybox.vbCount, 0, _skybox.ibCount / 3));
	}
	
	
	DX_RET(_device->EndScene());
	HRESULT status = _device->Present( NULL, NULL, NULL, NULL );
	// Quit if occluded, to fix application sometimes remaining active after quitting
	return status != S_OK;
}

