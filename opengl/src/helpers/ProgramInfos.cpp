#include "ProgramInfos.h"

#include <iostream>

#include "GLUtilities.h"

ProgramInfos::ProgramInfos(){
	_id = 0;
	_uniforms.clear();
}

ProgramInfos::ProgramInfos(const std::string & vertexContent, const std::string & fragmentContent){
	_id = GLUtilities::createProgram(vertexContent, fragmentContent);
	_uniforms.clear();
}

void ProgramInfos::registerUniform(const std::string & name){
	if(_uniforms.count(name) > 0){
		// Already setup, ignore.
		return;
	}
	glUseProgram(_id);
	_uniforms[name] = glGetUniformLocation(_id, name.c_str());
	glUseProgram(0);
}

void ProgramInfos::registerTexture(const std::string & name, int slot){
	if(_uniforms.count(name) > 0){
		// Already setup, ignore.
		return;
	}
	glUseProgram(_id);
	_uniforms[name] = glGetUniformLocation(_id, name.c_str());
	glUniform1i(_uniforms[name], slot);
	glUseProgram(0);
}


ProgramInfos::~ProgramInfos(){
	//glDeleteProgram(id);
}
