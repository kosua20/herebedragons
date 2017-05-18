

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

VECTOR light_direction[1] = {
	{  -0.577f,  -0.577f,  -0.577f, 1.00f }
};

VECTOR light_color[1] = {
	{ 1.00f, 1.00f, 1.00f, 1.00f }
};

int light_type[1] = {
	LIGHT_DIRECTIONAL
};

Object::Object(bool interpolateTexture){
	
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
	prim.antialiasing = DRAW_ENABLE;
	prim.mapping_type = PRIM_MAP_ST;
	prim.colorfix = PRIM_UNFIXED;
 
	color.r = 0x80;
	color.g = 0x80;
	color.b = 0x80;
	color.a = 0x80;
	color.q = 1.0f;
	
	lod.calculation = LOD_USE_K;
	lod.max_level = 0;
	lod.mag_filter = interpolateTexture ? LOD_MAG_LINEAR : LOD_MAG_NEAREST;
	lod.min_filter = LOD_MIN_NEAREST;
	lod.l = 0;
	lod.k = 0;
	
	clut.storage_mode = CLUT_STORAGE_MODE1;
	clut.start = 0;
	clut.psm = 0;
	clut.load_method = CLUT_NO_LOAD;
	clut.address = 0;
}

void Object::init(int pc, int vc, int * p, VECTOR * v, VECTOR * uv, VECTOR * n, unsigned char * t){
	_points_count = pc;
	_vertex_count = vc;
	_points = p;
	_vertices = v;
	_uvs = uv;
	_normals = n;
	_texture = t;
}

void Object::render(packet_t * packet, packet_t * texturePacket, MATRIX world_view, MATRIX view_screen, texbuffer_t * _texbuf){
	
	// Load the texture into vram.
	qword_t * qt = texturePacket->data;
	
	qt = draw_texture_transfer(qt, _texture, 256, 256, GS_PSM_24, _texbuf->address, _texbuf->width);
	
	qt = draw_texture_flush(qt);
	dma_channel_send_chain(DMA_CHANNEL_GIF, texturePacket->data, qt - texturePacket->data, 0,0);
	dma_wait_fast();
	
	
	qword_t * q = packet->data;
	qword_t * start = q++;

	// Texture sampling.
	q = draw_texture_sampling(q, 0, &lod);
	q = draw_texturebuffer(q, 0, _texbuf, &clut);
	
	xyz_t   *verts;
	color_t *colors;
	texel_t *uvs;
	
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
	
	VECTOR * temp_vertices;
	VECTOR * temp_normals;
	VECTOR * temp_lights;
	
	temp_vertices = static_cast<VECTOR *>(memalign(128, sizeof(VECTOR) * _vertex_count));
	temp_normals = static_cast<VECTOR *>(memalign(128, sizeof(VECTOR) * _vertex_count));
	temp_lights = static_cast<VECTOR *>(memalign(128, sizeof(VECTOR) * _vertex_count));
	
	// Calculate the normal values.
	calculate_normals(temp_normals, _vertex_count, _normals, local_light);
	
	// Calculate the lighting values.
	calculate_lights(temp_lights, _vertex_count, temp_normals, light_direction, light_color, light_type, 1);
	
	// Calculate the vertex values.
	calculate_vertices_no_clip(temp_vertices, _vertex_count, _vertices, local_screen);
	
	// Convert floating point vertices to fixed point and translate to center of screen.
	verts  = static_cast<xyz_t*>(memalign(128, sizeof(xyz_t) * _vertex_count));
	colors = static_cast<color_t*>(memalign(128, sizeof(color_t)  * _vertex_count));
	uvs = static_cast<texel_t*>(memalign(128, sizeof(texel_t)  * _vertex_count));
	
	draw_convert_xyz(verts, 2048, 2048, 32, _vertex_count, (vertex_f_t*)temp_vertices);
	draw_convert_rgbq(colors, _vertex_count, (vertex_f_t*)temp_vertices, (color_f_t*)temp_lights, 0x80);
	draw_convert_st(uvs, _vertex_count, (vertex_f_t*)temp_vertices, (texel_f_t*)_uvs);
	
	// QUEUE
	// Draw the triangles using triangle primitive type.
	q = draw_prim_start(q,0,&prim, &color);
	u64 *dw = (u64*)(q);
	
	for(int i = 0; i < _points_count; i+=3){
		
		int p0 = _points[i];
		int p1 = _points[i+1];
		int p2 = _points[i+2];
		
		// Backface culling.
		float orientation = (temp_vertices[p1][0] - temp_vertices[p0][0]) * (temp_vertices[p2][1] - temp_vertices[p0][1]) - (temp_vertices[p1][1] - temp_vertices[p0][1]) * (temp_vertices[p2][0] - temp_vertices[p0][0]);
		if(orientation < 0.0) {
			continue;
		}
		
		// Clipping.
		// As soon as one vertex is clipped, we discard the triangle.
		// We rely on the relative high-density of the meshes to avoid having huge faces disappearing all of a sudden.
		if(temp_vertices[p0][2] < -1.0 || temp_vertices[p0][2] > 0 || temp_vertices[p0][0] > 1.0 || temp_vertices[p0][0] < -1.0 || temp_vertices[p0][1] > 1.0 || temp_vertices[p0][1] < -1.0){
			continue;
		}
		if(temp_vertices[p1][2] < -1.0 || temp_vertices[p1][2] > 0 || temp_vertices[p1][0] > 1.0 || temp_vertices[p1][0] < -1.0 || temp_vertices[p1][1] > 1.0 || temp_vertices[p1][1] < -1.0){
			continue;
		}
		if(temp_vertices[p2][2] < -1.0 || temp_vertices[p2][2] > 0 || temp_vertices[p2][0] > 1.0 || temp_vertices[p2][0] < -1.0 || temp_vertices[p2][1] > 1.0 || temp_vertices[p2][1] < -1.0){
			continue;
		}
	
		*dw++ = colors[p0].rgbaq;
		*dw++ = uvs[p0].uv;
		*dw++ = verts[p0].xyz;
		
		*dw++ = colors[p1].rgbaq;
		*dw++ = uvs[p1].uv;
		*dw++ = verts[p1].xyz;
		
		*dw++ = colors[p2].rgbaq;
		*dw++ = uvs[p2].uv;
		*dw++ = verts[p2].xyz;
		
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
	
	free(temp_vertices);
	free(temp_normals);
	free(temp_lights);
	free(uvs);
	free(verts);
	free(colors);
}
