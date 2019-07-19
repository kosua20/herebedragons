#ifndef PointLight_h
#define PointLight_h
#include "Light.h"
#include "../helpers/ResourcesManager.h"

class PointLight : public Light {

public:
	
	PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius, const glm::mat4& projection = glm::mat4(1.0f));
	
	void init(const std::map<std::string, GLuint>& textureIds);
	
	void draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	void drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	void clean() const;
	
	static void loadProgramAndGeometry();
	
private:
	
	float _radius;
	std::vector<GLuint> _textureIds;
	
	ProgramInfos _program;
	
	static ProgramInfos _debugProgram;
	static MeshInfos _debugMesh;
	
};

#endif
