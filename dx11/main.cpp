#include <chrono>
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Renderer.hpp"
#include "helpers/Log.hpp"

static unsigned int resizeWidth = 0;
static unsigned int resizeHeight = 0;

static MouseState mouse;

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;
		case WM_SIZE:
			if( wParam == SIZE_MINIMIZED )
				return 0;
			resizeWidth = ( unsigned int )LOWORD( lParam );
			resizeHeight = ( unsigned int )HIWORD( lParam );
			return 0;
		case WM_MOUSEWHEEL:
			mouse.scroll = (float)(GET_WHEEL_DELTA_WPARAM(wParam)) / (float)WHEEL_DELTA;
			return 0;
		case WM_LBUTTONDOWN:
			mouse.held = true;
			POINT mousePos;
			if (GetCursorPos(&mousePos) && ScreenToClient(hWnd, &mousePos)) {
				mouse.clickedX = mousePos.x;
				mouse.clickedY = mousePos.y;
			}
			return 0;
		case WM_LBUTTONUP:
			mouse.held = false;
			return 0;
		default:
			break;
	}
	return DefWindowProc( hWnd, message, wParam, lParam );
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					 _In_opt_ HINSTANCE hPrevInstance,
					 _In_ LPWSTR    lpCmdLine,
					 _In_ int       nCmdShow)
{
	// Setup window.
	WNDCLASSEXW wcex;
	memset( &wcex, 0, sizeof( WNDCLASSEX ) );
	wcex.cbSize = sizeof( WNDCLASSEX );
	// Fill in the struct with the needed information
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
	wcex.hbrBackground = ( HBRUSH )COLOR_WINDOW;
	wcex.lpszClassName = L"DragonDX11Window";
	// Register window class.
	RegisterClassEx( &wcex );

	// Perform application initialization:
	HWND hWnd = CreateWindowEx( NULL, L"DragonDX11Window", L"Dragon DX11", WS_OVERLAPPEDWINDOW, 300, 300, 800, 600, nullptr, nullptr, hInstance, nullptr );
	if( !hWnd ) {
		UnregisterClass( wcex.lpszClassName, wcex.hInstance );
		return -1;
	}

	ShowWindow( hWnd, nCmdShow );
	UpdateWindow( hWnd );

	MSG msg;

	const auto startTime = std::chrono::high_resolution_clock::now();
	auto previousTime = startTime;

	{
		Renderer renderer( hWnd );

		bool shouldQuit = false;
		while( !shouldQuit )
		{

			while( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
				if( msg.message == WM_QUIT )
					shouldQuit = true;
			}
			if( shouldQuit )
				break;

			if( resizeWidth != 0 && resizeHeight != 0 )
			{
				renderer.resize( resizeWidth, resizeHeight );
				resizeWidth = 0;
				resizeHeight = 0;
			}

			
			// update inputs
			POINT mousePos;
			if (GetCursorPos(&mousePos) && ScreenToClient(hWnd, &mousePos)) {
				mouse.currentX = mousePos.x;
				mouse.currentY = mousePos.y;
			}

			const auto currentTime{ std::chrono::high_resolution_clock::now() };
			const std::chrono::duration<double> deltaTime{ currentTime - previousTime };
			const std::chrono::duration<double> timeFromStart{ currentTime - startTime };
			previousTime = currentTime;

			renderer.update(mouse, timeFromStart.count(), deltaTime.count());

			shouldQuit = renderer.draw();

			// Prepare for next frame.
			previousTime = currentTime;
			if (mouse.held) {
				mouse.clickedX = mouse.currentX;
				mouse.clickedY = mouse.currentY;
			}
			mouse.scroll = 0;
		}

	}

	DestroyWindow( hWnd );
	UnregisterClass( wcex.lpszClassName, wcex.hInstance );

	return (int) msg.wParam;
}