#include <stdio.h>
#include <iostream>
#include <vector>

#include "helpers/ResourcesManager.h"

#include "ScreenQuad.h"

ScreenQuad::ScreenQuad(){}

ScreenQuad::~ScreenQuad(){}

void ScreenQuad::init(GLuint textureId, const std::string & shaderRoot){
	
	// Load the shaders
	_program = Resources::manager().getProgram(shaderRoot);

	// Load geometry.
	loadGeometry();
	
	// Link the texture of the framebuffer for this program.
	_textureIds.push_back(textureId);
	_program.registerTexture("screenTexture", 0);
	//glBindTexture(GL_TEXTURE_2D, _textureIds[0]);
	_program.registerUniform("inverseScreenSize");
	
	checkGLError();
	
}

void ScreenQuad::init(std::map<std::string, GLuint> textureIds, const std::string & shaderRoot){
	
	// Load the shaders
	_program = Resources::manager().getProgram(shaderRoot);
	
	loadGeometry();
	
	// Link the texture of the framebuffer for this program.
	GLint currentTextureSlot = 0;
	for(auto& texture : textureIds){
		_textureIds.push_back(texture.second);
		//glBindTexture(GL_TEXTURE_2D, _textureIds.back());
		_program.registerTexture(texture.first, currentTextureSlot);
		currentTextureSlot += 1;
	}
	
	_program.registerUniform("inverseScreenSize");

	checkGLError();
	
}

void ScreenQuad::loadGeometry(){
	// Load geometry.
	std::vector<float> quadVertices{ -1.0, -1.0,  0.0,
		1.0, -1.0,  0.0,
		-1.0,  1.0,  0.0,
		1.0,  1.0,  0.0
	};
	
	// Array to store the indices of the vertices to use.
	std::vector<unsigned int> quadIndices{0, 1, 2, 2, 1, 3};
	

	// Create an array buffer to host the geometry data.
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// Upload the data to the Array buffer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * quadVertices.size(), &(quadVertices[0]), GL_STATIC_DRAW);
	
	// Generate a vertex array (useful when we add other attributes to the geometry).
	_vao = 0;
	glGenVertexArrays (1, &_vao);
	glBindVertexArray(_vao);
	// The first attribute will be the vertices positions.
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	
	// We load the indices data
	glGenBuffers(1, &_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * quadIndices.size(), &(quadIndices[0]), GL_STATIC_DRAW);
	
	glBindVertexArray(0);
}


void ScreenQuad::draw(const glm::vec2& invScreenSize) const {
	
	// Select the program (and shaders).
	glUseProgram(_program.id());
	
	// Inverse screen size uniform.
	glUniform2fv(_program.uniform("inverseScreenSize"), 1, &(invScreenSize[0]));
	
	// Active screen texture.
	for(GLuint i = 0;i < _textureIds.size(); ++i){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, _textureIds[i]);
	}
	
	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
}



void ScreenQuad::clean() const {
	glDeleteVertexArrays(1, &_vao);
}


