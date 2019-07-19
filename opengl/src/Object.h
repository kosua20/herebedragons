#ifndef Object_h
#define Object_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "helpers/ResourcesManager.h"


class Object {

public:

	Object();

	~Object();

	/// Init function
	void init(const std::string& meshPath, const std::vector<std::string>& texturesPaths, int materialId);
	
	/// Update function
	void update(const glm::mat4& model);
	
	/// Draw function
	void draw(const glm::mat4& view, const glm::mat4& projection) const;
	
	/// Draw depth function
	void drawDepth(const glm::mat4& lightVP) const;
	
	/// Clean function
	void clean() const;


private:
	
	ProgramInfos _program;
	ProgramInfos _programDepth;
	MeshInfos _mesh;
	
	GLuint _texColor;
	GLuint _texNormal;
	GLuint _texEffects;
	
	glm::mat4 _model;
	
	int _material;
	
};

#endif
