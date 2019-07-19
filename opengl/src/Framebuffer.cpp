#include <stdio.h>
#include <iostream>

#include "Framebuffer.h"


Framebuffer::Framebuffer(int width, int height, GLuint format, GLuint type, GLuint preciseFormat, GLuint filtering, GLuint wrapping) {
	_width = width;
	_height = height;
	_format = format;
	_type = type;
	_preciseFormat = preciseFormat;
	
	// Create a framebuffer.
	glGenFramebuffers(1, &_id);
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
	
	// Create the texture to store the result.
	glGenTextures(1, &_idColor);
	glBindTexture(GL_TEXTURE_2D, _idColor);
	glTexImage2D(GL_TEXTURE_2D, 0, preciseFormat, _width , _height, 0, format, type, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping);
	if(wrapping == GL_CLAMP_TO_BORDER){
		// Setup the border value for the shadow map
		GLfloat border[] = { 1.0, 1.0, 1.0, 1.0 };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	}
	
	// Link the texture to the first color attachment (ie output) of the framebuffer.
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 ,GL_TEXTURE_2D, _idColor, 0);
	
	// Create the renderbuffer (depth buffer + color(s) buffer(s)).
	glGenRenderbuffers(1, &_idRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, _idRenderbuffer);
	// Setup the depth buffer storage.
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, _width, _height);
	// Link the renderbuffer to the framebuffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _idRenderbuffer);
	
	//Register which color attachments to draw to.
	GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Framebuffer::~Framebuffer(){ clean(); }

void Framebuffer::bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
}

void Framebuffer::unbind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



void Framebuffer::resize(int width, int height){
	_width = width;
	_height = height;
	// Resize the renderbuffer.
	glBindRenderbuffer(GL_RENDERBUFFER, _idRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, _width, _height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	// Resize the texture.
	glBindTexture(GL_TEXTURE_2D, _idColor);
	glTexImage2D(GL_TEXTURE_2D, 0, _preciseFormat, _width, _height, 0, _format, _type, 0);
}

void Framebuffer::resize(glm::vec2 size){
	resize((int)size[0], (int)size[1]);
}

void Framebuffer::clean() const {
	glDeleteRenderbuffers(1, &_idRenderbuffer);
	glDeleteTextures(1, &_idColor);
	glDeleteFramebuffers(1, &_id);
}

