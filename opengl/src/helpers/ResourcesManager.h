#ifndef ResourcesManager_h
#define ResourcesManager_h

#include <gl3w/gl3w.h>
#include <string>
#include <vector>
#include <map>

#include "GLUtilities.h"
#include "ProgramInfos.h"

class Resources {
public:
	
	const ProgramInfos getProgram(const std::string & name);
	
	const MeshInfos getMesh(const std::string & name);
	
	const TextureInfos getTexture(const std::string & name, bool srgb = true);
	
	const TextureInfos getCubemap(const std::string & name, bool srgb = true);
	
	const std::string getTextFile(const std::string & filename);
	
private:
	
	enum ShaderType {
		Vertex, Fragment
	};
	
	const std::string getShader(const std::string & name, const ShaderType & type);
	
	void parseDirectory(const std::string & directoryPath);
	
	const std::string getImagePath(const std::string & name);
	
	const std::vector<std::string> getCubemapPaths(const std::string & name);
	
	static std::string loadStringFromFile(const std::string & filename);
	
	
	const std::string & _rootPath;
	
	std::map<std::string, std::string> _files;
	
	std::map<std::string, TextureInfos> _textures;
	
	std::map<std::string, MeshInfos> _meshes;
	
	std::map<std::string, ProgramInfos> _programs;
	
	//std::map<std::string, std::string> _shaders;
	
/// Singleton management.
		
public:
	
	static Resources& manager(){ return _resourcesManager; }
	
private:
	
	Resources(const std::string & root);
	
	~Resources();
	
	Resources& operator= (const Resources&);
	
	Resources (const Resources&);

	static Resources _resourcesManager;
	
};



#endif
