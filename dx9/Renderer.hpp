#pragma once

#include <vector>
#include <string>
#include <DirectXMath.h>

#include <d3d9.h>
#pragma comment (lib, "d3d9.lib")


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

	void setDefaultState();

	struct Object
	{
		IDirect3DVertexBuffer9* vertexBuffer{ nullptr };
		IDirect3DIndexBuffer9* indexBuffer{ nullptr };
		unsigned int vbCount{ 0 };
		unsigned int ibCount{ 0 };
		DirectX::XMMATRIX model;

		void load(const std::string& name, IDirect3DDevice9* device);
	};

	struct ShadedObject : public Object {
		IDirect3DTexture9* diffuse{ nullptr };
		IDirect3DTexture9* normal{ nullptr };
		float shininess;

		void load(const std::string& name, IDirect3DDevice9* device);
	};


	struct Skybox : public Object {
		IDirect3DCubeTexture9* cubemap{ nullptr };
		void load(const std::string& name, IDirect3DDevice9* device);
	};


	IDirect3D9* _api{ nullptr };
	IDirect3DDevice9* _device{ nullptr };
	IDirect3DTexture9* _shadowMap{ nullptr };
	IDirect3DSurface9* _shadowMapDepth{ nullptr };

	IDirect3DVertexDeclaration9* _fullVertexDec{ nullptr };
	IDirect3DVertexDeclaration9* _simpleVertexDec{ nullptr };

	IDirect3DVertexShader9* _litVertexShader{ nullptr };
	IDirect3DPixelShader9* _litPixelShader{ nullptr };
	IDirect3DVertexShader9* _unlitVertexShader{ nullptr };
	IDirect3DPixelShader9* _unlitPixelShader{ nullptr };
	IDirect3DVertexShader9* _shadowVertexShader{ nullptr };
	IDirect3DPixelShader9* _shadowPixelShader{ nullptr };

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