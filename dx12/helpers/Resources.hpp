#pragma once
#include <vector>
#include <string>

#include <DirectXMath.h>

struct Vertex {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT3 binormal;
	DirectX::XMFLOAT2 texCoord;
};

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

struct Image {
	std::vector<unsigned char> data;
	unsigned int w;
	unsigned int h;
	unsigned int c;

	void swizzle(unsigned int i, unsigned int j );
};

class Resources {

	
public:
	
	static char * loadRawDataFromExternalFile(const std::string & path, size_t & size);
	
	static std::string loadStringFromExternalFile(const std::string & filename);

	static void loadImage(const std::string & path, Image& image, bool flip);
	
	static void loadMesh(const std::string& path, Mesh& mesh);

};
