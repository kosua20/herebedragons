#pragma once
#include "../common.hpp"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 binormal;
	glm::vec2 texCoord;
};

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

struct Image {
	std::vector<unsigned char> data;
	uint w;
	uint h;
	uint c;
};

class Resources {

	
public:
	
	static char * loadRawDataFromExternalFile(const std::string & path, size_t & size);
	
	static std::string loadStringFromExternalFile(const std::string & filename);

	static void loadImage(const std::string & path, Image& image, bool flip);
	
};
