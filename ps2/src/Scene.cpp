
#include <math3d.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>
#include <gs_psm.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <packet.h>
#include <math.h>

#include "data.h"
#include "transform.h"
#include "Scene.hpp"

#define ANGULAR_SPEED 0.1f
#define RADIUS_SPEED 3.0f

Scene::Scene(int width, int height, zbuffer_t * z) : _plane(true), _monkey(false), _dragon(false) {
	_width = width;
	_height = height;
	_z = z;
	
	_radius = 150.0f;
	_horizontalAngle = 3.14159f/2.0f;
	_verticalAngle = 0.15f;
	
	camera_position[0] = _radius * cosf(_horizontalAngle) * cosf(_verticalAngle);
	camera_position[1] = _radius * sinf(_verticalAngle);
	camera_position[2] = _radius * sinf(_horizontalAngle) * cosf(_verticalAngle);
	
	// Create the view_screen matrix.
	create_view_screen(view_screen, graph_aspect_ratio(), -3.00f, 3.00f, -3.00f, 3.00f, 1.00f, 2000.00f);
	
	allocateVRAM();
	
	// Setup meshes.
	_skybox.init(points_count_cube, points_cube, vertex_count_cube, vertices_cube, uvs_cube, texture_cubemap);
	
	_plane.init(points_count_plane, vertex_count_plane, points_plane, vertices_plane, uvs_plane, normals_plane, texture_floor);
	
	_monkey.init(points_count_monkey, vertex_count_monkey, points_monkey, vertices_monkey, uvs_monkey, normals_monkey, texture_monkey);
	_monkey.object_position[0] = 8.0f;
	_monkey.object_position[1] = 12.0f;
	_monkey.object_position[2] = 8.0f;
	
	_dragon.init(points_count_dragon, vertex_count_dragon, points_dragon, vertices_dragon, uvs_dragon, normals_dragon, texture_dragon);
	_dragon.object_position[0] = -11.0f;
	_dragon.object_position[1] = 15.0f;
	_dragon.object_position[2] = -11.0f;

}

void Scene::allocateVRAM(){
	// Allocate some vram for the texture buffer
	_texbuf.width = 256;
	_texbuf.psm = GS_PSM_24;
	_texbuf.address = graph_vram_allocate(256,256,GS_PSM_24,GRAPH_ALIGN_BLOCK);
	
	// Sampling will be setup for each object, at render time.
	_texbuf.info.width = draw_log2(256);
	_texbuf.info.height = draw_log2(256);
	_texbuf.info.components = TEXTURE_COMPONENTS_RGB;
	_texbuf.info.function = TEXTURE_FUNCTION_MODULATE;
}

void Scene::clear(packet_t * packet){
	
	qword_t * q = packet->data;
	qword_t * start = q++;
	q = draw_disable_tests(q, 0, _z);
	q = draw_clear(q, 0, 2048.0f-(_width/2), 2048.0f-(_height/2), _width, _height, 60, 68, 85);
	q = draw_enable_tests(q,0,_z);
	q = draw_finish(q);
	
	DMATAG_END(start, q - start - 1, 0, 0, 0);
	dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();
}

void Scene::update(Pad & pad){
	// Read pad buttons.
	if(pad.pressed(Pad::UP)){
		_verticalAngle += ANGULAR_SPEED;
	}
	if(pad.pressed(Pad::DOWN)){
		_verticalAngle -= ANGULAR_SPEED;
	}
	if(pad.pressed(Pad::LEFT)){
		_horizontalAngle += ANGULAR_SPEED;
	}
	if(pad.pressed(Pad::RIGHT)){
		_horizontalAngle -= ANGULAR_SPEED;
	}
	if(pad.pressed(Pad::TRIANGLE)){
		_radius -= RADIUS_SPEED;
	}
	if(pad.pressed(Pad::CROSS)){
		_radius += RADIUS_SPEED;
	}
	
	// Clamping.
	if(_radius < 0.0f){
		_radius = 0.0f;
	} else if(_radius > 280.0f){
		_radius = 280.0f;
	}
	
	if(_verticalAngle > 3.14159f*0.5f){
		_verticalAngle = 3.14159f*0.5f;
	} else if(_verticalAngle < -3.14159f*0.5f){
		_verticalAngle = -3.14159f*0.5f;
	}
	
	// Update camera position.
	camera_position[0] = _radius * cosf(_horizontalAngle) * cosf(_verticalAngle);
	camera_position[1] = _radius * sinf(_verticalAngle);
	camera_position[2] = _radius * sinf(_horizontalAngle) * cosf(_verticalAngle);
	
	_monkey.object_rotation[1] -= 0.020f;
}


void Scene::render(packet_t * packet, packet_t * texturePacket){
	
	// Create the world_view matrix.
	create_look_at_center(world_view,  camera_position);
	// Render the objects.
	_monkey.render(packet, texturePacket, world_view, view_screen, &_texbuf);
	_dragon.render(packet, texturePacket, world_view, view_screen, &_texbuf);
	_plane.render(packet, texturePacket, world_view, view_screen, &_texbuf);
	_skybox.render(packet, texturePacket, world_view, view_screen, &_texbuf, camera_position);
}
