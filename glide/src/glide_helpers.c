#include "glide_helpers.h"

#include <glide.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void errorCallback(const char *string, FxBool fatal){
	printf("Glide %s: %s", fatal ? "error" : "warning", string);
}

int findResolution(GrResolution* screen){
	FxI32 listSize = grQueryResolutions(screen, NULL); 
	if(listSize <= 0){
		return 0;
	}

	GrResolution *list = malloc(listSize);
	grQueryResolutions(screen, list);

	screen->resolution = list[0].resolution;
	int selectedId = 0;
	// Weird bug with many resolutions.
	for(int rid = 1; rid < MIN(listSize, 16); ++rid){
		if(list[rid].resolution > screen->resolution){
			screen->resolution = list[rid].resolution;
			selectedId = rid;
		}
	}
	screen->refresh = list[selectedId].refresh;
	screen->numColorBuffers = list[selectedId].numColorBuffers;
	screen->numAuxBuffers = list[selectedId].numAuxBuffers;
	free(list);
	return 1;
}

static const char* glideResNames[] = {"320x200", "320x240", "400x256", "512x384", "640x200", "640x350", "640x400", "640x480", "800x600", "960x720", "856x480", "512x256", "1024x768", "1280x1024", "1600x1200", "400x300"};
static const char* glideFreqNames[] = { "60Hz", "70Hz", "72Hz", "75Hz", "80Hz", "90Hz", "100Hz", "85Hz", "120Hz"};

void printResolution(const GrResolution* screen){
	printf("Resolution: %s, freq: %s, #buffers: %d, #aux: %d\n", glideResNames[screen->resolution], glideFreqNames[screen->refresh], screen->numColorBuffers, screen->numAuxBuffers);
}

void transformClipToViewport(GrVertex* a, float w, float h){
	float oow = 1.f / a->xyzw[3];
	a->xyzw[3] = oow,
	a->xyzw[0] = (a->xyzw[0] * oow + 1.f) * w * 0.5f ;
	a->xyzw[1] = (a->xyzw[1] * oow + 1.f) * h * 0.5f ;
	a->ooz = (a->xyzw[2] * oow + 1.f) * 65535.f * 0.5f;
	a->xyzw[2] = a->ooz;
	a->sow *= oow;
	a->tow *= oow;
}

void calcParams(const GrVertex *a, const GrVertex *b, GrVertex *isect, float d){
	// Interpolate z, w, r, g, b, s, t
	isect->r = a->r + d * (b->r - a->r);
	isect->g = a->g + d * (b->g - a->g);
	isect->b = a->b + d * (b->b - a->b);
	isect->a = a->a + d * (b->a - a->a);
	isect->ooz = a->ooz + d * (b->ooz - a->ooz);
	isect->xyzw[3] = a->xyzw[3] + d * (b->xyzw[3] - a->xyzw[3]);
	isect->sow = a->sow + d * (b->sow - a->sow);
	isect->tow = a->tow + d * (b->tow - a->tow);
}

int clipTriangleZ(const GrVertex* a, const GrVertex* b, const GrVertex* c, GrVertex outVerts[]){

	// Early exits.
	if((a->xyzw[3] <= 0.0f) && (b->xyzw[3]<= 0.0f) && (c->xyzw[3] <= 0.0f)){
		return 0;
	}	

	GrVertex inVerts[3];
	inVerts[0] = *a;
	inVerts[1] = *b;
	inVerts[2] = *c;
	int outlength = 0;

	float curZ = inVerts[0].xyzw[2];
	char curInside = curZ >= 1.f;

	for(int j = 0; j < 3; ++j){
		int next = j + 1;
		if(next == 3){
			next = 0;
		}

		if(curInside){
			outVerts[outlength] = inVerts[j];
			++outlength;
		}

		float nextZ = inVerts[next].xyzw[2];
		char nextInside = nextZ >= 1.f;

		if(curInside != nextInside){

			if(curZ >= nextZ){
				float scale = (float)(1.f - curZ)/(nextZ - curZ);
				outVerts[outlength].xyzw[0] = inVerts[j].xyzw[0] + scale * (inVerts[next].xyzw[0] - inVerts[j].xyzw[0]);
				outVerts[outlength].xyzw[1] = inVerts[j].xyzw[1] + scale * (inVerts[next].xyzw[1] - inVerts[j].xyzw[1]);
				outVerts[outlength].xyzw[2] = inVerts[j].xyzw[2] + scale * (inVerts[next].xyzw[2] - inVerts[j].xyzw[2]);
				calcParams(&inVerts[j], &inVerts[next], &outVerts[outlength], scale);

			} else {
				float scale = (float)(1.f - nextZ)/(curZ - nextZ);
				outVerts[outlength].xyzw[0] = inVerts[next].xyzw[0] + scale * (inVerts[j].xyzw[0] - inVerts[next].xyzw[0]);
				outVerts[outlength].xyzw[1] = inVerts[next].xyzw[1] + scale * (inVerts[j].xyzw[1] - inVerts[next].xyzw[1]);
				outVerts[outlength].xyzw[2] = inVerts[next].xyzw[2] + scale * (inVerts[j].xyzw[2] - inVerts[next].xyzw[2]);
				calcParams(&inVerts[next], &inVerts[j], &outVerts[outlength], scale);

			}
			++outlength;
		}

		curZ = nextZ;
		curInside = nextInside;
	}
	return outlength;
}

// Code below lifted from the Glide 2 utilities (guclip.c)

static void intersectTop(const GrVertex *a, const GrVertex *b, GrVertex *intersect, float ymin){
	float d = (ymin - a->xyzw[1]) / (b->xyzw[1] - a->xyzw[1]);
	intersect->xyzw[0] = a->xyzw[0] + d * (b->xyzw[0] - a->xyzw[0]);
	intersect->xyzw[1] = ymin; 
	calcParams(a, b, intersect, d);
}

static void intersectBottom(const GrVertex *a, const GrVertex *b, GrVertex *intersect, float ymax){
	float d = (ymax - a->xyzw[1]) / (b->xyzw[1] - a->xyzw[1]);
	intersect->xyzw[0] = a->xyzw[0] + d * (b->xyzw[0] - a->xyzw[0]);
	intersect->xyzw[1] = ymax; 
	calcParams(a, b, intersect, d);
}

static void intersectRight(const GrVertex *a, const GrVertex *b, GrVertex *intersect, float xmax){
	float d = (xmax - a->xyzw[0]) / (b->xyzw[0] - a->xyzw[0]);
	intersect->xyzw[0] = xmax; 
	intersect->xyzw[1] = a->xyzw[1] + d * (b->xyzw[1] - a->xyzw[1]);
	calcParams(a, b, intersect, d);
}

static void intersectLeft(const GrVertex *a, const GrVertex *b, GrVertex *intersect, float xmin){
	float d = (xmin - a->xyzw[0]) / (b->xyzw[0] - a->xyzw[0]);
	intersect->xyzw[0] = xmin;
	intersect->xyzw[1] = a->xyzw[1] + d * (b->xyzw[1] - a->xyzw[1]);
	calcParams(a, b, intersect, d);
}

static FxBool aboveYMin(const GrVertex *p, float ymin){
	return ((p->xyzw[1] > ymin) ? FXTRUE : FXFALSE);
}
		 
static FxBool belowYMax(const GrVertex *p, float ymax){
	return ((p->xyzw[1] < ymax) ? FXTRUE : FXFALSE);
}

static FxBool aboveXMin(const GrVertex *p, float xmin){
	return ((p->xyzw[0] > xmin) ? FXTRUE : FXFALSE);
}

static FxBool belowXMax(const GrVertex *p, float xmax){
	return ((p->xyzw[0] < xmax) ? FXTRUE : FXFALSE);
}

static void shClipPolygon( const GrVertex invertexarray[], GrVertex outvertexarray[],
	int inlength, int *outlength, float bound, FxBool (*inside)(const GrVertex *p, float bound),
	void (*intersect)( const GrVertex *a, const GrVertex *b, GrVertex *intersect, float bound )){
	
	*outlength = 0;
	
	GrVertex s = invertexarray[inlength-1];
	for(int j = 0; j < inlength; j++ ){
		GrVertex p = invertexarray[j];
		if(inside(&p, bound)){
			if(inside(&s, bound)){
				outvertexarray[*outlength] = p;
				(*outlength)++;
			} else {
				intersect(&s, &p, &outvertexarray[*outlength], bound);
				(*outlength)++;
				outvertexarray[*outlength] = p;
				(*outlength)++;
			}
		} else {
			if(inside(&s, bound)){
				intersect(&s, &p, &outvertexarray[*outlength], bound);
				(*outlength)++;
			}
		}
		s = p;
	}
}

int clipTriangleXY(const GrVertex *a, const GrVertex *b, const GrVertex *c, GrVertex outVerts[], float xmax, float ymax){
	GrVertex output_array[8];
	
 	const float xmin = 0.f;
 	const float ymin = 0.f;

	// Perform trivial accept
	if((a->xyzw[0] >= xmin) && (a->xyzw[0] < xmax) &&
	   (a->xyzw[1] >= ymin) && (a->xyzw[1] < ymax) &&
	   (b->xyzw[0] >= xmin) && (b->xyzw[0] < xmax) &&
	   (b->xyzw[1] >= ymin) && (b->xyzw[1] < ymax) &&
	   (c->xyzw[0] >= xmin) && (c->xyzw[0] < xmax) &&
	   (c->xyzw[1] >= ymin) && (c->xyzw[1] < ymax) ){
		outVerts[0] = *a;
		outVerts[1] = *b;
		outVerts[2] = *c;
		return 3;
	}

	// Perform clipping.
	GrVertex input_array[3];
	input_array[0] = *a;
	input_array[1] = *b;
	input_array[2] = *c;
	
	int outlength;
	shClipPolygon(input_array, output_array, 3, 	  &outlength, xmax, belowXMax, intersectRight);
	shClipPolygon(output_array, outVerts, outlength, &outlength, ymax, belowYMax, intersectBottom);
	shClipPolygon(outVerts, output_array, outlength, &outlength, xmin, aboveXMin, intersectLeft);
	shClipPolygon(output_array, outVerts, outlength, &outlength, ymin, aboveYMin, intersectTop);
	
	// Snap vertices.
	static const float vertexSnapConstant = (float)(1L << 19);
	for(int i = 0; i < outlength; i++){
		outVerts[i].xyzw[0] += vertexSnapConstant;
		outVerts[i].xyzw[0] -= vertexSnapConstant;
		outVerts[i].xyzw[1] += vertexSnapConstant;
		outVerts[i].xyzw[1] -= vertexSnapConstant;
	}

	return outlength;
} 
