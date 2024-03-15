#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <kernel.h>
#include <tamtypes.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct Memory;

class Object {
	
public:
	
	Object();
	
	void render(packet2_t * packet, packet2_t * texturePacket, MATRIX world_view, MATRIX view_screen, Memory& memory);
	
	void init(int pc, int vc, int * p, VECTOR * v, VECTOR * uv, VECTOR * n, unsigned char * t, unsigned char * c);
	
	VECTOR object_position;
	VECTOR object_rotation;
	VECTOR object_scale;
	
private:
	
	Object(const Object &);
	Object & operator = (const Object &);
	
	int _points_count;
	int _vertex_count;
	int * _points;
	VECTOR * _vertices;
	VECTOR * _uvs;
	VECTOR * _normals;
	unsigned char * _texture;
	unsigned char * _clut;
	
	prim_t prim;
	color_t color;
	lod_t lod;
	texbuffer_t tex;
	clutbuffer_t clut;
};

#endif
