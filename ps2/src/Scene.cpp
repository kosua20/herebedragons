
#include <math3d.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>
#include <gs_psm.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <packet2.h>
#include <packet2_utils.h>
#include <math.h>
#include <malloc.h>

#include "data.h"
#include "transform.h"
#include "Scene.hpp"
#include "Draw.hpp"

// VUA programs data.

extern u32 VU1SkyboxProgram_CodeStart __attribute__((section(".vudata")));
extern u32 VU1SkyboxProgram_CodeEnd __attribute__((section(".vudata")));

extern u32 VU1ObjectProgram_CodeStart __attribute__((section(".vudata")));
extern u32 VU1ObjectProgram_CodeEnd __attribute__((section(".vudata")));

// Camera params.

#define ANGULAR_SPEED 0.1f
#define RADIUS_SPEED 3.0f


Scene::Scene(int width, int height)  {
	_width = width;
	_height = height;
	_time = 0.0f;
	
	_radius = 150.0f;
	_horizontalAngle = 3.14159f/2.0f;
	_verticalAngle = 0.15f;
	
	_camera_position[0] = _radius * cosf(_horizontalAngle) * cosf(_verticalAngle);
	_camera_position[1] = _radius * sinf(_verticalAngle);
	_camera_position[2] = _radius * sinf(_horizontalAngle) * cosf(_verticalAngle);
	_camera_position[3] = 1.f;
	
	_light_direction[0] = 1.f;
	_light_direction[1] = 0.6f;
	_light_direction[2] = 1.f;
	_light_direction[3] = 0.f;

	// Create the view_screen matrix.
	create_view_screen(_view_screen, graph_aspect_ratio(), -3.00f, 3.00f, -3.00f, 3.00f, 1.00f, 2000.00f);
	
	// Allocate memory for texture and palette upload.
	allocateVRAM(1024u);

	// Setup meshes.
	unsigned char* skyTexs[] = { texture_cubemap_b, texture_cubemap_f,texture_cubemap_l, texture_cubemap_r, texture_cubemap_d, texture_cubemap_u};
	unsigned char* skyCluts[] = { palette_cubemap_b, palette_cubemap_f, palette_cubemap_l, palette_cubemap_r, palette_cubemap_d, palette_cubemap_u};
	_skybox.init(_memory, vertices_count_cube, vertices_cube, uvs_cube, skyTexs, skyCluts);
	
	_plane.init(_memory, vertex_count_plane, vertices_plane, normals_plane, texture_floor, (unsigned char*)palette_floor);
	
	_monkey.init(_memory, vertex_count_monkey, vertices_monkey, normals_monkey, texture_monkey, (unsigned char*)palette_monkey);
	_monkey.object_position[0] = 8.0f;
	_monkey.object_position[1] = 12.0f;
	_monkey.object_position[2] = 8.0f;
	
	_dragon.init(_memory, vertex_count_dragon, vertices_dragon, normals_dragon, texture_dragon, (unsigned char*)palette_dragon);
	_dragon.object_position[0] = -11.0f;
	_dragon.object_position[1] = 15.0f;
	_dragon.object_position[2] = -11.0f;

}

void Scene::allocateVRAM(unsigned int maxTextureSize){

	// Allocate some vram for the texture buffer
	_memory.texture = graph_vram_allocate(maxTextureSize, maxTextureSize, GS_PSM_8, GRAPH_ALIGN_BLOCK);
	// Allocate some vram for the texture buffer
	_memory.palette = graph_vram_allocate(16,16,GS_PSM_32,GRAPH_ALIGN_BLOCK);

	// Upload VU1 programs
	// Sizes in packets, each sending up to 256 instructions of 2 bytes.
	unsigned int size0 = packet2_utils_get_packet_size_for_program(&VU1ObjectProgram_CodeStart, &VU1ObjectProgram_CodeEnd);
	unsigned int size1 = packet2_utils_get_packet_size_for_program(&VU1SkyboxProgram_CodeStart, &VU1SkyboxProgram_CodeEnd);
	unsigned int totalSize = size0 + size1 + 2u;
	// Offsets in qwords, to store the programs in VU1 program memory.
	_memory.programObject = 0u;
	_memory.programSkybox = ( (size0 + 1) * 512 ) / 16;
	// One shot packet chain for initial setup..
	packet2_t *p = packet2_create(totalSize + 1, P2_TYPE_NORMAL, P2_MODE_CHAIN, 1);
	packet2_vif_add_micro_program(p, _memory.programObject, &VU1ObjectProgram_CodeStart, &VU1ObjectProgram_CodeEnd);
	packet2_vif_add_micro_program(p, _memory.programSkybox, &VU1SkyboxProgram_CodeStart, &VU1SkyboxProgram_CodeEnd);
	// Configure data memory to use double buffering (toggle via top in the program).
	
	//Immediately setup constant data: all textureswill use the same sampler.
	lod_t lod;
	lod.calculation = LOD_USE_K;
	lod.max_level = 0;
	lod.mag_filter = LOD_MAG_LINEAR;
	lod.min_filter = LOD_MIN_NEAREST;
	lod.l = 0;
	lod.k = 0;

	packet2_t *p0 = packet2_create(24, P2_TYPE_NORMAL, P2_MODE_CHAIN, 1);
	packet2_add_float(p0, 2048.0f);					// Viewport W
	packet2_add_float(p0, -2048.0f); 				// Viewport H
	packet2_add_float(p0, (float)((1u << 31u))); 	// Z range
	packet2_add_u32(p0, 128);  						// Default opaque alpha
	packet2_utils_gif_add_set(p0, 1);
	packet2_utils_gs_add_lod(p0, &lod);

	unsigned int constantSize = packet2_get_qw_count(p0);
	assert(constantSize + 2 * VU1_BUFFER_SIZE <= 1024);

	packet2_utils_vu_add_double_buffer(p, constantSize, VU1_BUFFER_SIZE);
	packet2_utils_vu_add_unpack_data(p, 0, p0->base, constantSize, 0);

	packet2_utils_vu_add_end_tag(p);
	dma_channel_send_packet2(p, DMA_CHANNEL_VIF1, 1);
	dma_channel_wait(DMA_CHANNEL_VIF1, 0);
	packet2_free(p);

}

void Scene::clear(Commands& commands, zbuffer_t * z){
	packet2_t* p = commands.nextGeneral();
	// Room for dma end tag
	qword_t* start = (p->next)++;
	packet2_update(p, draw_disable_tests(p->next, 0, z));
	packet2_update(p, draw_clear(p->next, 0, 2048.0f-(_width/2), 2048.0f-(_height/2), _width, _height, 57, 66, 82));
	packet2_update(p, draw_enable_tests(p->next, 0, z));
	packet2_update(p, draw_finish(p->next));
	DMATAG_END(start, p->next - start - 1, 0, 0, 0);
	dma_channel_send_packet2(p, DMA_CHANNEL_GIF, 0);
	dma_wait_fast();
}

void Scene::render(Commands& commands){
	
	dma_channel_wait(DMA_CHANNEL_VIF1, 0);

	// Render the objects.
	_monkey.render(commands, _memory, _world_view, _view_screen, _light_direction);
	_dragon.render(commands, _memory, _world_view, _view_screen, _light_direction);
	_plane.render(commands, _memory, _world_view, _view_screen, _light_direction);
	_skybox.render(commands, _memory, _world_view, _view_screen);
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
	_camera_position[0] = _radius * cosf(_horizontalAngle) * cosf(_verticalAngle);
	_camera_position[1] = _radius * sinf(_verticalAngle);
	_camera_position[2] = _radius * sinf(_horizontalAngle) * cosf(_verticalAngle);
	
	const float deltaTime = 0.016f;
	_time += deltaTime;

	// Update light direction.
	_light_direction[0] = 1.0f;
	_light_direction[1] = 0.5f * sinf(_time) + 1.0f;
	_light_direction[2] = 1.0f;
	vector_normalize(_light_direction,_light_direction);
	

	// Update head rotation.
	_monkey.object_rotation[1] -= 1.3f * deltaTime;

	// Update the world_view matrix.
	create_look_at_center(_world_view, _camera_position);
}


