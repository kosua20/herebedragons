
#include <math3d.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>
#include <gs_psm.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <packet2.h>
#include <math.h>
#include <malloc.h>

#include "data.h"
#include "transform.h"
#include "Scene.hpp"

#define ANGULAR_SPEED 0.1f
#define RADIUS_SPEED 3.0f

Scene::Scene(int width, int height)  {
	_width = width;
	_height = height;
	
	_radius = 150.0f;
	_horizontalAngle = 3.14159f/2.0f;
	_verticalAngle = 0.15f;
	
	camera_position[0] = _radius * cosf(_horizontalAngle) * cosf(_verticalAngle);
	camera_position[1] = _radius * sinf(_verticalAngle);
	camera_position[2] = _radius * sinf(_horizontalAngle) * cosf(_verticalAngle);
	
	// Create the view_screen matrix.
	create_view_screen(view_screen, graph_aspect_ratio(), -3.00f, 3.00f, -3.00f, 3.00f, 1.00f, 2000.00f);
	
	// Setup meshes.
	unsigned char* skyTexs[] = { texture_cubemap_b, texture_cubemap_f,texture_cubemap_l, texture_cubemap_r, texture_cubemap_d, texture_cubemap_u};
	unsigned char* skyCluts[] = { palette_cubemap_b, palette_cubemap_f, palette_cubemap_l, palette_cubemap_r, palette_cubemap_d, palette_cubemap_u};
	_skybox.init(points_count_cube, points_cube, vertex_count_cube, vertices_cube, uvs_cube, skyTexs, skyCluts);
	
	_plane.init(points_count_plane, vertex_count_plane, points_plane, vertices_plane, uvs_plane, normals_plane, texture_floor, (unsigned char*)palette_floor);
	
	_monkey.init(points_count_monkey, vertex_count_monkey, points_monkey, vertices_monkey, uvs_monkey, normals_monkey, texture_monkey, (unsigned char*)palette_monkey);
	_monkey.object_position[0] = 8.0f;
	_monkey.object_position[1] = 12.0f;
	_monkey.object_position[2] = 8.0f;
	
	_dragon.init(points_count_dragon, vertex_count_dragon, points_dragon, vertices_dragon, uvs_dragon, normals_dragon, texture_dragon, (unsigned char*)palette_dragon);
	_dragon.object_position[0] = -11.0f;
	_dragon.object_position[1] = 15.0f;
	_dragon.object_position[2] = -11.0f;

	int maxVertexCount = vertex_count_cube;
	maxVertexCount = vertex_count_dragon > maxVertexCount ? vertex_count_dragon : maxVertexCount;
	maxVertexCount = vertex_count_monkey > maxVertexCount ? vertex_count_monkey : maxVertexCount;
	maxVertexCount = vertex_count_plane > maxVertexCount ? vertex_count_plane : maxVertexCount;

	allocateVRAM(maxVertexCount, 1024u);
}

void Scene::allocateVRAM(unsigned int maxVertexCount, unsigned int maxTextureSize){

	// Allocate some vram for the texture buffer
	_memory.texture = graph_vram_allocate(maxTextureSize, maxTextureSize, GS_PSM_8, GRAPH_ALIGN_BLOCK);
	// Allocate some vram for the texture buffer
	_memory.palette = graph_vram_allocate(16,16,GS_PSM_32,GRAPH_ALIGN_BLOCK);

	_memory.verts_tmp    = static_cast<VECTOR* >(memalign(128, sizeof(VECTOR)  * maxVertexCount));
	_memory.normals_tmp  = static_cast<VECTOR* >(memalign(128, sizeof(VECTOR)  * maxVertexCount));
	_memory.lights_tmp   = static_cast<VECTOR* >(memalign(128, sizeof(VECTOR)  * maxVertexCount));
	
	_memory.verts_final  = static_cast<xyz_t*  >(memalign(128, sizeof(xyz_t  ) * maxVertexCount));
	_memory.uvs_final 	 = static_cast<texel_t*>(memalign(128, sizeof(texel_t) * maxVertexCount));
	_memory.colors_final = static_cast<color_t*>(memalign(128, sizeof(color_t) * maxVertexCount));

}

void Scene::clear(packet2_t * p, zbuffer_t * z){
	
	// Room for dma end tag
	qword_t* start = (p->next)++;
	packet2_update(p, draw_disable_tests(p->next, 0, z));
	packet2_update(p, draw_clear(p->next, 0, 2048.0f-(_width/2), 2048.0f-(_height/2), _width, _height, 74, 68, 85));
	packet2_update(p, draw_enable_tests(p->next, 0, z));
	packet2_update(p, draw_finish(p->next));

	DMATAG_END(start, p->next - start - 1, 0, 0, 0);
	dma_channel_send_packet2(p, DMA_CHANNEL_GIF, 0);
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


void Scene::render(packet2_t * packet, packet2_t * texturePacket){
	
	// Create the world_view matrix.
	create_look_at_center(world_view,  camera_position);
	// Render the objects.
	_monkey.render(packet, texturePacket, world_view, view_screen, _memory);
	_dragon.render(packet, texturePacket, world_view, view_screen, _memory);
	_plane.render(packet, texturePacket, world_view, view_screen, _memory);
	_skybox.render(packet, texturePacket, world_view, view_screen, camera_position, _memory);
}
