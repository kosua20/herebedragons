#include "scene.h"
#include "glide_helpers.h"
#include "keyboard.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>


#define ANGULAR_SPEED 0.1f
#define RADIUS_SPEED 3.0f


void resetState(FxU32 w, FxU32 h){
	grCoordinateSpace(GR_WINDOW_COORDS);
	grViewport((FxU32)0, (FxU32)0, (FxU32)w, (FxU32)h);
	grClipWindow((FxU32)0, (FxU32)0, (FxU32)w, (FxU32)h);
	grDepthRange(0.0f, 1.0f);
	grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
	grDepthBufferFunction(GR_CMP_LESS);
	grDepthMask(FXTRUE);
	grColorMask(FXTRUE, FXFALSE);
	grCullMode(GR_CULL_POSITIVE);
	// Texture filtering.
	grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
	grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
	grTexMipMapMode(GR_TMU0, GR_MIPMAP_DISABLE, FXFALSE);
	// Combining:	diffus * texture + specular
	//				rgb vertex * texture0 + alpha vertex
	grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA, 
		GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
	grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL, 
		GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_NONE, FXFALSE);
	grTexCombine(GR_TMU0, GR_COMBINE_FUNCTION_LOCAL, 
		GR_COMBINE_FACTOR_NONE, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);
}

void resetCamera(float* radius, float* horizontalAngle, float* verticalAngle){
	*radius = 40.0f;
	*horizontalAngle = 3.14159f/2.0f;
	*verticalAngle = 0.15f;
}

int main( int argc, char **argv){
	
	// Initialize Glide
	grGlideInit();

	// Select first device if present.
	FxI32 n = grGet(GR_NUM_BOARDS, sizeof(n), &n);
	if(n == 0){
		printf("Unable to find device.\n");
		return 0;
	}
	grSstSelect(0);
	
	// Find best fit resolution.
	GrResolution screen;
	screen.resolution = GR_QUERY_ANY;
	screen.refresh = GR_QUERY_ANY; 
	screen.numColorBuffers = GR_QUERY_ANY; 
	screen.numAuxBuffers = 1; // Need a depth buffer.

	if(!findResolution(&screen)){
		printf("Unable to find valid resolution.\n");
		return 0;
	}

	// Basic infos.
	const char* hardware = grGetString(GR_HARDWARE);
	const char *version = grGetString( GR_VERSION );
	printf( "Device: %s, %s\n", version, hardware );
	printResolution(&screen);

	// Create window.
	GrContext_t context = grSstWinOpen((FxU32)NULL, screen.resolution, screen.refresh, 
		GR_COLORFORMAT_ABGR, GR_ORIGIN_UPPER_LEFT, screen.numColorBuffers, screen.numAuxBuffers );
	if(context == 0){
		printf("Unable to open window.\n");
		return 0;
	}
	grErrorSetCallback(errorCallback);

	// Input setup.
	initKeyboard();

	// Graphics setup.
	FxI32 viewport[4];
	grGet(GR_VIEWPORT, 16, viewport);
	FxI32 drange[2];
	grGet(GR_WDEPTH_MIN_MAX, 8, drange); 
	printf("Depth range: %ld, %ld.\nViewport: %ld,%ld, %ld,%ld\n", drange[0], drange[1], 
		viewport[0], viewport[1], viewport[2], viewport[3]);
	// Set viewport.
	resetState(viewport[2], viewport[3]);

	// Define vertex layout.
	grVertexLayout(GR_PARAM_XY, GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
	grVertexLayout(GR_PARAM_Z, GR_VERTEX_OOZ_OFFSET << 2, GR_PARAM_ENABLE);
	grVertexLayout(GR_PARAM_Q, GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
	grVertexLayout(GR_PARAM_RGB, GR_VERTEX_RGB_OFFSET << 2, GR_PARAM_ENABLE);
	grVertexLayout(GR_PARAM_A, GR_VERTEX_A_OFFSET << 2, GR_PARAM_ENABLE);
	grVertexLayout(GR_PARAM_ST0, GR_VERTEX_STOW_OFFSET << 2, GR_PARAM_ENABLE);

	// Camera:
	Matrix view;
	Matrix proj;
	Matrix viewProj;

	// Initialize position.
	float radius, horizontalAngle, verticalAngle;
	resetCamera(&radius, &horizontalAngle, &verticalAngle);
	Vec4 cameraPos;
	cameraPos[0] = radius * cosf(horizontalAngle) * cosf(verticalAngle);
	cameraPos[1] = radius * sinf(verticalAngle);
	cameraPos[2] = radius * sinf(horizontalAngle) * cosf(verticalAngle);
	cameraPos[3] = 1.f;
	
	// Projection matrix.
	createProjection((float)viewport[2]/(float)viewport[3], -1.00f, 1.00f, -1.00f, 1.00f, 1.0f, 1000.00f, &proj);
	
	// Objects init.
	Scene scene;
	sceneInit(&scene);

	// Texture upload.
	FxU32 minTex = grTexMinAddress(GR_TMU0);
	FxU32 maxTex = grTexMaxAddress(GR_TMU0);
	sceneUploadTextures(&scene, minTex, maxTex);

	// Temporary vertices.
	GrVertex* vertices = (GrVertex*)calloc(scene.maxVertexCount, sizeof(GrVertex));

	// Render loop.
	char updateCamera = 1;
	while(1) {
	
		// Handle inputs.
		if(isPressed(KEY_ESC)){
			break;
		}

		if(isPressed(KEY_LEFT)){
			horizontalAngle += ANGULAR_SPEED;
			updateCamera = 1;
		}
		if(isPressed(KEY_RIGHT)){
			horizontalAngle -= ANGULAR_SPEED;
			updateCamera = 1;
		}
		if(isPressed(KEY_UP)){
			verticalAngle += ANGULAR_SPEED;
			updateCamera = 1;
		}
		if(isPressed(KEY_DOWN)){
			verticalAngle -= ANGULAR_SPEED;
			updateCamera = 1;
		}
		if(isPressed(KEY_W)){
			radius -= RADIUS_SPEED;
			updateCamera = 1;
		}
		if(isPressed(KEY_S)){
			radius += RADIUS_SPEED;
			updateCamera = 1;
		}
		if(isPressed(KEY_R)){
			resetCamera(&radius, &horizontalAngle, &verticalAngle);
			updateCamera = 1;
		}

		if(updateCamera){
			// Clamping.
			if(radius < 0.0f){
				radius = 0.0f;
			} else if(radius > 280.0f){
				radius = 280.0f;
			}
			
			if(verticalAngle > 3.14159f*0.5f){
				verticalAngle = 3.14159f*0.5f;
			} else if(verticalAngle < -3.14159f*0.5f){
				verticalAngle = -3.14159f*0.5f;
			}
			
			// Update camera position.
			cameraPos[0] = radius * cosf(horizontalAngle) * cosf(verticalAngle);
			cameraPos[1] = radius * sinf(verticalAngle);
			cameraPos[2] = radius * sinf(horizontalAngle) * cosf(verticalAngle);

			// Update view matrix.
			createLookAtCenter(&view, cameraPos);
			mulMat(proj, view, &viewProj);
			updateCamera = 0;
		}

		sceneUpdate(&scene);
		
		// Clear depth buffer.
		grRenderBuffer( GR_BUFFER_BACKBUFFER );
		grColorMask(FXFALSE, FXFALSE);
		grBufferClear( 0x000000, 0, drange[1] );
		grColorMask(FXTRUE, FXFALSE);

		// Draw each object.
		for(int oid = 0; oid < scene.count; ++oid){

			Object* obj = &(scene.objects[oid]);

			// Attach the right texture.
			grTexSource(GR_TMU0, obj->texAdress, GR_MIPMAPLEVELMASK_BOTH, &(obj->tex));

			// Transform each vertex and compute its shading.
			sceneTransformAndShadeObject(obj, &viewProj, &cameraPos, &scene.light, vertices);

			GrVertex clipVerts[8];
			GrVertex vpVerts[8];
			for(unsigned int tid = 0; tid < obj->iCount; tid += 3){

				// Clip into multiple triangles based on Z planes if needed.
				int clipCount = clipTriangleZ(&vertices[obj->indices[tid]],
					&vertices[obj->indices[tid+1]],
					&vertices[obj->indices[tid+2]], clipVerts);
				
				if(clipCount < 3){
					continue;
				}

				// Perspective divide and convert from NDC to viewport.
				for(int cid = 0; cid < clipCount; ++cid){
					transformClipToViewport(&clipVerts[cid], viewport[2], viewport[3]);
				}

				// Clip to viewoprt and draw.
				for(int cid = 0; cid < clipCount-1; ++cid){

					int subCount = clipTriangleXY(&clipVerts[0], &clipVerts[cid], &clipVerts[cid+1], 
						vpVerts, viewport[2], viewport[3]);
					if(subCount < 3){
						continue;
					}

					grDrawVertexArrayContiguous( GR_POLYGON, subCount, vpVerts, sizeof(GrVertex) );
				}
			}
		}
		
		grBufferSwap(1);
	}
	
	grSstWinClose(context);
	grGlideShutdown();
	free(vertices);
	sceneCleanup(&scene);

    cleanupKeyboard();
	return 0;
}

