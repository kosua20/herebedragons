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

class Scene {
	
public:
	
	Scene(int width, int height, zbuffer_t * z);
	
	void clear(packet_t * packet);
	
	void render(packet_t * packet, packet_t * texturePacket);
	
	void update(Pad & pad);
	
private:
	
	Scene(const Scene &);
	Scene & operator = (const Scene &);
	
	void allocateVRAM();
	
	int _width;
	int _height;
	zbuffer_t * _z;
	texbuffer_t _texbuf;
	
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
