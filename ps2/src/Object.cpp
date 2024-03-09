

#include <math3d.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>
#include <gs_psm.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <packet.h>
#include <debug.h>

#include <tamtypes.h>
#include <string.h>
#include <math.h>
#include "transform.h"
#include "Object.hpp"
#include "Scene.hpp"

VECTOR light_direction[1] = {
	{  -0.577f,  -0.577f,  -0.577f, 1.00f }
};

VECTOR light_color[1] = {
	{ 1.00f, 1.00f, 1.00f, 1.00f }
};

int light_type[1] = {
	LIGHT_DIRECTIONAL
};

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
 
	color.r = 0x80;
	color.g = 0x80;
	color.b = 0x80;
	color.a = 0x80;
	color.q = 1.0f;
	
	lod.calculation = LOD_USE_K;
	lod.max_level = 0;
	lod.mag_filter = LOD_MAG_LINEAR;
	lod.min_filter = LOD_MIN_LINEAR;
	lod.l = 0;
	lod.k = 0;
	
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

void Object::init(int pc, int vc, int * p, VECTOR * v, VECTOR * uv, VECTOR * n, unsigned char * t, unsigned char * c){
	_points_count = pc;
	_vertex_count = vc;
	_points = p;
	_vertices = v;
	_uvs = uv;
	_normals = n;
	_texture = t;
	_clut = c;
}

void Object::render(packet_t * packet, packet_t * texturePacket, MATRIX world_view, MATRIX view_screen, Memory& memory){
	
	// Load the texture into vram.
	qword_t * qt = texturePacket->data;
	
	qt = draw_texture_transfer(qt, _texture, 1024, 1024, GS_PSM_8, memory.texture, 1024);
	qt = draw_texture_transfer(qt, _clut, 16, 16, GS_PSM_32, memory.palette, 16);
	
	qt = draw_texture_flush(qt);
	dma_channel_send_chain(DMA_CHANNEL_GIF, texturePacket->data, qt - texturePacket->data, 0,0);
	dma_wait_fast();
	
	qword_t * q = packet->data;
	qword_t * start = q++;

	// Texture sampling.
	tex.address = memory.texture;
	clut.address = memory.palette;
	q = draw_texture_sampling(q, 0, &lod);
	q = draw_texturebuffer(q, 0, &tex, &clut);
	
	// UPDATE
	// Create the local_world matrix.
	MATRIX local_world;
	MATRIX local_light;
	MATRIX local_screen;
	// Create the local world matrix.
	matrix_unit(local_world);
	matrix_rotate(local_world, local_world, object_rotation);
	matrix_translate(local_world, local_world, object_position);
	// Create the local_screen matrix.
	create_local_screen(local_screen, local_world, world_view, view_screen);
	// Create the local light matrix.
	create_local_light(local_light, object_rotation);
	
	// Calculate the normal values.
	calculate_normals(memory.normals_tmp, _vertex_count, _normals, local_light);
	
	// Calculate the lighting values.
	calculate_lights(memory.lights_tmp, _vertex_count, memory.normals_tmp, light_direction, light_color, light_type, 1);
	
	// Calculate the vertex values.
	calculate_vertices_no_clip(memory.verts_tmp, _vertex_count, _vertices, local_screen);
	
	// Convert floating point vertices to fixed point and translate to center of screen.
	draw_convert_xyz(memory.verts_final, 2048, 2048, 16, _vertex_count, (vertex_f_t*)memory.verts_tmp);
	draw_convert_rgbq(memory.colors_final, _vertex_count, (vertex_f_t*)memory.verts_tmp, (color_f_t*)memory.lights_tmp, 0x80);
	draw_convert_st(memory.uvs_final, _vertex_count, (vertex_f_t*)memory.verts_tmp, (texel_f_t*)_uvs);
	
	// QUEUE
	// Draw the triangles using triangle primitive type.
	q = draw_prim_start(q,0,&prim, &color);
	u64 *dw = (u64*)(q);
	
	for(int i = 0; i < _points_count; i+=3){
		
		const int p0 = _points[i];
		const int p1 = _points[i+1];
		const int p2 = _points[i+2];

		const float p0x = memory.verts_tmp[p0][0];
		const float p0y = memory.verts_tmp[p0][1];
		const float p1x = memory.verts_tmp[p1][0];
		const float p1y = memory.verts_tmp[p1][1];
		const float p2x = memory.verts_tmp[p2][0];
		const float p2y = memory.verts_tmp[p2][1];

		// Backface culling.
		const float orientation = (p1x - p0x) * (p2y - p0y) - (p1y - p0y) * (p2x - p0x);
		if(orientation < 0.0f) {
			continue;
		}
		
		// Clipping.
		// As soon as one vertex is clipped, we discard the triangle.
		// We rely on the relative high-density of the meshes to avoid having huge faces disappearing all of a sudden.
		if(fabs(p0x) > 1.0f || fabs(p0y) > 1.0f || fabs(p1x) > 1.0f || fabs(p1y) > 1.0f || fabs(p2x) > 1.0f || fabs(p2y) > 1.0f){
			continue;
		}

		const float p0z = memory.verts_tmp[p0][2];
		const float p1z = memory.verts_tmp[p1][2];
		const float p2z = memory.verts_tmp[p2][2];
		if(p0z < -1.f || p0z > 0.f || p1z < -1.f || p1z > 0.f || p2z < -1.f || p2z > 0.f){
			continue;
		}
	
		*dw++ = memory.colors_final[p0].rgbaq;
		*dw++ = memory.uvs_final[p0].uv;
		*dw++ = memory.verts_final[p0].xyz;
		
		*dw++ = memory.colors_final[p1].rgbaq;
		*dw++ = memory.uvs_final[p1].uv;
		*dw++ = memory.verts_final[p1].xyz;
		
		*dw++ = memory.colors_final[p2].rgbaq;
		*dw++ = memory.uvs_final[p2].uv;
		*dw++ = memory.verts_final[p2].xyz;
		
	}
	// Check if we're in middle of a qword or not.
	while (reinterpret_cast<u32>(dw) % 16) {
		*dw++ = 0u;
	}
	
	q = draw_prim_end((qword_t*)dw,3,DRAW_STQ_REGLIST);
	q = draw_finish(q);
	DMATAG_END(start, q - start - 1, 0, 0, 0);
	
	dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();
	
}
