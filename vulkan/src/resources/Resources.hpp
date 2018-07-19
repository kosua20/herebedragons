#ifndef Resources_h
#define Resources_h


#include "../common.hpp"


class Resources {

	
public:
	
	static char * loadRawDataFromExternalFile(const std::string & path, size_t & size);
	
	static std::string loadStringFromExternalFile(const std::string & filename);
	
	static int loadImage(const std::string & path, unsigned int & width, unsigned int & height, unsigned int & channels, void **data, const bool flip);
	
};


#endif
