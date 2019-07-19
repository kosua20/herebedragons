#include <stdio.h>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>


#include "Object.h"

Object::Object(){}

Object::~Object(){}

void Object::init(const std::string& meshPath, const std::vector<std::string>& texturesPaths, int materialId){
	
	// Load the shaders
	_programDepth = Resources::manager().getProgram("object_depth");
	_program = Resources::manager().getProgram("object_gbuffer");
	
	// Load geometry.
	_mesh = Resources::manager().getMesh(meshPath);
	
	// Load and upload the textures.
	_texColor = Resources::manager().getTexture(texturesPaths[0]).id;
	_texNormal = Resources::manager().getTexture(texturesPaths[1], false).id;
	_texEffects = Resources::manager().getTexture(texturesPaths[2], false).id;
	
	_program.registerTexture("textureColor", 0);
	_program.registerTexture("textureNormal", 1);
	_program.registerTexture("textureEffects", 2);
	_program.registerUniform("mvp");
	_program.registerUniform("mv");
	_program.registerUniform("p");
	_program.registerUniform("normalMatrix");
	_program.registerUniform("materialId");
	
	_material = materialId;
	
	_programDepth.registerUniform("mvp");
	
	checkGLError();
	
}

void Object::update(const glm::mat4& model){
	
	_model = model;
	
}


void Object::draw(const glm::mat4& view, const glm::mat4& projection) const {
	
	// Combine the three matrices.
	glm::mat4 MV = view * _model;
	glm::mat4 MVP = projection * MV;

	// Compute the normal matrix
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));
	// Select the program (and shaders).
	glUseProgram(_program.id());
	
	
	// Upload the MVP matrix.
	glUniformMatrix4fv(_program.uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
	// Upload the MV matrix.
	glUniformMatrix4fv(_program.uniform("mv"), 1, GL_FALSE, &MV[0][0]);
	// Upload the normal matrix.
	glUniformMatrix3fv(_program.uniform("normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
	// Upload the projection matrix.
	glUniformMatrix4fv(_program.uniform("p"), 1, GL_FALSE, &projection[0][0]);
	// Material id.
	glUniform1i(_program.uniform("materialId"), _material);

	// Bind the textures.
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texColor);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _texNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _texEffects);
	
	// Select the geometry.
	glBindVertexArray(_mesh.vId);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mesh.eId);
	glDrawElements(GL_TRIANGLES, _mesh.count, GL_UNSIGNED_INT, (void*)0);

	glBindVertexArray(0);
	glUseProgram(0);
	
	
}


void Object::drawDepth(const glm::mat4& lightVP) const {
	
	// Combine the three matrices.
	glm::mat4 lightMVP = lightVP * _model;
	
	glUseProgram(_programDepth.id());
	
	// Upload the MVP matrix.
	glUniformMatrix4fv(_programDepth.uniform("mvp"), 1, GL_FALSE, &lightMVP[0][0]);
	
	// Select the geometry.
	glBindVertexArray(_mesh.vId);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mesh.eId);
	glDrawElements(GL_TRIANGLES, _mesh.count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
	
}


void Object::clean() const {
	glDeleteVertexArrays(1, &_mesh.vId);
	glDeleteTextures(1, &_texColor);
	glDeleteTextures(1, &_texNormal);
	glDeleteTextures(1, &_texEffects);
	glDeleteProgram(_program.id());
}


