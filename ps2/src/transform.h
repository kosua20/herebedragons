

#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include <tamtypes.h>
#include <math3d.h>
#include <draw_types.h>

#ifdef __cplusplus
extern "C" {
#endif

	void create_look_at_center(MATRIX view_screen, VECTOR eye);
	
	// Unused

	void calculate_vertices_no_clip(VECTOR *output, int count, VECTOR *vertices, MATRIX local_screen);

	int draw_convert_white_q(color_t *output, int count, vertex_f_t *vertices);

#ifdef __cplusplus
}
#endif

#endif
