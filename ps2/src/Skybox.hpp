#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include <kernel.h>
#include <tamtypes.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

class Skybox {
	
public:
	
	Skybox();
	
	void render(packet_t * packet, packet_t * texturePacket, MATRIX world_view, MATRIX view_screen, texbuffer_t * _texbuf, VECTOR cam_pos);
	
	void init(int points_count, int * points, int vc, VECTOR * v, VECTOR * uv, unsigned char * t);
	
	
private:
	
	Skybox(const Skybox &);
	Skybox & operator = (const Skybox &);
	
	int _points_count;
	int _vertex_count;
	int * _points;
	VECTOR * _vertices;
	VECTOR * _uvs;
	unsigned char * _texture;
	
	prim_t prim;
	color_t color;
	clutbuffer_t clut;
	lod_t lod;
};

#endif
