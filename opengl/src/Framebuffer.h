#ifndef Framebuffer_h
#define Framebuffer_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


class Framebuffer {

public:
	
	/// Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	Framebuffer(int width, int height, GLuint format, GLuint type, GLuint preciseFormat, GLuint filtering, GLuint wrapping);

	~Framebuffer();
	
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
	const GLuint textureId() const { return _idColor; }
	
	/// The framebuffer size (can be different from the default renderer size).
	const int width() const { return _width; }
	const int height() const { return _height; }
	
	
private:
	
	int _width;
	int _height;
	
	GLuint _id;
	GLuint _idColor;
	GLuint _idRenderbuffer;
	
	GLuint _format;
	GLuint _type;
	GLuint _preciseFormat;
	
};

#endif
