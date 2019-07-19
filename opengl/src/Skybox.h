#ifndef Cube_h
#define Cube_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "helpers/ResourcesManager.h"

class Skybox {

public:

	Skybox();

	~Skybox();

	/// Init function
	void init();

	/// Draw function
	void draw(const glm::mat4& view, const glm::mat4& projection) const;

	/// Clean function
	void clean() const;


private:
	
	ProgramInfos _program;
	GLuint _vao;
	GLuint _ebo;
	GLuint _texCubeMap;

};

#endif
