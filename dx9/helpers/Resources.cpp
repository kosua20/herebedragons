#include "Resources.hpp"
#include "MeshUtilities.hpp"
#include "Log.hpp"

#include <algorithm>
#include <ios>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#ifdef _WIN32
#define STBI_MSC_SECURE_CRT
#endif

char * Resources::loadRawDataFromExternalFile(const std::string & path, size_t & size) {
	char * rawContent;
	std::ifstream inputFile(path, std::ios::binary|std::ios::ate);
	if (inputFile.bad() || inputFile.fail()){
		logInfo("Unable to load file at path %s.\n", path.c_str());
		return NULL;
	}
	std::ifstream::pos_type fileSize = inputFile.tellg();
	rawContent = new char[fileSize];
	inputFile.seekg(0, std::ios::beg);
	inputFile.read(&rawContent[0], fileSize);
	inputFile.close();
	size = fileSize;
	return rawContent;
}

std::string Resources::loadStringFromExternalFile(const std::string & filename) {
	std::ifstream in;
	// Open a stream to the file.
	in.open(filename.c_str());
	if (!in) {
		logInfo("%s is not a valid file.\n", filename.c_str());
		return "";
	}
	std::stringstream buffer;
	// Read the stream in a buffer.
	buffer << in.rdbuf();
	// Create a string based on the content of the buffer.
	std::string line = buffer.str();
	in.close();
	return line;
}

void Resources::loadImage(const std::string & path, Image& image, bool flip){
	image.w = image.h = image.c = 0;
	image.data.clear();

	size_t rawSize = 0;
	unsigned char * rawData = (unsigned char*)(Resources::loadRawDataFromExternalFile(path, rawSize));
	
	if(rawData == NULL || rawSize == 0){
		return;
	}
	
	stbi_set_flip_vertically_on_load(flip);
	// Force 4 channels.
	int localChannels = 4;
	int localWidth = 0;
	int localHeight = 0;
	// Beware: the size has to be cast to int, imposing a limit on big file sizes.
	unsigned char* data = stbi_load_from_memory(rawData, (int)rawSize, &localWidth, &localHeight, NULL, localChannels);
	free(rawData);
	
	if(data == NULL){
		return;
	}
	
	image.w = (unsigned int)localWidth;
	image.h = (unsigned int)localHeight;
	image.c = (unsigned int)localChannels;
	image.data = std::vector<unsigned char>(data, data + image.w * image.h * image.c);
	
	STBI_FREE(data);
}

void Resources::loadMesh(const std::string& path, Mesh& mesh){
	MeshUtilities::loadObj(path, mesh, MeshUtilities::LoadMode::Indexed);
	MeshUtilities::centerAndUnitMesh(mesh);
	MeshUtilities::computeTangentsAndBinormals(mesh);
}

void Image::swizzle( unsigned int i, unsigned int j )
{
	if( (i >= c) ||( j >= c) || (i == j))
		return;

	for( unsigned int y = 0; y < h; ++y )
	{
		for( unsigned int x = 0; x < w; ++x )
		{
			int pixelIndex = (y * w + x) * c;
			std::swap( data[ pixelIndex + i ], data[ pixelIndex + j ] );
		}
	}
}