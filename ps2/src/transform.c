#include <tamtypes.h>
#include <graph.h>
#include <string.h>
#include <math3d.h>

#include "transform.h"

void create_look_at_center(MATRIX view_screen, VECTOR eye) {
	
	// Create the look_at matrix.
	matrix_unit(view_screen);
	
	VECTOR f;
	f[0] = eye[0];
	f[1] = eye[1];
	f[2] = eye[2];
	f[3] = 0.0f;
	vector_normalize(f, f);
	VECTOR s;
	VECTOR up = {0.0f,1.0f,0.0f,0.0f};
	vector_outerproduct(s, up, f);
	vector_normalize(s,s);
	VECTOR u;
	vector_outerproduct(u,f, s);
	
	view_screen[0x00] = s[0];
	view_screen[0x04] = s[1];
	view_screen[0x08] = s[2];
	
	view_screen[0x01] = u[0];
	view_screen[0x05] = u[1];
	view_screen[0x09] = u[2];
	
	view_screen[0x02] = f[0];
	view_screen[0x06] = f[1];
	view_screen[0x0A] = f[2];
	
	view_screen[0x03] = -(s[0]*eye[0]+s[1]*eye[1]+s[2]*eye[2]);
	view_screen[0x07] = -(u[0]*eye[0]+u[1]*eye[1]+u[2]*eye[2]);
	view_screen[0x0E] = -(f[0]*eye[0]+f[1]*eye[1]+f[2]*eye[2]);
}

void calculate_vertices_no_clip(VECTOR *output,  int count, VECTOR *vertices, MATRIX local_screen) {
asm __volatile__ (
					  "lqc2		$vf1, 0x00(%3)	\n"
					  "lqc2		$vf2, 0x10(%3)	\n"
					  "lqc2		$vf3, 0x20(%3)	\n"
					  "lqc2		$vf4, 0x30(%3)	\n"
					  "1:					\n"
					  "lqc2		$vf6, 0x00(%2)	\n"
					  "vmulaw		$ACC, $vf4, $vf0	\n"
					  "vmaddax		$ACC, $vf1, $vf6	\n"
					  "vmadday		$ACC, $vf2, $vf6	\n"
					  "vmaddz		$vf7, $vf3, $vf6	\n"
					  "3:					\n"
					  "vdiv		$Q, $vf0w, $vf7w	\n"
					  "vwaitq				\n"
					  "vmulq.xyz		$vf7, $vf7, $Q	\n"
					  "sqc2		$vf7, 0x00(%0)	\n"
					  "4:					\n"
					  "addi		%0, 0x10	\n"
					  "addi		%2, 0x10	\n"
					  "addi		%1, -1		\n"
					  "bne		$0, %1, 1b	\n"
					  : : "r" (output), "r" (count), "r" (vertices), "r" (local_screen) : "$10"
					  );
}


int draw_convert_white_q(color_t *output, int count, vertex_f_t *vertices)
{
	
	int i;
	float q = 1.00f;
	
	// For each colour...
	for (i=0;i<count;i++)
	{
		
		// Calculate the Q value.
		if (vertices[i].w != 0)
		{
			
			q = 1 / vertices[i].w;
			
		}
		
		// Calculate the RGBA values.
		output[i].r = 0x80;
		output[i].g = 0x80;
		output[i].b = 0x80;
		output[i].a = 0x80;
		output[i].q = q;
		
	}
	
	// End function.
	return 0;
	
}
