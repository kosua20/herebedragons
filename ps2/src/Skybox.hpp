#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include <kernel.h>
#include <tamtypes.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct Memory;

class Skybox {
	
public:
	
	Skybox();
	
	void render(packet2_t * packet, packet2_t * texturePacket, MATRIX world_view, MATRIX view_screen, VECTOR cam_pos, Memory& memory);
	
	void init(int pc, int * p, int vc, VECTOR * v, VECTOR * uv, unsigned char* t[6], unsigned char * c[6]);
	
	
private:
	
	Skybox(const Skybox &);
	Skybox & operator = (const Skybox &);
	
	int _points_count;
	int _vertex_count;
	int * _points;
	VECTOR * _vertices;
	VECTOR * _uvs;

	unsigned char * _textures[6];
	unsigned char * _cluts[6];
	
	prim_t prim;
	color_t color;
	lod_t lod;
	texbuffer_t tex;
	clutbuffer_t clut;
};

#endif
