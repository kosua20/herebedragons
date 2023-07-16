#include "common.hpp"

#include "Renderer.hpp"
#include "GPU.hpp"
#include "input/Input.hpp"

#include <glfw3webgpu.h>

/// GLFW callbacks.
static void resize_callback(GLFWwindow* window, int width, int height) {
	Input::manager().resizeEvent(width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	Input::manager().keyPressedEvent(key, action);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
	Input::manager().mousePressedEvent(button, action);
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos){
	Input::manager().mouseMovedEvent(xpos, ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	Input::manager().mouseScrolledEvent(xoffset, yoffset);
}

void window_iconify_callback(GLFWwindow* window, int iconified){
	Input::manager().pauseEvent(iconified);
}

// WebGPU callbacks

template<typename T>
struct AsyncResult {
	T result = 0;
	bool complete = false;
};

void requestAdapterCallback(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* userdata){
	AsyncResult<WGPUAdapter>* dst = (AsyncResult<WGPUAdapter>*)userdata;
	if(status == WGPURequestAdapterStatus_Success){
		dst->result = adapter;
	} else {
		std::cout << "Error retrieving adapter :\"" << (message ? message : "unknown") << "\"." << std::endl;
		dst->result = 0;
	}
	dst->complete = true;
}

void populateRequestLimits(const WGPULimits& referenceLimits, WGPULimits& targetLimits){
	targetLimits.maxTextureDimension1D = 0;					// No 1D texture
	targetLimits.maxTextureDimension2D = 4096;				// Max res in resources is 2048, support large rendertargets
	targetLimits.maxTextureDimension3D = 0;					// No 3D texture
	targetLimits.maxTextureArrayLayers = 6;					// Cubemap
	targetLimits.maxBindGroups = 3;							// Uniforms, textures, static
	targetLimits.maxBindGroupsPlusVertexBuffers = 4;		// + 1 interlaced vertex buffer
	targetLimits.maxBindingsPerBindGroup = 3;				// 2 UBOs, up to 2 textures, 2 samplers + 1 shadowmap
	targetLimits.maxDynamicUniformBuffersPerPipelineLayout = 1; // 1 dynamic UBO (per object data)
	targetLimits.maxDynamicStorageBuffersPerPipelineLayout = 0; // No storage buffer.
	targetLimits.maxSampledTexturesPerShaderStage = 3;		// 2 textures + shadowmap
	targetLimits.maxSamplersPerShaderStage = 2;				// Linear and shadow samplers
	targetLimits.maxStorageBuffersPerShaderStage = 0; 		// No storage buffers
	targetLimits.maxStorageTexturesPerShaderStage = 1; 		// One storage texture (in mipmap.wgsl)
	targetLimits.maxUniformBuffersPerShaderStage = 2; 		// 2 uniform buffers (frame and objects)
	targetLimits.maxUniformBufferBindingSize = 2048; 		// Above 5 objects * ubo min alignment
	targetLimits.maxStorageBufferBindingSize = 0; 			// No storage buffer
	targetLimits.minUniformBufferOffsetAlignment = referenceLimits.minUniformBufferOffsetAlignment;
	targetLimits.minStorageBufferOffsetAlignment = referenceLimits.minStorageBufferOffsetAlignment;
	targetLimits.maxVertexBuffers = 1; 						// Interlaced attributes
	targetLimits.maxBufferSize = 512000;					// Largest vertex buffer
	targetLimits.maxVertexAttributes = 5; 					// Pos, normal, tangent, bitangent, uv
	targetLimits.maxVertexBufferArrayStride = sizeof(Vertex); // Forced stride
	targetLimits.maxInterStageShaderComponents = 22; 		// See object.wgsl
	targetLimits.maxInterStageShaderVariables = 7; 			// Idem
	targetLimits.maxColorAttachments = 1; 					// No MRT
	targetLimits.maxColorAttachmentBytesPerSample = 4; 		// Draw to rgba8 swapchain.
	targetLimits.maxComputeWorkgroupStorageSize = 0; 		// No local storage
	targetLimits.maxComputeInvocationsPerWorkgroup = 64; 	// 8x8 in mipmap generation
	targetLimits.maxComputeWorkgroupSizeX = 8; 				// See mipmap.wgsl
	targetLimits.maxComputeWorkgroupSizeY = 8; 				// Idem
	targetLimits.maxComputeWorkgroupSizeZ = 1; 				// Workload is 2D
	targetLimits.maxComputeWorkgroupsPerDimension = 256; 	// 8x8 compute on a 2048 image to mipmap.
}

void requestDeviceCallback(WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* userdata){
	AsyncResult<WGPUDevice>* dst = (AsyncResult<WGPUDevice>*)userdata;
	if(status == WGPURequestDeviceStatus_Success){
		dst->result = device;
	} else {
		std::cout << "Error retrieving device :\"" << (message ? message : "unknown") << "\"." << std::endl;
		dst->result = 0;
	}
	dst->complete = true;
}

void errorCallback(WGPUErrorType type, char const* message, void* userdata){
	static const char* typeStrs[] = {
		"NoError",
		"Validation",
		"OutOfMemory",
		"Internal",
		"Unknown",
		"DeviceLost"
	};

	std::cout << "WGPU device error: type: " << (type <= WGPUErrorType_DeviceLost ? typeStrs[type] : "unknown") << ", info: " << (message ? message : "none" ) << std::endl;
}

WGPUSwapChain createSwapchain(WGPUDevice device, WGPUSurface surface, unsigned int w, unsigned int h, WGPUTextureFormat& selectedFormat){
	WGPUSwapChainDescriptor swapchainDesc{};
	swapchainDesc.nextInChain = nullptr;
	swapchainDesc.width = w;
	swapchainDesc.height = h;
	#ifdef SUPPORT_PREFERRED_SWAP_FORMAT
	swapchainDesc.format = wgpuSurfaceGetPreferredFormat(surface, adapter); // Use the format of the surface.
	#else
	swapchainDesc.format = WGPUTextureFormat_BGRA8Unorm;
	#endif
	swapchainDesc.usage = WGPUTextureUsage_RenderAttachment;
	swapchainDesc.presentMode = WGPUPresentMode_Fifo;
	WGPUSwapChain swapchain = wgpuDeviceCreateSwapChain(device, surface, &swapchainDesc);

	selectedFormat = swapchainDesc.format;
	return swapchain;
}

int main(int argc, char** argv){
	// Init glfw.
	if(!glfwInit()){
		std::cerr << "Could not init GLFW" << std::endl;
		return -2;
	}
	// Don't create an OpenGL context.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	// Create window
	GLFWwindow* window = glfwCreateWindow(720, 480, "Dragon WebGPU", nullptr, nullptr);
	if(!window){
		std::cerr << "Could not create GLFW window" << std::endl;
		glfwTerminate();
		return -3;
	}

	//Get window effective size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	// Create the instance.
	WGPUInstanceDescriptor instanceDesc {};
	instanceDesc.nextInChain = nullptr;
	WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
	if(!instance){
		std::cerr << "Could not retrieve WebGPU instance" << std::endl;
		return -1;
	}

	// Query the surface from the GLFW window via helper.
	WGPUSurface surface = glfwGetWGPUSurface(instance, window);

	/// Adapter
	WGPURequestAdapterOptions adapOptions{};
	adapOptions.nextInChain = nullptr;
	adapOptions.compatibleSurface = surface;
	AsyncResult<WGPUAdapter> adapterResult;
	wgpuInstanceRequestAdapter(instance, &adapOptions, requestAdapterCallback, &adapterResult);
	assert(adapterResult.complete);
	WGPUAdapter adapter = adapterResult.result;
	if(!adapter){
		std::cerr << "Could not retrieve WebGPU adapter" << std::endl;
		return -1;
	}

	// Query the adapter limits.
	WGPUSupportedLimits supportedLimits{};
	wgpuAdapterGetLimits(adapter, &supportedLimits);
	const size_t uboAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
	// Setup proper limits
	WGPURequiredLimits requestedLimits{nullptr};
	requestedLimits.nextInChain = nullptr;
	populateRequestLimits(supportedLimits.limits, requestedLimits.limits);

	/// Device
	WGPUDeviceDescriptor deviceDesc{};
	deviceDesc.nextInChain = nullptr;
	deviceDesc.label = "Dragon device";
	deviceDesc.requiredFeaturesCount = 0;
	deviceDesc.requiredFeatures = nullptr;

	deviceDesc.requiredLimits = &requestedLimits;
	deviceDesc.defaultQueue = {};
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "Main queue";

	AsyncResult<WGPUDevice> deviceResult;
	wgpuAdapterRequestDevice(adapter, &deviceDesc, requestDeviceCallback, &deviceResult);
	assert(deviceResult.complete);
	WGPUDevice device = deviceResult.result;
	if(!device){
		std::cerr << "Could not retrieve WebGPU device" << std::endl;
		return -1;
	}

	wgpuDeviceSetUncapturedErrorCallback(device, errorCallback, nullptr);
	WGPUQueue queue = wgpuDeviceGetQueue(device);
	if(!queue){
		std::cerr << "Could not retrieve WebGPU queue" << std::endl;
		return -1;
	}

	/// Create the swapchain.
	WGPUTextureFormat selectedFormat;
	WGPUSwapChain swapchain = createSwapchain(device, surface, width, height, selectedFormat);
	if(!swapchain){
		std::cerr << "Could not retrieve WebGPU swapchain" << std::endl;
		return -1;
	}
	GPU::init(device, uboAlignment);

	Input::manager().resizeEvent(width, height);

	/// Register callbacks.
	glfwSetFramebufferSizeCallback(window, resize_callback);
	glfwSetKeyCallback(window, key_callback);					// Pressing a key
	glfwSetMouseButtonCallback(window, mouse_button_callback);	// Clicking the mouse buttons
	glfwSetCursorPosCallback(window, cursor_pos_callback);		// Moving the cursor
	glfwSetScrollCallback(window, scroll_callback);				// Scrolling
	glfwSetWindowIconifyCallback(window, window_iconify_callback);

	double timer = glfwGetTime();

	/// Create the renderer.
	Renderer renderer;
	renderer.resize(device, width, height);
	renderer.upload(device, queue, selectedFormat);

	/// Main loop.
	while(!glfwWindowShouldClose(window)){
		if(Input::manager().paused()){
			glfwWaitEvents();
			continue;
		}
		Input::manager().update();
		// Compute the time elapsed since last frame
		double currentTime = glfwGetTime();
		double frameTime = currentTime - timer;
		timer = currentTime;
		renderer.update(frameTime);

		if(Input::manager().resized()){
			glm::vec2 dims = Input::manager().size();
			dims = glm::max(dims, glm::vec2(4.f));
			unsigned int w = (unsigned int)dims[0];
			unsigned int h = (unsigned int)dims[1];
			// We need to recreate the swapchain.
			wgpuSwapChainRelease(swapchain);
			WGPUTextureFormat newFormat;
			swapchain = createSwapchain(device, surface, w, h, newFormat);
			assert(newFormat == selectedFormat);
			if(!swapchain || (newFormat != selectedFormat)){
				std::cerr << "Could not retrieve swapchain after resize" << std::endl;
				break;
			}
			renderer.resize(device, w, h);
		}
		/// Draw frame.

		// Query texture from swapchain
		WGPUTextureView backbuffer = wgpuSwapChainGetCurrentTextureView(swapchain);
		if(!backbuffer){
			std::cerr << "Could not retrieve swapchain next backbuffer" << std::endl;
			break;
		}

		// Create an encoder.
		WGPUCommandEncoderDescriptor encoderDesc{};
		encoderDesc.nextInChain = nullptr;
		encoderDesc.label = "Main encoder";
		WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

		renderer.draw(queue, encoder, backbuffer);

		// Generate filled command buffer and submit it to the queue.
		WGPUCommandBufferDescriptor commandBufferDesc{};
		commandBufferDesc.nextInChain = nullptr;
		commandBufferDesc.label = "Main command buffer";
		WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);
		wgpuQueueSubmit(queue, 1, &commandBuffer);

		// Done with the backbuffer, return it to the swapchain for presentation.
		wgpuTextureViewRelease(backbuffer);
		wgpuSwapChainPresent(swapchain);

		wgpuCommandBufferRelease(commandBuffer);
		wgpuCommandEncoderRelease(encoder);

	}

	// Cleanup everything.
	renderer.clean();

	GPU::clean();
	wgpuSwapChainRelease(swapchain);
	wgpuDeviceRelease(device);
	wgpuAdapterRelease(adapter);
	wgpuSurfaceRelease(surface);
	wgpuInstanceRelease(instance);
	// Clean up GLFW.
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
