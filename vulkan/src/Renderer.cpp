#include "Renderer.hpp"
#include "VulkanUtilities.hpp"
#include "PipelineUtilities.hpp"
#include "resources/Resources.hpp"

#include <array>



Renderer::~Renderer(){
}

Renderer::Renderer(Swapchain & swapchain, const int width, const int height) : _skybox("cubemap"), _shadowPass(2048, 2048){
	
	const auto & physicalDevice = swapchain.physicalDevice;
	const auto & commandPool = swapchain.commandPool;
	const auto & finalRenderPass = swapchain.finalRenderPass;
	const auto & graphicsQueue = swapchain.graphicsQueue;
	const uint32_t count = swapchain.count;
	_device = swapchain.device;
	
	_lightProj = glm::ortho(-5.0, 5.0, -5.0, 5.0, 0.1, 5.0);
	_lightProj[1][1] *= -1;
	_worldLightDir = glm::normalize(glm::vec4(1.0f,1.0f,1.0f,0.0f));
	glm::mat4 lightView = glm::lookAt(2.0f*glm::vec3(_worldLightDir), glm::vec3(0.0f), glm::vec3(0.0,1.0,0.0));
	_lightViewproj = _lightProj * lightView;
	
	_objects.emplace_back("dragon", 64);
	_objects.back().infos.model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f,0.0f,-0.5f)), glm::vec3(1.2f));
	_objects.emplace_back("suzanne", 8);
	_objects.back().infos.model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.5, 0.0, 0.5)), glm::vec3(0.65f));
	_objects.emplace_back("plane", 32);
	_objects.back().infos.model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0,-0.8,0.0)), glm::vec3(2.75f));
	_skybox.infos.model = glm::scale(glm::mat4(1.0f), glm::vec3(15.0f));
	
	_size = glm::vec2(width, height);
	
	_shadowPass.init(physicalDevice, _device, commandPool,count);
	
	// Create sampler.
	_textureSampler = VulkanUtilities::createSampler(_device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, MAX_MIPMAP_LEVELS);
	
	// Objects setup.
	for(auto & object : _objects){
		object.upload(physicalDevice, _device, commandPool, graphicsQueue);
	}
	_skybox.upload(physicalDevice, _device, commandPool, graphicsQueue);
	
	Skybox::createDescriptorSetLayout(_device, _textureSampler);
	Object::createDescriptorSetLayout(_device, _textureSampler, _shadowPass.depthSampler);
	
	
	/// Pipeline.
	createPipelines(finalRenderPass);
	
	/// Uniform buffers.
	VkDeviceSize bufferSize = VulkanUtilities::nextOffset(sizeof(CameraInfos)) + sizeof(LightInfos);
	_uniformBuffers.resize(count);
	_uniformBuffersMemory.resize(count);
	for (size_t i = 0; i < count; i++) {
		VulkanUtilities::createBuffer(physicalDevice, _device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _uniformBuffers[i], _uniformBuffersMemory[i]);
	}
	
	// Create descriptor pools.
	// 2 pools: one for uniform, one for image+sampler.
	const uint32_t objectsCount = static_cast<uint32_t>(_objects.size()*2)+1;
	std::array<VkDescriptorPoolSize, 3> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = objectsCount*count*4;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = objectsCount*count*4;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	poolSizes[2].descriptorCount = objectsCount*count*4;
	VkDescriptorPoolCreateInfo descPoolInfo = {};
	descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descPoolInfo.pPoolSizes = poolSizes.data();
	descPoolInfo.maxSets = objectsCount*count;
	
	if (vkCreateDescriptorPool(_device, &descPoolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
		std::cerr << "Unable to create descriptor pool." << std::endl;
	}
	
	
	// Create descriptors sets.
	for(auto & object : _objects){
		object.generateDescriptorSets(_device, _shadowPass.descriptorSetLayout, _descriptorPool, _uniformBuffers, _shadowPass.depthViews, count);
	}
	_skybox.generateDescriptorSets(_device, _descriptorPool, _uniformBuffers, count);
	//_shadowPass.generateCommandBuffer(_objects);
	
	
}
void Renderer::createPipelines(const VkRenderPass & finalRenderPass){
	const int pushSize = (16 + 1) * 4;
	PipelineUtilities::createPipeline(_device, "object", finalRenderPass, Object::descriptorSetLayout, _size[0], _size[1], false, VK_CULL_MODE_BACK_BIT, true, true, false, VK_COMPARE_OP_LESS, pushSize, _objectPipelineLayout, _objectPipeline);
	PipelineUtilities::createPipeline(_device, "skybox", finalRenderPass, Skybox::descriptorSetLayout, _size[0], _size[1], false, VK_CULL_MODE_FRONT_BIT, true, false, false, VK_COMPARE_OP_EQUAL, pushSize, _skyboxPipelineLayout, _skyboxPipeline);
}

void Renderer::updateUniforms(const uint32_t index){
	CameraInfos ubo = {};
	ubo.view = _camera.view();
	ubo.proj = _camera.projection();
	ubo.proj[1][1] *= -1; // Flip compared to OpenGL.
	
	LightInfos light = {};
	light.mvp = _lightViewproj;
	light.viewSpaceDir = glm::vec3(glm::normalize(ubo.view * _worldLightDir));
	// Send data.
	void* data;
	vkMapMemory(_device, _uniformBuffersMemory[index], 0, sizeof(ubo)+sizeof(light), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	memcpy(static_cast<char*>(data) + VulkanUtilities::nextOffset(sizeof(CameraInfos)), &light, sizeof(light));
	vkUnmapMemory(_device, _uniformBuffersMemory[index]);
	
}

void Renderer::encode(const VkQueue & graphicsQueue, const uint32_t imageIndex, VkCommandBuffer & finalCommmandBuffer, VkRenderPassBeginInfo & finalPassInfos, const VkSemaphore & startSemaphore, const VkSemaphore & endSemaphore, const VkFence & submissionFence){
	VkDeviceSize offsets[1] = { 0 };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	
	updateUniforms(imageIndex);
	
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	
	vkBeginCommandBuffer(finalCommmandBuffer, &beginInfo);
	
	VkRenderPassBeginInfo shadowInfos = {};
	shadowInfos.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	shadowInfos.renderPass = _shadowPass.renderPass;
	shadowInfos.framebuffer = _shadowPass.frameBuffers[imageIndex];
	shadowInfos.renderArea.offset = { 0, 0 };
	shadowInfos.renderArea.extent = _shadowPass.extent;
	std::array<VkClearValue, 1> clearValuesShadow = {};
	clearValuesShadow[0].depthStencil = {1.0f, 0};
	shadowInfos.clearValueCount = static_cast<uint32_t>(clearValuesShadow.size());
	shadowInfos.pClearValues = clearValuesShadow.data();
	
	vkCmdBeginRenderPass(finalCommmandBuffer, &shadowInfos, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(finalCommmandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPass.pipeline);
	for(auto & object : _objects){
		VkBuffer vertexBuffers[] = {object._vertexBuffer};
		vkCmdBindVertexBuffers(finalCommmandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(finalCommmandBuffer, object._indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(finalCommmandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPass.pipelineLayout, 0, 1, &object.shadowDescriptorSet(imageIndex), 0, nullptr);
		vkCmdPushConstants(finalCommmandBuffer, _shadowPass.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, (16+1)*4, &object.infos);
		vkCmdDrawIndexed(finalCommmandBuffer, object._count, 1, 0, 0, 0);
	}
	vkCmdEndRenderPass(finalCommmandBuffer);
	
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // We don't change queue here.
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = _shadowPass.depthImages[imageIndex];
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	
	vkCmdPipelineBarrier(finalCommmandBuffer,  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	
	
	// ---- Final pass.
	// Complete final pass infos.
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	finalPassInfos.clearValueCount = static_cast<uint32_t>(clearValues.size());
	finalPassInfos.pClearValues = clearValues.data();
	// Submit final pass.
	vkCmdBeginRenderPass(finalCommmandBuffer, &finalPassInfos, VK_SUBPASS_CONTENTS_INLINE);
	
	// Bind and draw.
	vkCmdBindPipeline(finalCommmandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _objectPipeline);
	for(auto & object : _objects){
		VkBuffer vertexBuffers[] = {object._vertexBuffer};
		vkCmdBindVertexBuffers(finalCommmandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(finalCommmandBuffer, object._indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(finalCommmandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _objectPipelineLayout, 0, 1, &object.descriptorSet(imageIndex), 0, nullptr);
		vkCmdPushConstants(finalCommmandBuffer, _objectPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, (16+1)*4, &object.infos);
		vkCmdDrawIndexed(finalCommmandBuffer, object._count, 1, 0, 0, 0);
	}
	vkCmdBindPipeline(finalCommmandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _skyboxPipeline);
	VkBuffer vertexBuffers[] = {_skybox._vertexBuffer};
	vkCmdBindVertexBuffers(finalCommmandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(finalCommmandBuffer, _skybox._indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(finalCommmandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _skyboxPipelineLayout, 0, 1, &_skybox.descriptorSet(imageIndex), 0, nullptr);
	vkCmdPushConstants(finalCommmandBuffer, _skyboxPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, (16+1)*4, &_skybox.infos.model);
	vkCmdDrawIndexed(finalCommmandBuffer, _skybox._count, 1, 0, 0, 0);
	
	// Finish final pass and command buffer.
	vkCmdEndRenderPass(finalCommmandBuffer);
	vkEndCommandBuffer(finalCommmandBuffer);
	// SUbmit the last command buffer.
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &startSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &finalCommmandBuffer;
	// Semaphore for when the command buffer is done, so that we can present the image.
	VkSemaphore signalSemaphores[] = { endSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	// Add the fence so that we don't reuse the command buffer while it's in use.
	vkResetFences(_device, 1, &submissionFence);
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, submissionFence);
}

void Renderer::update(const double deltaTime) {
	_time += deltaTime;
	_camera.update();
	_camera.physics(deltaTime);
	
	_worldLightDir = glm::normalize(glm::vec4(1.0,0.5*sin(_time)+0.6, 1.0,0.0));
	glm::mat4 lightView = glm::lookAt(2.0f*glm::vec3(_worldLightDir), glm::vec3(0.0f), glm::vec3(0.0,1.0,0.0));
	_lightViewproj = _lightProj * lightView;
	
	//TODO: don't rely on arbitrary indexing.
	_objects[1].infos.model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.5,0.0,0.5)), float(fmod(_time, 2*M_PI)), glm::vec3(0.0f,1.0f,0.0f)) , glm::vec3(0.65));
}

void Renderer::resize(VkRenderPass & finalRenderPass, const int width, const int height){
	if(width == _size[0] && height == _size[1]){
		return;
	}
	_camera.ratio(float(width)/float(height));
	_size[0] = width; _size[1] = height;
	/// Recreate Pipeline.
	vkDestroyPipeline(_device, _objectPipeline, nullptr);
	vkDestroyPipelineLayout(_device, _objectPipelineLayout, nullptr);
	vkDestroyPipeline(_device, _skyboxPipeline, nullptr);
	vkDestroyPipelineLayout(_device, _skyboxPipelineLayout, nullptr);
	
	createPipelines(finalRenderPass);
}

void Renderer::clean(){
	vkDestroyPipeline(_device, _objectPipeline, nullptr);
	vkDestroyPipelineLayout(_device, _objectPipelineLayout, nullptr);
	vkDestroyPipeline(_device, _skyboxPipeline, nullptr);
	vkDestroyPipelineLayout(_device, _skyboxPipelineLayout, nullptr);
	
	vkDestroySampler(_device, _textureSampler, nullptr);
	vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
	
	vkDestroyDescriptorSetLayout(_device, Object::descriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(_device, Skybox::descriptorSetLayout, nullptr);

	for (size_t i = 0; i < _uniformBuffers.size(); i++) {
		vkDestroyBuffer(_device, _uniformBuffers[i], nullptr);
		vkFreeMemory(_device, _uniformBuffersMemory[i], nullptr);
	}
	for(auto & object : _objects){
		object.clean(_device);
	}
	_skybox.clean(_device);
	
	_shadowPass.clean(_device);
}

