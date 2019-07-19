#include <stdio.h>
#include <iostream>
#include <algorithm>
#include "Gbuffer.h"


Gbuffer::Gbuffer(int width, int height) {
	_width = width;
	_height = height;
	
	// Create a framebuffer.
	glGenFramebuffers(1, &_id);
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
	
	// Create the textures.
	// Albedo
	GLuint albedoId, normalId, depthId, effectsId;
	
	glGenTextures(1, &albedoId);
	glBindTexture(GL_TEXTURE_2D, albedoId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _width , _height, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedoId, 0);
	_textureIds[TextureType::Albedo] = albedoId;
	
	glGenTextures(1, &normalId);
	glBindTexture(GL_TEXTURE_2D, normalId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, _width , _height, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalId, 0);
	_textureIds[TextureType::Normal] = normalId;
	
	glGenTextures(1, &effectsId);
	glBindTexture(GL_TEXTURE_2D, effectsId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width , _height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, effectsId, 0);
	_textureIds[TextureType::Effects] = effectsId;
	
	glGenTextures(1, &depthId);
	glBindTexture(GL_TEXTURE_2D, depthId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, _width , _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthId, 0);
	_textureIds[TextureType::Depth] = depthId;
	
	
	//Register which color attachments to draw to.
	GLenum drawBuffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	glDrawBuffers(3, drawBuffers);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Gbuffer::~Gbuffer(){ clean(); }

void Gbuffer::bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
}

void Gbuffer::unbind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

const std::map<std::string, GLuint> Gbuffer::textureIds(const std::vector<TextureType>& included) const {
	
	bool includeAll = (included.size() == 0);
	
	std::map<std::string, GLuint> texs;
	for(auto& tex : _textureIds){
		
		// Skip if not included
		if(!includeAll && (std::find(included.cbegin(), included.cend(), tex.first) == included.cend())){
			continue;
		}
		std::string name = "albedoTexture";
		if(tex.first == TextureType::Normal){
			name = "normalTexture";
		} else if (tex.first == TextureType::Depth){
			name = "depthTexture";
		} else if (tex.first == TextureType::Effects){
			name = "effectsTexture";
		}
		
		texs[name] = tex.second;
		
	}
	return texs;
}


void Gbuffer::resize(int width, int height){
	_width = width;
	_height = height;
	
	
	// Resize the texture.
	glBindTexture(GL_TEXTURE_2D, _textureIds[TextureType::Albedo]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _width , _height, 0, GL_RGBA, GL_FLOAT, 0);
	
	glBindTexture(GL_TEXTURE_2D, _textureIds[TextureType::Normal]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, _width , _height, 0, GL_RGB, GL_FLOAT, 0);
	
	glBindTexture(GL_TEXTURE_2D, _textureIds[TextureType::Effects]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width , _height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	
	glBindTexture(GL_TEXTURE_2D, _textureIds[TextureType::Depth]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, _width , _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
}

void Gbuffer::resize(glm::vec2 size){
	resize((int)size[0], (int)size[1]);
}

void Gbuffer::clean() const {
	for(auto& tex : _textureIds){
		glDeleteTextures(1, &(tex.second));
	}
	glDeleteFramebuffers(1, &_id);
}

