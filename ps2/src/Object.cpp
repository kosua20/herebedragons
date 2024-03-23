

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
#include <debug.h>
#include <malloc.h>

#include <tamtypes.h>
#include <string.h>
#include <math.h>
#include "transform.h"
#include "Object.hpp"
#include "Scene.hpp"
#include "Draw.hpp"

Object::Object(){
	
	object_position[0] = 0.0f;
	object_position[1] = 0.0f;
	object_position[2] = 0.0f;
	object_position[3] = 1.0f;
	
	object_rotation[0] = 0.0f;
	object_rotation[1] = 0.0f;
	object_rotation[2] = 0.0f;
	object_rotation[3] = 1.0f;
	
	// Define the triangle primitive we want to use.
	prim.type = PRIM_TRIANGLE;
	prim.shading = PRIM_SHADE_GOURAUD;
	prim.mapping = DRAW_ENABLE;
	prim.fogging = DRAW_DISABLE;
	prim.blending = DRAW_DISABLE;
	prim.antialiasing = DRAW_DISABLE; // Probably unnecessary? Can be re-enabled later
	prim.mapping_type = PRIM_MAP_ST;
	prim.colorfix = PRIM_UNFIXED;
 
	tex.width = 1024;
	tex.psm = GS_PSM_8;
	tex.info.width = draw_log2(1024);
	tex.info.height = draw_log2(1024);
	tex.info.components = TEXTURE_COMPONENTS_RGB;
	tex.info.function = TEXTURE_FUNCTION_MODULATE;

	clut.start = 0;
	clut.storage_mode = CLUT_STORAGE_MODE1;
	clut.load_method = CLUT_LOAD;
	clut.psm = GS_PSM_32;
}

void Object::init(const Memory& memory, unsigned int vc, VECTOR * v, VECTOR * n, unsigned char * t, unsigned char * c){
	tex.address = memory.texture;
	clut.address = memory.palette;

	_vertex_count = vc;
	_vertices = v;
	_normals = n;
	_texture = t;
	_clut = c;
}

void Object::render(Commands& commands, Memory& memory, MATRIX world_view, MATRIX view_screen, VECTOR light_dir){
	
	// UPDATE
	// Create the local_world matrix.
	MATRIX local_world_rot;
	MATRIX world_local_rot;
	MATRIX local_world;
	MATRIX local_screen;
	// Create the local world matrix, and its translation-independent inverse.
	matrix_unit(local_world_rot);
	matrix_rotate(local_world_rot, local_world_rot, object_rotation);
	matrix_transpose(world_local_rot, local_world_rot);
	matrix_translate(local_world, local_world_rot, object_position);
	// Create the local_screen matrix.
	create_local_screen(local_screen, local_world, world_view, view_screen);
	// And the local light direction.
	VECTOR local_light_dir;
	vector_apply(local_light_dir, light_dir, world_local_rot);
	vector_normalize(local_light_dir, local_light_dir);

	// Load the texture into vram.
	packet2_t* t = commands.nextTexture();
	packet2_update(t, draw_texture_transfer(t->next, _texture, 1024, 1024, GS_PSM_8, memory.texture, 1024));
	packet2_update(t, draw_texture_transfer(t->next, _clut, 16, 16, GS_PSM_32, memory.palette, 16));
	packet2_update(t, draw_texture_flush(t->next));
	dma_channel_send_packet2(t, DMA_CHANNEL_GIF, 0);
	dma_wait_fast();
	
	// Submit batches of vertices for processing on VU1.
	for(unsigned int pIndex = 0; pIndex < _vertex_count; pIndex += BATCH_VERTEX_SIZE){

		unsigned int vertexCount = _vertex_count - pIndex;
		vertexCount = vertexCount > BATCH_VERTEX_SIZE ? BATCH_VERTEX_SIZE : vertexCount;

		// To keep things simple, rebuild the basic parameter packets at each batch.
		packet2_t * p = commands.nextIndirect();
		// Prescale light color for conversion, but keep it as float for computations.
		packet2_add_float(p, 128.f); // White light	
		packet2_add_float(p, 128.f); 
		packet2_add_float(p, 128.f); 
		packet2_add_s32(p, vertexCount); // Vertex count
		packet2_utils_gs_add_texbuff_clut(p, &tex, &clut);
		packet2_utils_gs_add_prim_giftag(p, &prim, vertexCount, DRAW_STQ2_REGLIST, 3, 0);

		// Schedule copies from data to VU1 memory.
		packet2_t* vuPacket = commands.nextDraw();
		u32 bufferOffsetQw = 0;

		// Merge packets
		packet2_utils_vu_add_unpack_data(vuPacket, bufferOffsetQw, p->base, packet2_get_qw_count(p), 1);
		bufferOffsetQw += packet2_get_qw_count(p);
		// Transformation data.
		packet2_utils_vu_add_unpack_data(vuPacket, bufferOffsetQw, &local_screen, 4, 1);
		bufferOffsetQw += 4;
		packet2_utils_vu_add_unpack_data(vuPacket, bufferOffsetQw, &local_light_dir, 1, 1);
		bufferOffsetQw += 1;
		// Add vertices
		packet2_utils_vu_add_unpack_data(vuPacket, bufferOffsetQw, _vertices + pIndex, vertexCount, 1);
		bufferOffsetQw += vertexCount;
		// Add normals
		packet2_utils_vu_add_unpack_data(vuPacket, bufferOffsetQw, _normals + pIndex, vertexCount, 1);
		bufferOffsetQw += vertexCount;

		// Execute.
		packet2_utils_vu_add_start_program(vuPacket, memory.programObject);
		packet2_utils_vu_add_end_tag(vuPacket);
		dma_channel_send_packet2(vuPacket, DMA_CHANNEL_VIF1, 1);
		dma_wait_fast();
	}
}
