#ifndef Gbuffer_h
#define Gbuffer_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <map>
#include <vector>

enum class TextureType {
	Albedo,
	Normal,
	Depth,
	Effects
};

class Gbuffer {

public:
	
	/// Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	Gbuffer(int width, int height);

	~Gbuffer();
	
	/// Bind the framebuffer.
	void bind() const;
	
	/// Unbind the framebuffer.
	void unbind() const;
	
	/// Resize the framebuffer.
	void resize(int width, int height);
	
	void resize(glm::vec2 size);
	
	/// Clean.
	void clean() const;
	
	/// The ID to the texture containing the result of the framebuffer pass.
	const GLuint textureId(const TextureType& type) { return _textureIds[type]; }
	
	const std::map<std::string, GLuint> textureIds(const std::vector<TextureType>& included = std::vector<TextureType>()) const ;
	
	/// The framebuffer size (can be different from the default renderer size).
	const int width() const { return _width; }
	const int height() const { return _height; }
	
private:
	int _width;
	int _height;
	
	GLuint _id;
	std::map<TextureType, GLuint> _textureIds;
};

#endif
