#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <kernel.h>
#include <tamtypes.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct Memory;
struct Commands;

class Object {
	
public:
	
	Object();
	
	void render(Commands& commands, Memory& memory, MATRIX world_view, MATRIX view_screen, VECTOR light_dir);
	
	void init(const Memory& memory, unsigned int vc, VECTOR * v, VECTOR * n, unsigned char * t, unsigned char * c);

	VECTOR object_position;
	VECTOR object_rotation;
	VECTOR object_scale;
	
private:
	
	Object(const Object &);
	Object & operator = (const Object &);
	
	unsigned int _vertex_count;
	VECTOR * _vertices __attribute__((aligned(128)));
	VECTOR * _normals __attribute__((aligned(128)));

	unsigned char * _texture;
	unsigned char * _clut;
	
	prim_t prim;
	texbuffer_t tex;
	clutbuffer_t clut;
};

#endif
