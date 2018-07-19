//
//  Skybox.hpp
//  DragonVulkan
//
//  Created by Simon Rodriguez on 14/07/2018.
//  Copyright © 2018 Simon Rodriguez. All rights reserved.
//

#ifndef _hpp
#define Skybox_hpp

#include "common.hpp"
#include "resources/MeshUtilities.hpp"

class Skybox {
public:
	
	Skybox(const std::string & textureName);
	
	~Skybox();
	
	void upload(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & graphicsQueue);

	void clean(VkDevice & device);
	
	void generateDescriptorSets(const VkDevice & device, const VkDescriptorPool & pool, const std::vector<VkBuffer> & constants, const int count);
	
	const VkDescriptorSet & descriptorSet(const int i){ return _descriptorSets[i]; }
	
	
	VkBuffer _vertexBuffer;
	VkBuffer _indexBuffer;
	uint32_t _count;
	ObjectInfos infos;
	
	static VkDescriptorSetLayout createDescriptorSetLayout(const VkDevice & device, const VkSampler & sampler);
	static VkDescriptorSetLayout descriptorSetLayout;
	
private:
	
	
	
	std::string _name;
	
	VkImage _textureCubeImage;
	VkImageView _textureCubeView;
	
	VkDeviceMemory _vertexBufferMemory;
	VkDeviceMemory _indexBufferMemory;
	VkDeviceMemory _textureCubeMemory;
	std::vector<VkDescriptorSet> _descriptorSets;
	
	
	
};

#endif /* Skybox_hpp */
