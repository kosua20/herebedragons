//
//  Skybox.cpp
//  DragonVulkan
//
//  Created by Simon Rodriguez on 14/07/2018.
//  Copyright © 2018 Simon Rodriguez. All rights reserved.
//

#include "Skybox.hpp"
#include "VulkanUtilities.hpp"
#include "resources/Resources.hpp"

VkDescriptorSetLayout Skybox::descriptorSetLayout = VK_NULL_HANDLE;

Skybox::~Skybox() {  }

Skybox::Skybox(const std::string &name) {
	_name = name;
	infos.model = glm::mat4(1.0f);
	infos.shininess = 0;
}

void Skybox::upload(const VkPhysicalDevice & physicalDevice, const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & graphicsQueue) {
	
	// Mesh.
	Mesh mesh;
	const std::string meshPath = "resources/meshes/cubemap.obj";
	MeshUtilities::loadObj(meshPath, mesh, MeshUtilities::Indexed);
	MeshUtilities::centerAndUnitMesh(mesh);
	MeshUtilities::computeTangentsAndBinormals(mesh);
	
	/// Buffers.
	VulkanUtilities::setupBuffers(physicalDevice, device, commandPool, graphicsQueue, mesh, _vertexBuffer, _vertexBufferMemory, _indexBuffer, _indexBufferMemory);
	
	_count  = static_cast<uint32_t>(mesh.indices.size());
	
	/// Textures.
	unsigned int texWidth, texHeight, texChannels;
	char* images[6];
	const std::vector<std::string> suffixes = {"r", "l", "u", "d", "b", "f"};
	int rett = 0;
	for(size_t i = 0; i < 6; ++i){
		rett = Resources::loadImage("resources/textures/" + _name + "_" + suffixes[i] + ".png", texWidth, texHeight, texChannels, (void**)&images[i], false);
	}
	const int layerSize = texWidth*texHeight*texChannels;
	char* mergedImages = (char*)malloc(6*layerSize);
	for(size_t i = 0; i < 6; ++i){
		memcpy(mergedImages + i*layerSize, images[i], layerSize);
	}
	VulkanUtilities::createTexture(mergedImages, texWidth, texHeight, true, MAX_MIPMAP_LEVELS, physicalDevice, device, commandPool, graphicsQueue, _textureCubeImage, _textureCubeMemory, _textureCubeView);
	
	// Cleaning.
	for(size_t i = 0; i < 6; ++i){
		free(images[i]);
	}
	free(mergedImages);
}

void Skybox::generateDescriptorSets(const VkDevice & device, const VkDescriptorPool & pool, const std::vector<VkBuffer> & constants, const int count){
	
	_descriptorSets.resize(count);
	
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
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = _textureCubeView;
		
		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
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
		
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Skybox::clean(VkDevice & device){
	vkDestroyImageView(device, _textureCubeView, nullptr);
	vkDestroyImage(device, _textureCubeImage, nullptr);
	vkFreeMemory(device, _textureCubeMemory, nullptr);
	
	vkDestroyBuffer(device, _vertexBuffer, nullptr);
	vkFreeMemory(device, _vertexBufferMemory, nullptr);
	vkDestroyBuffer(device, _indexBuffer, nullptr);
	vkFreeMemory(device, _indexBufferMemory, nullptr);
}


VkDescriptorSetLayout Skybox::createDescriptorSetLayout(const VkDevice & device, const VkSampler & sampler){
	descriptorSetLayout = {};
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
	
	// Create the layout (== defining a struct)
	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		std::cerr << "Unable to create uniform descriptor." << std::endl;
	}
	return descriptorSetLayout;
}

