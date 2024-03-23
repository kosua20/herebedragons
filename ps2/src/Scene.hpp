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
#include "Draw.hpp"

class Scene {
	
public:
	
	Scene(int width, int height);
	
	void clear(Commands& commands, zbuffer_t * z);
	
	void render(Commands& commands);
	
	void update(Pad & pad);
	
private:

	Memory _memory;
	
	Scene(const Scene &);
	Scene & operator = (const Scene &);
	
	void allocateVRAM(unsigned int maxTextureSize);
	
	Skybox _skybox;
	Object _plane;
	Object _monkey;
	Object _dragon;
	
	MATRIX _world_view;
	MATRIX _view_screen;

	VECTOR _camera_position;
	VECTOR _light_direction;
	float _radius;
	float _horizontalAngle;
	float _verticalAngle;

	float _time;
	int _width;
	int _height;
	
};

#endif
