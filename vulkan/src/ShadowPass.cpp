//
//  ShadowPass.cpp
//  DragonVulkan
//
//  Created by Simon Rodriguez on 15/07/2018.
//  Copyright © 2018 Simon Rodriguez. All rights reserved.
//

#include "ShadowPass.hpp"
#include "VulkanUtilities.hpp"
#include "resources/MeshUtilities.hpp"
#include "PipelineUtilities.hpp"


VkDescriptorSetLayout ShadowPass::descriptorSetLayout = VK_NULL_HANDLE;

ShadowPass::ShadowPass(const int width, const int height){
	size = glm::vec2(width, height);
	extent = {static_cast<uint32_t>(size[0]), static_cast<uint32_t>(size[1])};
}

void ShadowPass::init(const VkPhysicalDevice & physicalDevice,const VkDevice & device, const VkCommandPool & commandPool, const  uint32_t count ){
	frameBuffers.resize(count);
	depthImages.resize(count);
	depthMemorys.resize(count);
	depthViews.resize(count);
	descriptors.resize(count);
	// Create a sampler for the shadow map.
	depthSampler = VulkanUtilities::createSampler(device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	// Init shadow pass and framebuffer.
	// For shadow mapping we only need a depth attachment
	for(size_t i = 0; i < count; ++i){
		VulkanUtilities::createImage(physicalDevice, device, size[0], size[1], VK_FORMAT_D32_SFLOAT , VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, depthImages[i], depthMemorys[i]);
		depthViews[i] = VulkanUtilities::createImageView(device, depthImages[i], VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, false);
	}
	
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = VK_FORMAT_D32_SFLOAT;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the depth for the next pass.
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	// Depth buffer ref.
	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0; // No color attachment.
	subpass.pDepthStencilAttachment = &depthReference;
	// Dependencies.
	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	// Creation infos.
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &attachmentDescription;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();
	
	if(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		std::cerr << "Unable to create shadow render pass." << std::endl;
	}
	
	for(size_t i = 0; i < count; ++i){
		// Create the framebuffer.
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &depthViews[i];
		framebufferInfo.width = size[0];
		framebufferInfo.height = size[1];
		framebufferInfo.layers = 1;
		if(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffers[i]) != VK_SUCCESS) {
			std::cerr << "Unable to create shadow map framebuffer." << std::endl;
		}
	}
	
	ShadowPass::createDescriptorSetLayout(device);
	const int pushSize = (16 + 1) * 4;
	PipelineUtilities::createPipeline(device, "shadow", renderPass, descriptorSetLayout, size[0], size[1], true, VK_CULL_MODE_BACK_BIT, true, true, true, VK_COMPARE_OP_LESS, pushSize, pipelineLayout, pipeline);
}

VkDescriptorSetLayout ShadowPass::createDescriptorSetLayout(const VkDevice & device){
	descriptorSetLayout = {};
	// Uniform binding.
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;// binding in 0.
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	// Create the layout (== defining a struct)
	std::array<VkDescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding};
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		std::cerr << "Unable to create uniform descriptor." << std::endl;
	}
	return descriptorSetLayout;
}


void ShadowPass::clean(const VkDevice & device){
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroySampler(device, depthSampler, nullptr);
	for(size_t i = 0; i < frameBuffers.size(); ++i){
		vkDestroyImageView(device, depthViews[i], nullptr);
		vkDestroyImage(device, depthImages[i], nullptr);
		vkFreeMemory(device, depthMemorys[i], nullptr);
		vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
	}
	vkDestroyRenderPass(device, renderPass, nullptr);
}
