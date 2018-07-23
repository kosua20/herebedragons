//
//  Object.cpp
//  DragonVulkan
//
//  Created by Simon Rodriguez on 14/07/2018.
//  Copyright © 2018 Simon Rodriguez. All rights reserved.
//

#include "Object.hpp"
#include "VulkanUtilities.hpp"
#include "resources/Resources.hpp"

VkDescriptorSetLayout Object::descriptorSetLayout = VK_NULL_HANDLE;

Object::~Object() {  }

Object::Object(const std::string &name, const float shininess) {
	_name = name;
	infos.model = glm::mat4(1.0f);
	infos.shininess = shininess;
}

void Object::upload(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & graphicsQueue) {
	
	// Mesh.
	Mesh mesh;
	const std::string meshPath = "resources/meshes/" + _name + ".obj";
	MeshUtilities::loadObj(meshPath, mesh, MeshUtilities::Indexed);
	MeshUtilities::centerAndUnitMesh(mesh);
	MeshUtilities::computeTangentsAndBinormals(mesh);
	
	/// Buffers.
	VulkanUtilities::setupBuffers(physicalDevice, device, commandPool, graphicsQueue, mesh, _vertexBuffer, _vertexBufferMemory, _indexBuffer, _indexBufferMemory);
	
	_count  = static_cast<uint32_t>(mesh.indices.size());
	
	/// Textures.
	unsigned int texWidth, texHeight, texChannels;
	void* image;
	int rett = Resources::loadImage("resources/textures/" + _name + "_texture_color.png", texWidth, texHeight, texChannels, &image, true);
	if(rett != 0){ std::cerr << "Error loading color image." << std::endl; }
	VulkanUtilities::createTexture(image, texWidth, texHeight, false, MAX_MIPMAP_LEVELS, physicalDevice, device, commandPool, graphicsQueue, _textureColorImage, _textureColorMemory, _textureColorView);
	free(image);
	
	rett = Resources::loadImage("resources/textures/" + _name + "_texture_normal.png", texWidth, texHeight, texChannels, &image, true);
	if(rett != 0){ std::cerr << "Error loading normal image." << std::endl; }
	VulkanUtilities::createTexture(image, texWidth, texHeight, false, MAX_MIPMAP_LEVELS, physicalDevice, device, commandPool, graphicsQueue, _textureNormalImage, _textureNormalMemory, _textureNormalView);
	free(image);
}

void Object::generateDescriptorSets(const VkDevice & device, const VkDescriptorSetLayout & shadowLayout, const VkDescriptorPool & pool, const std::vector<VkBuffer> & constants, const std::vector<VkImageView> & shadowMaps, int count){
	
	_descriptorSets.resize(count);
	_shadowDescriptorSets.resize(count);
	
	for (size_t i = 0; i < _descriptorSets.size(); i++) {
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;
		
		if (vkAllocateDescriptorSets(device, &allocInfo, &_descriptorSets[i]) != VK_SUCCESS) {
			std::cerr << "Unable to create descriptor sets." << std::endl;
		}
		
		VkDescriptorBufferInfo bufferCameraInfo = {};
		bufferCameraInfo.buffer = constants[i];
		bufferCameraInfo.offset = 0;
		bufferCameraInfo.range = sizeof(CameraInfos);
		
		VkDescriptorBufferInfo bufferLightInfo = {};
		bufferLightInfo.buffer = constants[i];
		bufferLightInfo.offset = VulkanUtilities::nextOffset(sizeof(CameraInfos));
		bufferLightInfo.range = sizeof(LightInfos);
		
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = _textureColorView;
		
		VkDescriptorImageInfo imageNormalInfo = {};
		imageNormalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageNormalInfo.imageView = _textureNormalView;
		
		VkDescriptorImageInfo imageShadowInfo = {};
		imageShadowInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageShadowInfo.imageView = shadowMaps[i];
		
		std::array<VkWriteDescriptorSet, 5> descriptorWrites = {};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = _descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferCameraInfo;
		
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = _descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;
		
		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = _descriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &imageNormalInfo;
		
		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = _descriptorSets[i];
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pBufferInfo = &bufferLightInfo;
		
		descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[4].dstSet = _descriptorSets[i];
		descriptorWrites[4].dstBinding = 4;
		descriptorWrites[4].dstArrayElement = 0;
		descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[4].descriptorCount = 1;
		descriptorWrites[4].pImageInfo = &imageShadowInfo;
		
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		
		// Shadow descriptor.
		
		VkDescriptorSetAllocateInfo allocInfoShadow = {};
		allocInfoShadow.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfoShadow.descriptorPool = pool;
		allocInfoShadow.descriptorSetCount = 1;
		allocInfoShadow.pSetLayouts = &shadowLayout;
		
		if (vkAllocateDescriptorSets(device, &allocInfoShadow, &_shadowDescriptorSets[i]) != VK_SUCCESS) {
			std::cerr << "Unable to create descriptor sets." << std::endl;
		}
		
		
		std::array<VkWriteDescriptorSet, 1> shadowDescriptorWrites = {};
		shadowDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		shadowDescriptorWrites[0].dstSet = _shadowDescriptorSets[i];
		shadowDescriptorWrites[0].dstBinding = 0;
		shadowDescriptorWrites[0].dstArrayElement = 0;
		shadowDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		shadowDescriptorWrites[0].descriptorCount = 1;
		shadowDescriptorWrites[0].pBufferInfo = &bufferLightInfo;
		
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(shadowDescriptorWrites.size()), shadowDescriptorWrites.data(), 0, nullptr);
	}
	
	
	
}

void Object::clean(VkDevice & device){
	vkDestroyImageView(device, _textureColorView, nullptr);
	vkDestroyImage(device, _textureColorImage, nullptr);
	vkFreeMemory(device, _textureColorMemory, nullptr);
	vkDestroyImageView(device, _textureNormalView, nullptr);
	vkDestroyImage(device, _textureNormalImage, nullptr);
	vkFreeMemory(device, _textureNormalMemory, nullptr);
	
	vkDestroyBuffer(device, _vertexBuffer, nullptr);
	vkFreeMemory(device, _vertexBufferMemory, nullptr);
	vkDestroyBuffer(device, _indexBuffer, nullptr);
	vkFreeMemory(device, _indexBufferMemory, nullptr);
}

VkDescriptorSetLayout Object::createDescriptorSetLayout(const VkDevice & device, const VkSampler & sampler, const VkSampler & shadowSampler){
	descriptorSetLayout = {};
	// Descriptor layout for standard objects.
	// Uniform binding.
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;// binding in 0.
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	// Image+sampler binding.
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = &sampler;
	// Image+sampler binding.
	VkDescriptorSetLayoutBinding samplerLayoutNormalBinding = {};
	samplerLayoutNormalBinding.binding = 2;
	samplerLayoutNormalBinding.descriptorCount = 1;
	samplerLayoutNormalBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutNormalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutNormalBinding.pImmutableSamplers = &sampler;
	
	VkDescriptorSetLayoutBinding uboLayoutLightBinding = {};
	uboLayoutLightBinding.binding = 3;
	uboLayoutLightBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutLightBinding.descriptorCount = 1;
	uboLayoutLightBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
	
	VkDescriptorSetLayoutBinding samplerLayoutShadowmapBinding = {};
	samplerLayoutShadowmapBinding.binding = 4;
	samplerLayoutShadowmapBinding.descriptorCount = 1;
	samplerLayoutShadowmapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutShadowmapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutShadowmapBinding.pImmutableSamplers = &shadowSampler;
	// Create the layout (== defining a struct)
	std::array<VkDescriptorSetLayoutBinding, 5> bindings = {uboLayoutBinding, uboLayoutLightBinding, samplerLayoutBinding, samplerLayoutNormalBinding, samplerLayoutShadowmapBinding};
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		std::cerr << "Unable to create uniform descriptor." << std::endl;
	}
	return descriptorSetLayout;
}

