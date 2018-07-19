#include "Resources.hpp"

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
		std::cerr << "Unable to load file at path \"" << path << "\"." << std::endl;
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
		std::cerr << "" << filename + " is not a valid file." << std::endl;
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

int Resources::loadImage(const std::string & path, unsigned int & width, unsigned int & height, unsigned int & channels, void **data, const bool flip){
	
	size_t rawSize = 0;
	unsigned char * rawData;
	rawData = (unsigned char*)(Resources::loadRawDataFromExternalFile(path, rawSize));
	
	if(rawData == NULL || rawSize == 0){
		return 1;
	}
	
	stbi_set_flip_vertically_on_load(flip);
	// Force 4 channels.
	channels = 4;
	int localWidth = 0;
	int localHeight = 0;
	// Beware: the size has to be cast to int, imposing a limit on big file sizes.
	*data = stbi_load_from_memory(rawData, (int)rawSize, &localWidth, &localHeight, NULL, channels);
	free(rawData);
	
	if(*data == NULL){
		return 1;
	}
	
	width = (unsigned int)localWidth;
	height = (unsigned int)localHeight;
	
	return 0;
}


