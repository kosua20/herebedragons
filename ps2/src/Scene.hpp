#ifndef SCENE_HPP
#define SCENE_HPP

#include <kernel.h>
#include <tamtypes.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Pad.hpp"
#include "Object.hpp"
#include "Skybox.hpp"


struct Memory {
	unsigned int texture;
	unsigned int palette;

	VECTOR* verts_tmp;
	VECTOR* normals_tmp;
	VECTOR* lights_tmp;

	xyz_t* verts_final;
	texel_t* uvs_final;
	color_t* colors_final;
};

class Scene {
	
public:
	
	Scene(int width, int height);
	
	void clear(packet2_t * packet, zbuffer_t * z);
	
	void render(packet2_t * packet, packet2_t * texturePacket);
	
	void update(Pad & pad);
	
private:

	Memory _memory;
	
	Scene(const Scene &);
	Scene & operator = (const Scene &);
	
	void allocateVRAM(unsigned int maxVertexCount, unsigned int maxTextureSize);
	
	int _width;
	int _height;
	
	float _radius;
	float _horizontalAngle;
	float _verticalAngle;
	VECTOR camera_position;
	
	Skybox _skybox;
	Object _plane;
	Object _monkey;
	Object _dragon;
	
	MATRIX world_view;
	MATRIX view_screen;
	
};

#endif
