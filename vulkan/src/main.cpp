#include "common.hpp"

#include <set>
#include <algorithm>
#include <fstream>

#include "Renderer.hpp"
#include "input/Input.hpp"

const int WIDTH = 1280;
const int HEIGHT = 800;

#ifdef DEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

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

/// Entry point.

int main() {

	/// Init GLFW3.
	if(!glfwInit()){
		std::cerr << "Unable to initialize GLFW." << std::endl;
		return 2;
	}
	// Don't create an OpenGL context.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	// Create window.
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Dragon Vulkan", nullptr, nullptr);
	if(!window){
		std::cerr << "Unable to create GLFW window." << std::endl;
		return 2;
	}
	//Get window effective size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	// Debug setup.
	bool debugEnabled = enableValidationLayers;
	// Check if the validation layers are needed and available.
	if(enableValidationLayers && !VulkanUtilities::checkValidationLayerSupport()){
		std::cerr << "Validation layers required and unavailable." << std::endl;
		debugEnabled = false;
	}

	/// Vulkan instance creation.
	VkInstance instance;
	VulkanUtilities::createInstance("Dragon Vulkan", debugEnabled, instance);
	
	/// Surface window setup.
	VkSurfaceKHR surface;
	if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		std::cerr << "Unable to create the surface." << std::endl;
		return 2;
	}
	
	/// Create the swapchain.
	Swapchain swapchain(instance, surface, width, height);
	VkRenderPassBeginInfo finalPassInfos;
	
	/// Create the renderer.	
	Renderer renderer(swapchain, width, height);
	Input::manager().resizeEvent(width, height);
	
	/// Register callbacks.
	glfwSetWindowUserPointer(window, &renderer);
	glfwSetFramebufferSizeCallback(window, resize_callback);
	glfwSetKeyCallback(window,key_callback);					// Pressing a key
	glfwSetMouseButtonCallback(window,mouse_button_callback);	// Clicking the mouse buttons
	glfwSetCursorPosCallback(window,cursor_pos_callback);		// Moving the cursor
	glfwSetScrollCallback(window,scroll_callback);				// Scrolling
	glfwSetWindowIconifyCallback(window, window_iconify_callback);
	
	double timer = glfwGetTime();
	
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
		
		/// Draw frame.
		VkResult status = swapchain.begin(finalPassInfos);
		if (status == VK_SUCCESS || status == VK_SUBOPTIMAL_KHR) {
			// If the init was successful, we can encode our frame and commit it.
			renderer.encode(swapchain.graphicsQueue, swapchain.imageIndex, swapchain.getCommandBuffer(), finalPassInfos, swapchain.getStartSemaphore(), swapchain.getEndSemaphore(), swapchain.getFence());
			status = swapchain.commit();
		}
		if(status == VK_ERROR_OUT_OF_DATE_KHR || status == VK_SUBOPTIMAL_KHR || Input::manager().resized()){
			int width = 0, height = 0;
			while(width == 0 || height == 0) {
				glfwGetFramebufferSize(window, &width, &height);
				glfwWaitEvents();
			}
			Input::manager().resizeEvent(width, height);
			swapchain.resize(width, height);
			renderer.resize(swapchain.finalRenderPass, width, height);
		} else if (status != VK_SUCCESS) {
			std::cerr << "Error while rendering or presenting." << std::endl;
			break;
		}
		swapchain.step();
		
	}

	/// Cleanup.
	vkDeviceWaitIdle(swapchain.device);
	renderer.clean();
	swapchain.clean();
	
	// Clean up instance and surface.
	VulkanUtilities::cleanupDebug(instance);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	// Clean up GLFW.
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
