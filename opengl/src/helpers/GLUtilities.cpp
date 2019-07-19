#include "GLUtilities.h"

#include <iostream>
#include <vector>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

std::string getGLErrorString(GLenum error) {
	std::string msg;
	switch (error) {
		case GL_INVALID_ENUM:
			msg = "GL_INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			msg = "GL_INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			msg = "GL_INVALID_OPERATION";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			msg = "GL_INVALID_FRAMEBUFFER_OPERATION";
			break;
		case GL_NO_ERROR:
			msg = "GL_NO_ERROR";
			break;
		case GL_OUT_OF_MEMORY:
			msg = "GL_OUT_OF_MEMORY";
			break;
		default:
			msg = "UNKNOWN_GL_ERROR";
	}
	return msg;
}

int _checkGLError(const char *file, int line){
	GLenum glErr = glGetError();
	if (glErr != GL_NO_ERROR){
		std::cerr << "glError in " << file << " (" << line << ") : " << getGLErrorString(glErr) << std::endl;
		return 1;
	}
	return 0;
}


GLuint GLUtilities::loadShader(const std::string & prog, GLuint type){
	GLuint id;
	// Create shader object.
	id = glCreateShader(type);
	checkGLError();
	// Setup string as source.
	const char * shaderProg = prog.c_str();
	glShaderSource(id,1,&shaderProg,(const GLint*)NULL);
	// Compile the shader on the GPU.
	glCompileShader(id);
	checkGLError();

	GLint success;
	glGetShaderiv(id,GL_COMPILE_STATUS, &success);
	
	// If compilation failed, get information and display it.
	if (success != GL_TRUE) {
		GLint infoLogLength;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		std::vector<char> infoLog(std::max(infoLogLength, int(1)));
		glGetShaderInfoLog(id, infoLogLength, NULL, &infoLog[0]);

		std::cerr << std::endl 
					<< "*--- " 
					<< (type == GL_VERTEX_SHADER ? "Vertex" : (type == GL_FRAGMENT_SHADER ? "Fragment" : "Geometry (or tess.)")) 
					<< " shader failed to compile ---*" 
					<< std::endl
					<< &infoLog[0]
					<< "*---------------------------------*" 
					<< std::endl << std::endl;
	}
	// Return the id to the successfuly compiled  shader program.
	return id;
}

GLuint GLUtilities::createProgram(const std::string & vertexContent, const std::string & fragmentContent){
	GLuint vp(0), fp(0), id(0);
	id = glCreateProgram();
	checkGLError();

	// If vertex program code is given, compile it.
	if (!vertexContent.empty()) {
		vp = loadShader(vertexContent,GL_VERTEX_SHADER);
		glAttachShader(id,vp);
	}
	// If fragment program code is given, compile it.
	if (!fragmentContent.empty()) {
		fp = loadShader(fragmentContent,GL_FRAGMENT_SHADER);
		glAttachShader(id,fp);
	}
	// If geometry  exists, load it and compile it.
	/*if(!geometryPath.empty()) {
		std::string geometryCode = loadStringFromFile(geometryPath);
		if (!geometryCode.empty()) {
			gp = loadShader(geometryCode,GL_GEOMETRY_SHADER);
			glAttachShader(id,gp);
		}
	}*/

	// Link everything
	glLinkProgram(id);
	checkGLError();
	//Check linking status.
	GLint success = GL_FALSE;
	glGetProgramiv(id, GL_LINK_STATUS, &success);

	// If linking failed, query info and display it.
	if(!success) {
		GLint infoLogLength;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		std::vector<char> infoLog(std::max(infoLogLength, int(1)));
		glGetProgramInfoLog(id, infoLogLength, NULL, &infoLog[0]);

		std::cerr << "Failed loading program: " << &infoLog[0] << std::endl;
		return 0;
	}
	// We can now clean the shaders objects, by first detaching them
	if (vp != 0) {
		glDetachShader(id,vp);
	}
	if (fp != 0) {
		glDetachShader(id,fp);
	}
	/*if (gp != 0) {
		glDetachShader(id,gp);
	}*/
	checkGLError();
	//And deleting them
	glDeleteShader(vp);
	glDeleteShader(fp);
	//glDeleteShader(gp);

	checkGLError();
	// Return the id to the succesfuly linked GLProgram.
	return id;
}

TextureInfos GLUtilities::loadTexture(const std::string& path, bool sRGB){
	TextureInfos infos;
	infos.cubemap = false;
	// Load and upload the texture.
	int components = 4;
	int width = 0;
	int height = 0;
	// We need to flip the texture.
	stbi_set_flip_vertically_on_load(true);
	unsigned char *image = stbi_load(path.c_str(), &width, &height, NULL, components);
	if(image == NULL){
		std::cerr << "Unable to load the texture at path " << path << "." << std::endl;
		return infos;
	}
	
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA, width , height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(image[0]));
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	free(image);
	
	infos.id = textureId;
	infos.width = width;
	infos.height = height;
	return infos;
}




TextureInfos GLUtilities::loadTextureCubemap(const std::vector<std::string> & paths, bool sRGB){
	TextureInfos infos;
	infos.cubemap = true;
	// If not enough images, return empty texture.
	if(paths.size() != 6){
		return infos;
	}
	
	// Create and bind texture.
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
	
	// Texture settings.
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
	std::vector<unsigned char> image;
	int components = 4;
	int width = 0;
	int height = 0;
	// For each side, load the image and upload it in the right slot.
	// We don't need to flip them.
	stbi_set_flip_vertically_on_load(false);
	
	for(size_t side = 0; side < 6; ++side){
		unsigned char *image = stbi_load(paths[side].c_str(), &width, &height, NULL, components);
		if(image == NULL){
			std::cerr << "Unable to load the texture at path " << paths[side] << "." << std::endl;
			return infos;
		}
		glTexImage2D(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + side), 0, sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(image[0]));
		free(image);
	}
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	
	infos.id = textureId;
	infos.width = width;
	infos.height = height;
	return infos;
}

MeshInfos GLUtilities::setupBuffers(const Mesh & mesh){
	MeshInfos infos;
	GLuint vbo = 0;
	GLuint vbo_nor = 0;
	GLuint vbo_uv = 0;
	GLuint vbo_tan = 0;
	GLuint vbo_binor = 0;
	
	// Create an array buffer to host the geometry data.
	if(mesh.positions.size() > 0){
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.positions.size() * 3, &(mesh.positions[0]), GL_STATIC_DRAW);
	}
	
	if(mesh.normals.size() > 0){
		glGenBuffers(1, &vbo_nor);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_nor);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.normals.size() * 3, &(mesh.normals[0]), GL_STATIC_DRAW);
	}
	
	if(mesh.texcoords.size() > 0){
		glGenBuffers(1, &vbo_uv);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.texcoords.size() * 2, &(mesh.texcoords[0]), GL_STATIC_DRAW);
	}
	
	if(mesh.tangents.size() > 0){
		glGenBuffers(1, &vbo_tan);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_tan);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.tangents.size() * 3, &(mesh.tangents[0]), GL_STATIC_DRAW);
	}
	
	if(mesh.binormals.size() > 0){
		glGenBuffers(1, &vbo_binor);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_binor);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.binormals.size() * 3, &(mesh.binormals[0]), GL_STATIC_DRAW);
	}
	
	// Generate a vertex array.
	GLuint vao = 0;
	glGenVertexArrays (1, &vao);
	glBindVertexArray(vao);
	
	// Setup attributes.
	int currentAttribute = 0;
	if(vbo > 0){
		glEnableVertexAttribArray(currentAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(currentAttribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		++currentAttribute;
	}
	if(vbo_nor > 0){
		glEnableVertexAttribArray(currentAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_nor);
		glVertexAttribPointer(currentAttribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		++currentAttribute;
	}
	if(vbo_uv > 0){
		glEnableVertexAttribArray(currentAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
		glVertexAttribPointer(currentAttribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		++currentAttribute;
	}
	if(vbo_tan > 0){
		glEnableVertexAttribArray(currentAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_tan);
		glVertexAttribPointer(currentAttribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		++currentAttribute;
	}
	if(vbo_binor > 0){
		glEnableVertexAttribArray(currentAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_binor);
		glVertexAttribPointer(currentAttribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		++currentAttribute;
	}
	
	// We load the indices data
	GLuint ebo = 0;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh.indices.size(), &(mesh.indices[0]), GL_STATIC_DRAW);
	
	glBindVertexArray(0);
	
	infos.vId = vao;
	infos.eId = ebo;
	infos.count = (GLsizei)mesh.indices.size();
	return infos;
}


