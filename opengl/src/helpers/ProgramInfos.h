#ifndef ProgramInfos_h
#define ProgramInfos_h

#include <gl3w/gl3w.h>
#include <string>
#include <map>
#include <vector>

class ProgramInfos {
public:
	
	ProgramInfos();
	
	ProgramInfos(const std::string & vertexContent, const std::string & fragmentContent);
	
	~ProgramInfos();
	
	const GLint uniform(const std::string & name) const { return _uniforms.at(name); }
	
	void registerUniform(const std::string & name);
	
	void registerTexture(const std::string & name, int slot);
	
	// To stay coherent with TextureInfos and MeshInfos, we keep the id public.
	
	const GLuint id() const { return _id; }
	
private:
	
	GLuint _id;
	std::map<std::string, GLint> _uniforms;
	
};



#endif
