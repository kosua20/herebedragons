//
//  Swapchain.cpp
//  DragonVulkan
//
//  Created by Simon Rodriguez on 14/07/2018.
//  Copyright © 2018 Simon Rodriguez. All rights reserved.
//

#include "Swapchain.hpp"
#include <array>

Swapchain::Swapchain(VkInstance & instance, VkSurfaceKHR & surface, const int width, const int height) {
	_surface = surface;
	currentFrame = 0;
	// Init basic Vulkan objects.
	/// Setup physical device (GPU).
	VulkanUtilities::createPhysicalDevice(instance, surface, physicalDevice);
	/// Setup logical device.
	// Queue setup.
	VulkanUtilities::ActiveQueues queues = VulkanUtilities::getGraphicsQueueFamilyIndex(physicalDevice, surface);
	std::set<int> uniqueQueueFamilies = queues.getIndices();
	// Device features we want.
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	/// Create the logical device.
	VulkanUtilities::createDevice(physicalDevice, uniqueQueueFamilies, deviceFeatures, device);
	/// Get references to the queues.
	vkGetDeviceQueue(device, queues.graphicsQueue, 0, &graphicsQueue);
	vkGetDeviceQueue(device, queues.presentQueue, 0, &_presentQueue);
	
	/// Command pool.
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queues.graphicsQueue;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		std::cerr << "Unable to create command pool." << std::endl;
	}
	
	setup(width, height);
	
	/// Semaphores and fences.
	_imageAvailableSemaphores.resize(count);
	_renderFinishedSemaphores.resize(count);
	_inFlightFences.resize(count);
	
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	
	for(size_t i = 0; i < _inFlightFences.size(); i++) {
		if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
		   vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
		   vkCreateFence(device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
			std::cerr << "Unable to create semaphores and fences." << std::endl;
		}
	}
}



void Swapchain::setup(const int width, const int height) {
	
	VulkanUtilities::ActiveQueues queues = VulkanUtilities::getGraphicsQueueFamilyIndex(physicalDevice, _surface);
	
	
	// Setup swapchain.
	parameters = VulkanUtilities::generateSwapchainParameters(physicalDevice, _surface, width, height);
	VulkanUtilities::createSwapchain(parameters, _surface, device, queues, _swapchain);
	count = parameters.count;
	
	/// Render pass.
	createMainRenderpass();
	
	/// Create depth buffer.
	VkFormat depthFormat = VulkanUtilities::findDepthFormat(physicalDevice);
	VulkanUtilities::createImage(physicalDevice, device, parameters.extent.width, parameters.extent.height, 1, depthFormat , VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, _depthImage, _depthImageMemory);
	_depthImageView = VulkanUtilities::createImageView(device, _depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, false, 1);
	VulkanUtilities::transitionImageLayout(device, commandPool, graphicsQueue, _depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, false, 1);
	
	// Retrieve images in the swap chain.
	vkGetSwapchainImagesKHR(device, _swapchain, &parameters.count, nullptr);
	count = parameters.count;
	_swapchainImages.resize(count);
	std::cout << "Swapchain using " << count << " images."<< std::endl;
	vkGetSwapchainImagesKHR(device, _swapchain, &parameters.count, _swapchainImages.data());
	// Create views for each image.
	_swapchainImageViews.resize(count);
	for(size_t i = 0; i < count; i++) {
		_swapchainImageViews[i] = VulkanUtilities::createImageView(device, _swapchainImages[i], parameters.surface.format, VK_IMAGE_ASPECT_COLOR_BIT, false, 1);
	}
	// Framebuffers.
	_swapchainFramebuffers.resize(count);
	for(size_t i = 0; i < count; ++i){
		std::array<VkImageView, 2> attachments = { _swapchainImageViews[i], _depthImageView };
		
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = finalRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = parameters.extent.width;
		framebufferInfo.height = parameters.extent.height;
		framebufferInfo.layers = 1;
		if(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &_swapchainFramebuffers[i]) != VK_SUCCESS) {
			std::cerr << "Unable to create swap framebuffers." << std::endl;
			
		}
	}
	
	// Command buffers.
	_commandBuffers.resize(count);
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
	if(vkAllocateCommandBuffers(device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
		std::cerr << "Unable to create command buffers." << std::endl;
	}
	
}

void Swapchain::createMainRenderpass(){
	/// Render pass.
	// Depth attachment.
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = VulkanUtilities::findDepthFormat(physicalDevice);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	// Color attachment.
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = parameters.surface.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	// Subpass.
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef; // linked to the layout in fragment shader.
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	
	// Dependencies.
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	
	// Render pass.
	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	
	if(vkCreateRenderPass(device, &renderPassInfo, nullptr, &finalRenderPass) != VK_SUCCESS) {
		std::cerr << "Unable to create render pass." << std::endl;
	}
}

void Swapchain::resize(const int width, const int height){
	if(width == parameters.extent.width && height == parameters.extent.height){
		return;
	}
	// TODO: some semaphores can leave the queue eternally waiting.
	vkDeviceWaitIdle(device);
	unsetup();
	// Recreate swapchain.
	setup(width, height);
}


VkResult Swapchain::begin(VkRenderPassBeginInfo & infos){
	// Wait for the current commands buffer to be done.
	vkWaitForFences(device, 1, &_inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	
	// Acquire image from swap chain.
	// Use a semaphore to know when the image is available.
	VkResult status = vkAcquireNextImageKHR(device, _swapchain, std::numeric_limits<uint64_t>::max(), _imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
	if(status != VK_SUCCESS && status != VK_SUBOPTIMAL_KHR) {
		return status;
	}
	
	// Partially fill infos with internal data.
	infos = {};
	infos.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	infos.renderPass = finalRenderPass;
	infos.framebuffer = _swapchainFramebuffers[imageIndex];
	infos.renderArea.offset = { 0, 0 };
	infos.renderArea.extent = parameters.extent;
	return status;
}

VkResult Swapchain::commit(){
	
	VkSemaphore signalSemaphores[] = { _renderFinishedSemaphores[currentFrame] };
	// Present on swap chain.
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	// Check for the command buffer to be done.
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapChains[] = { _swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	VkResult status = vkQueuePresentKHR(_presentQueue, &presentInfo);
	return status;
}

Swapchain::~Swapchain() {
}

void Swapchain::clean(){
	unsetup();
	for(size_t i = 0; i < _inFlightFences.size(); i++) {
		vkDestroySemaphore(device, _renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, _imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, _inFlightFences[i], nullptr);
	}
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyDevice(device, nullptr);
}

void Swapchain::unsetup() {
	for(size_t i = 0; i < count; i++) {
		vkDestroyFramebuffer(device, _swapchainFramebuffers[i], nullptr);
	}
	vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(_commandBuffers.size()), _commandBuffers.data());
	
	vkDestroyRenderPass(device, finalRenderPass, nullptr);
	vkDestroyImageView(device, _depthImageView, nullptr);
	for(size_t i = 0; i < _swapchainImageViews.size(); i++) {
		vkDestroyImageView(device, _swapchainImageViews[i], nullptr);
	}
	vkDestroyImage(device, _depthImage, nullptr);
	vkFreeMemory(device, _depthImageMemory, nullptr);
	vkDestroySwapchainKHR(device, _swapchain, nullptr);
}
