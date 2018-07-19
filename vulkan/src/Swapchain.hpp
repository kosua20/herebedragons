//
//  Swapchain.hpp
//  DragonVulkan
//
//  Created by Simon Rodriguez on 14/07/2018.
//  Copyright © 2018 Simon Rodriguez. All rights reserved.
//

#ifndef Swapchain_hpp
#define Swapchain_hpp

#include "common.hpp"
#include "VulkanUtilities.hpp"

class Swapchain {
public:
	
	Swapchain(VkInstance & instance, VkSurfaceKHR & surface, const int width, const int height);
	
	~Swapchain();
	
	VkResult begin(VkRenderPassBeginInfo & infos);
	
	VkResult commit();
	
	void resize(const int width, const int height);
	
	void clean();

	void step(){ currentFrame = (currentFrame + 1) % count; }

	VkCommandBuffer & getCommandBuffer(){ return _commandBuffers[imageIndex]; }
	VkSemaphore & getStartSemaphore(){ return _imageAvailableSemaphores[currentFrame]; }
	VkSemaphore & getEndSemaphore(){ return _renderFinishedSemaphores[currentFrame]; }
	VkFence & getFence(){ return _inFlightFences[currentFrame]; }
	
	VulkanUtilities::SwapchainParameters parameters;
	uint32_t count;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkCommandPool commandPool;
	VkQueue graphicsQueue;
	
	uint32_t imageIndex;
	VkRenderPass finalRenderPass;
	
private:
	
	void setup(const int width, const int height);
	
	void unsetup();
	
	void createMainRenderpass();
	
	VkSurfaceKHR _surface;
	VkQueue _presentQueue;
	std::vector<VkCommandBuffer> _commandBuffers;
	
	VkSwapchainKHR _swapchain;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	std::vector<VkFramebuffer> _swapchainFramebuffers;
	VkImage _depthImage;
	VkDeviceMemory _depthImageMemory;
	VkImageView _depthImageView;
	
	std::vector<VkSemaphore> _imageAvailableSemaphores;
	std::vector<VkSemaphore> _renderFinishedSemaphores;
	std::vector<VkFence> _inFlightFences;
	
	uint32_t currentFrame;
	
};

#endif /* Swapchain_hpp */
