//
//  ShadowPass.hpp
//  DragonVulkan
//
//  Created by Simon Rodriguez on 15/07/2018.
//  Copyright © 2018 Simon Rodriguez. All rights reserved.
//

#ifndef ShadowPass_hpp
#define ShadowPass_hpp

#include "common.hpp"
#include "Object.hpp"

class ShadowPass {
	
public:
	
	ShadowPass(const int width, const int height);
	
	void init(const VkPhysicalDevice & physicalDevice,const VkDevice & device, const VkCommandPool & commandPool, const uint32_t count);
	
	void clean(const VkDevice & device);
	
	static VkDescriptorSetLayout createDescriptorSetLayout(const VkDevice & device);
	static VkDescriptorSetLayout descriptorSetLayout;
	
	glm::vec2 size = glm::vec2(1024.0f, 1024.0f);
	
	VkRenderPass renderPass;
	VkSampler depthSampler;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	
	VkExtent2D extent;
	// Per frame data.
	std::vector<VkFramebuffer> frameBuffers;
	std::vector<VkImage> depthImages;
	std::vector<VkDeviceMemory> depthMemorys;
	std::vector<VkImageView>depthViews;
	std::vector<VkDescriptorImageInfo> descriptors;
};



#endif /* ShadowPass_hpp */
