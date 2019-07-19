#ifndef ProgramUtilities_h
#define ProgramUtilities_h

#include <gl3w/gl3w.h>
#include <string>
#include <vector>
#include "MeshUtilities.h"

/// This macro is used to check for OpenGL errors with access to the file and line number where the error is detected.
#define checkGLError() _checkGLError(__FILE__, __LINE__)

/// Converts a GLenum error number into a human-readable string.
std::string getGLErrorString(GLenum error);

/// Check if any OpenGL error has been detected and log it.
int _checkGLError(const char *file, int line);

struct TextureInfos {
	GLuint id;
	int width;
	int height;
	bool cubemap;

	TextureInfos() : id(0), width(0), height(0), cubemap(false) {}

};

struct MeshInfos {
	GLuint vId;
	GLuint eId;
	GLsizei count;

	MeshInfos() : vId(0), eId(0), count(0) {}

};


class GLUtilities {
	
	/// Load a shader of the given type from a string
	static GLuint loadShader(const std::string & prog, GLuint type);
	
public:
	
	// Program setup.
	/// Create a GLProgram using the shader code contained in the given strings.
	static GLuint createProgram(const std::string & vertexContent, const std::string & fragmentContent);
	
	// Texture loading.
	/// 2D texture.
	static TextureInfos loadTexture(const std::string& path, bool sRGB);
	/// Cubemap texture.
	static TextureInfos loadTextureCubemap(const std::vector<std::string> & paths, bool sRGB);
	
	// Mesh loading.
	static MeshInfos setupBuffers(const Mesh & mesh);
	
};


#endif
