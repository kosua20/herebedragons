//
//  PipelineUtilities.hpp
//  DragonVulkan
//
//  Created by Simon Rodriguez on 15/07/2018.
//  Copyright © 2018 Simon Rodriguez. All rights reserved.
//

#ifndef PipelineUtilities_hpp
#define PipelineUtilities_hpp

#include "common.hpp"

class PipelineUtilities {
public:
	static void createPipeline(const VkDevice & device, const std::string & moduleName, const VkRenderPass & renderPass,const VkDescriptorSetLayout & descriptorSetLayout, const uint32_t width, const uint32_t height, const bool vertexOnly, const VkCullModeFlags cullMode, const bool depthTest, const bool depthWrite, const bool depthBias, const VkCompareOp compareOp, const int pushSize, VkPipelineLayout & pipelineLayout, VkPipeline & pipeline);
};

#endif /* PipelineUtilities_hpp */
