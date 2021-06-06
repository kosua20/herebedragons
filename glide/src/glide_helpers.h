#ifndef GLIDEHELPERS_H
#define GLIDEHELPERS_H

#include <glide.h>
#include "geometry.h"

typedef struct{
	Vec4 xyzw;			// X and Y in screen space, 1/W (used for depth test)
	float ooz;			// 65535/Z (used for Z-buffering)
	float r, g, b, a;	// RGBA*255
	float sow, tow;		// Texture coordinates 255*(S/W, T/W)
	float pad;			// padding for alignment
} GrVertex;

#define GR_VERTEX_X_OFFSET			0
#define GR_VERTEX_Y_OFFSET			1
#define GR_VERTEX_Z_OFFSET			2
#define GR_VERTEX_OOW_OFFSET		3
#define GR_VERTEX_OOZ_OFFSET		4
#define GR_VERTEX_RGB_OFFSET		5
#define GR_VERTEX_A_OFFSET			8
#define GR_VERTEX_STOW_OFFSET		9
#define GR_VERTEX_OOW_TMU0_OFFSET	11

void errorCallback(const char *string, FxBool fatal);

int findResolution(GrResolution* screen);

void printResolution(const GrResolution* screen);

void transformClipToViewport(GrVertex* a, float w, float h);

int clipTriangleZ(const GrVertex *a, const GrVertex *b, const GrVertex *c, GrVertex outVerts[]);

int clipTriangleXY(const GrVertex *a, const GrVertex *b, const GrVertex *c, GrVertex outVerts[], float xmax, float ymax);

#endif