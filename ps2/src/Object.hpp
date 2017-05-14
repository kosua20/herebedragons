#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <kernel.h>
#include <tamtypes.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

class Object {
	
public:
	
	Object();
	
	void render(packet_t * packet, packet_t * texturePacket, MATRIX world_view, MATRIX view_screen, texbuffer_t * _texbuf);
	
	void init(int pc, int vc, int * p, VECTOR * v, VECTOR * uv, VECTOR * n, unsigned char * t);
	
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
	
	prim_t prim;
	color_t color;
	clutbuffer_t clut;
	lod_t lod;
};

#endif
