//
//  Object.hpp
//  DragonVulkan
//
//  Created by Simon Rodriguez on 14/07/2018.
//  Copyright © 2018 Simon Rodriguez. All rights reserved.
//

#ifndef Object_hpp
#define Object_hpp

#include "common.hpp"
#include "resources/MeshUtilities.hpp"

class Object {
public:
	
	Object(const std::string & name, const float shininess);
	
	~Object();
	
	void upload(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & graphicsQueue);

	void clean(VkDevice & device);
	
	void generateDescriptorSets(const VkDevice & device, const VkDescriptorSetLayout & shadowLayout, const VkDescriptorPool & pool, const std::vector<VkBuffer> & constants, const std::vector<VkImageView> & shadowMaps, const int count);
	
	const VkDescriptorSet & descriptorSet(const int i){ return _descriptorSets[i]; }
	const VkDescriptorSet & shadowDescriptorSet(const int i) const { return _shadowDescriptorSets[i]; }
	
	VkBuffer _vertexBuffer;
	VkBuffer _indexBuffer;
	uint32_t _count;
	ObjectInfos infos;
	
	static VkDescriptorSetLayout createDescriptorSetLayout(const VkDevice & device, const VkSampler & sampler, const VkSampler & shadowSampler);
	static VkDescriptorSetLayout descriptorSetLayout;
	
private:
	std::string _name;
	
	
	VkImage _textureColorImage;
	VkImage _textureNormalImage;
	VkImageView _textureColorView;
	VkImageView _textureNormalView;
	
	VkDeviceMemory _vertexBufferMemory;
	VkDeviceMemory _indexBufferMemory;
	VkDeviceMemory _textureColorMemory;
	VkDeviceMemory _textureNormalMemory;
	std::vector<VkDescriptorSet> _descriptorSets;
	std::vector<VkDescriptorSet> _shadowDescriptorSets;
};

#endif /* Object_hpp */
