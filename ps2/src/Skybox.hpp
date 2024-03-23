#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include <kernel.h>
#include <tamtypes.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct Memory;
struct Commands;

class Skybox {
	
public:
	
	Skybox();
	
	void render(Commands& commands, Memory& memory, MATRIX world_view, MATRIX view_screen);
	
	void init(const Memory& memory,unsigned int vc, VECTOR * v, VECTOR * uv, unsigned char* t[6], unsigned char * c[6]);
	
private:
	
	Skybox(const Skybox &);
	Skybox & operator = (const Skybox &);
	
	unsigned int _vertex_count;
	VECTOR * _vertices __attribute__((aligned(128)));
	VECTOR * _uvs __attribute__((aligned(128)));

	unsigned char * _textures[6];
	unsigned char * _cluts[6];
	
	prim_t prim;
	texbuffer_t tex;
	clutbuffer_t clut;
};

#endif
