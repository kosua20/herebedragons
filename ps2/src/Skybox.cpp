

#include <math3d.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>
#include <gs_psm.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <packet2.h>
#include <debug.h>
#include <malloc.h>
#include <packet2_utils.h>

#include <tamtypes.h>
#include <string.h>
#include <math.h>
#include "transform.h"
#include "Skybox.hpp"
#include "Scene.hpp"
#include "Draw.hpp"

Skybox::Skybox(){
	
	// Define the triangle primitive we want to use.
	prim.type = PRIM_TRIANGLE;
	prim.shading = PRIM_SHADE_GOURAUD;
	prim.mapping = DRAW_ENABLE;
	prim.fogging = DRAW_DISABLE;
	prim.blending = DRAW_DISABLE;
	prim.antialiasing = DRAW_DISABLE;
	prim.mapping_type = PRIM_MAP_ST;
	prim.colorfix = PRIM_UNFIXED;

	tex.width = 512;
	tex.psm = GS_PSM_8;
	tex.info.width = draw_log2(512);
	tex.info.height = draw_log2(512);
	tex.info.components = TEXTURE_COMPONENTS_RGB;
	tex.info.function = TEXTURE_FUNCTION_MODULATE;

	clut.start = 0;
	clut.storage_mode = CLUT_STORAGE_MODE1;
	clut.load_method = CLUT_LOAD;
	clut.psm = GS_PSM_32;
}

void Skybox::init(const Memory& memory, unsigned int vc, VECTOR * v, VECTOR * uv, unsigned char * t[6], unsigned char * c[6]){
	tex.address = memory.texture;
	clut.address = memory.palette;

	_vertex_count = vc;
	_vertices = v;
	_uvs = uv;
	for(int i = 0; i < 6; ++i){
		_textures[i] = t[i];
		_cluts[i] = c[i];
	}

}

void Skybox::render(Commands& commands, Memory& memory, MATRIX world_view, MATRIX view_screen){

	// UPDATE
	// Create the local_world matrix.
	MATRIX local_world;
	MATRIX local_screen;
	// Create the local world matrix.
	matrix_unit(local_world);
	// Create the local_screen matrix.
	create_local_screen(local_screen, local_world, world_view, view_screen);

	// Draw each face separately, as it uses a different texture.
	unsigned int faceVertexCount = _vertex_count / 6;
	assert(faceVertexCount <= BATCH_VERTEX_SIZE);
	
	for(int face = 0; face < 6; ++face){

		// Load the texture into vram.
		packet2_t* t = commands.nextTexture();
		packet2_update(t,draw_texture_transfer(t->next, _textures[face], 512, 512, GS_PSM_8, memory.texture, 512));
		packet2_update(t,draw_texture_transfer(t->next, _cluts[face], 16, 16, GS_PSM_32, memory.palette, 16));
		packet2_update(t,draw_texture_flush(t->next));
		dma_channel_send_packet2(t, DMA_CHANNEL_GIF,0);
		dma_wait_fast();

		// To keep things simple, rebuild the basic parameter packets at each batch.
		packet2_t * p = commands.nextIndirect();
		packet2_add_u32(p, 128); // R
		packet2_add_u32(p, 128); // G
		packet2_add_u32(p, 128); // B
		packet2_add_s32(p, faceVertexCount); // Vertex count
		packet2_utils_gs_add_texbuff_clut(p, &tex, &clut);
		packet2_utils_gs_add_prim_giftag(p, &prim, faceVertexCount, DRAW_STQ2_REGLIST, 3, 0);

		// Schedule copies from data to VU1 memory.
		packet2_t* vuPacket = commands.nextDraw();
		u32 bufferOffsetQw = 0;

		// Merge packets
		packet2_utils_vu_add_unpack_data(vuPacket, bufferOffsetQw, p->base, packet2_get_qw_count(p), 1);
		bufferOffsetQw += packet2_get_qw_count(p);
		// Add transformation matrix.
		packet2_utils_vu_add_unpack_data(vuPacket, bufferOffsetQw, &local_screen, 4, 1);
		bufferOffsetQw += 4;
		// Add vertices
		packet2_utils_vu_add_unpack_data(vuPacket, bufferOffsetQw, _vertices + face * faceVertexCount, faceVertexCount, 1);
		bufferOffsetQw += faceVertexCount;
		// Add UVs.
		packet2_utils_vu_add_unpack_data(vuPacket, bufferOffsetQw, _uvs + face * faceVertexCount, faceVertexCount, 1);
		bufferOffsetQw += faceVertexCount;

		// Execute.
		packet2_utils_vu_add_start_program(vuPacket, memory.programSkybox);
		packet2_utils_vu_add_end_tag(vuPacket);
		dma_channel_send_packet2(vuPacket, DMA_CHANNEL_VIF1, 1);
		dma_wait_fast();
	}
}
