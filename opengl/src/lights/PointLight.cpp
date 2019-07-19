#include <stdio.h>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>


#include "PointLight.h"


PointLight::PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius, const glm::mat4& projection ) : Light(worldPosition, color, projection) {
	_radius = radius;
}


void PointLight::loadProgramAndGeometry(){
	
	_debugProgram = Resources::manager().getProgram("point_light_debug");
	_debugProgram.registerUniform("radius");
	_debugProgram.registerUniform("lightWorldPosition");
	_debugProgram.registerUniform("mvp");
	_debugProgram.registerUniform("lightColor");
	// Load geometry.
	_debugMesh = Resources::manager().getMesh("sphere");
	
	checkGLError();
}

void PointLight::init(const std::map<std::string, GLuint>& textureIds){
	_program = Resources::manager().getProgram("point_light");
	//glUseProgram(_program.id());
	checkGLError();
	GLint currentTextureSlot = 0;
	for(auto& texture : textureIds){
		_textureIds.push_back(texture.second);
		//glBindTexture(GL_TEXTURE_2D, _textureIds.back());
		_program.registerTexture(texture.first, currentTextureSlot);
		currentTextureSlot += 1;
	}
	
	_program.registerUniform("lightColor");
	_program.registerUniform("projectionMatrix");
	_program.registerUniform("inverseScreenSize");
	_program.registerUniform("radius");
	_program.registerUniform("lightWorldPosition");
	_program.registerUniform("mvp");
	_program.registerUniform("lightPosition");
	
	
	checkGLError();
}


void PointLight::draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	// Store the four variable coefficients of the projection matrix.
	glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	glm::vec3 lightPositionViewSpace = glm::vec3(viewMatrix * glm::vec4(_local, 1.0f));
	glm::mat4 vp = projectionMatrix * viewMatrix;
	
	glUseProgram(_program.id());
	
	// For the vertex shader
	glUniform1f(_program.uniform("radius"),  _radius);
	glUniform3fv(_program.uniform("lightWorldPosition"), 1, &_local[0]);
	glUniformMatrix4fv(_program.uniform("mvp"), 1, GL_FALSE, &vp[0][0]);
	glUniform3fv(_program.uniform("lightPosition"), 1,  &lightPositionViewSpace[0]);
	glUniform3fv(_program.uniform("lightColor"), 1,  &_color[0]);
	// Projection parameter for position reconstruction.
	glUniform4fv(_program.uniform("projectionMatrix"), 1, &(projectionVector[0]));
	// Inverse screen size uniform.
	glUniform2fv(_program.uniform("inverseScreenSize"), 1, &(invScreenSize[0]));
	
	// Active screen texture.
	for(GLuint i = 0;i < _textureIds.size(); ++i){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, _textureIds[i]);
	}
	
	
	// Select the geometry.
	glBindVertexArray(_debugMesh.vId);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _debugMesh.eId);
	glDrawElements(GL_TRIANGLES, _debugMesh.count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
	
}

void PointLight::drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	glm::mat4 vp = projectionMatrix * viewMatrix;
	
	glUseProgram(_debugProgram.id());
	
	// For the vertex shader
	glUniform1f(_debugProgram.uniform("radius"),  0.1*_radius);
	glUniform3fv(_debugProgram.uniform("lightWorldPosition"), 1, &_local[0]);
	glUniformMatrix4fv(_debugProgram.uniform("mvp"), 1, GL_FALSE, &vp[0][0]);
	glUniform3fv(_debugProgram.uniform("lightColor"), 1,  &_color[0]);
	
	// Select the geometry.
	glBindVertexArray(_debugMesh.vId);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _debugMesh.eId);
	glDrawElements(GL_TRIANGLES, _debugMesh.count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
}


void PointLight::clean() const {
	
}

ProgramInfos PointLight::_debugProgram;
MeshInfos PointLight::_debugMesh;




