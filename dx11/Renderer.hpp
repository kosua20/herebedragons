#pragma once

#include <vector>
#include <string>
#include <DirectXMath.h>

#include <d3d11_1.h>
#pragma comment (lib, "d3d11.lib")

#define CB_COUNT 3

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

	struct Object {

		ID3D11Buffer* vertexBuffer{nullptr};
		ID3D11Buffer* indexBuffer{nullptr};

		unsigned int vbCount{ 0 };
		unsigned int ibCount{ 0 };
		DirectX::XMMATRIX model;

		void load(const std::string& name, ID3D11Device* device);
	};

	struct ShadedObject : public Object {

		ID3D11Texture2D* diffuseTexture{nullptr};
		ID3D11ShaderResourceView* diffuse{nullptr};

		ID3D11Texture2D* normalTexture{nullptr};
		ID3D11ShaderResourceView* normal{nullptr};

		float shininess;

		void load(const std::string& name, ID3D11Device* device, ID3D11DeviceContext* context );
	};


	struct Skybox : public Object {

		ID3D11Texture2D* cubemapTexture{nullptr};
		ID3D11ShaderResourceView* cubemap{nullptr};

		void load(const std::string& name, ID3D11Device* device, ID3D11DeviceContext* context );
	};

	// Device and context
	ID3D11Device* _device{nullptr};
	ID3D11DeviceContext1* _context{nullptr};
	// Backbuffer and views
	IDXGISwapChain* _swapchain{ nullptr };
	ID3D11Texture2D* _swapchainColor{ nullptr };
	ID3D11RenderTargetView* _swapchainColorRT{nullptr};
	ID3D11Texture2D* _swapchainDepth{nullptr};
	ID3D11DepthStencilView* _swapchainDepthRT{nullptr};
	// Shadow map resource and views
	ID3D11Texture2D* _shadowMapDepth{nullptr};
	ID3D11DepthStencilView* _shadowMapDepthRT{nullptr};
	ID3D11ShaderResourceView* _shadowMap{nullptr};

	// Global constant buffer
	ID3D11Buffer* _constantBuffers[CB_COUNT];

	// Shaders and layout for lit/unlit/shadow
	ID3D11InputLayout* _litLayout{nullptr};
	ID3D11VertexShader* _litVertexShader{nullptr};
	ID3D11PixelShader* _litPixelShader{nullptr};

	ID3D11InputLayout* _unlitLayout{nullptr};
	ID3D11VertexShader* _unlitVertexShader{nullptr};
	ID3D11PixelShader* _unlitPixelShader{nullptr};

	ID3D11InputLayout* _shadowLayout{nullptr};
	ID3D11VertexShader* _shadowVertexShader{nullptr};

	// TODO: need different states for shadow and main passes?
	ID3D11RasterizerState* _rasterState{nullptr};
	ID3D11DepthStencilState* _depthState{nullptr};
	ID3D11BlendState* _blendState{nullptr};

	ID3D11SamplerState* _anisoSampler{nullptr};
	ID3D11SamplerState* _shadowSampler{nullptr};

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
	UINT _cbIndex = 0;
};