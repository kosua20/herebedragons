#include "scene.h"

#include <stdlib.h>

#include "resources/plane_split_data.h"
#include "resources/monkey_data.h"
#include "resources/dragon_data.h"
#include "resources/cube_data.h"
#include "resources/floor_texture.h"
#include "resources/cubemap_texture.h"
#include "resources/monkey_texture.h"
#include "resources/dragon_texture.h"


void sceneInit(Scene* scene){
	// light direction
	scene->light[0] = -1.0f;
	scene->light[1] = -1.0f;
	scene->light[2] = -1.0f;
	normalize3(&(scene->light));

	scene->count = 4;
	scene->objects = (Object*)calloc(scene->count, sizeof(Object));
	
	// Monkey
	scene->objects[0].vCount = vertex_count_monkey;
	scene->objects[0].vertices = &vertices_monkey[0];
	scene->objects[0].normals = &normals_monkey[0];
	scene->objects[0].uvs = &uvs_monkey[0];
	scene->objects[0].iCount = points_count_monkey;
	scene->objects[0].indices = &points_monkey[0];
	scene->objects[0].position[0] = 8.f;
	scene->objects[0].position[1] = 2.f;
	scene->objects[0].position[2] = 8.f;
	scene->objects[0].angle = 0.0f;
	scene->objects[0].shininess = 5.0f;
	scene->objects[0].tex.data = &texture_monkey;
	scene->objects[0].type = REGULAR;

	// Floor
	scene->objects[1].vCount = vertex_count_plane_split;
	scene->objects[1].vertices = &vertices_plane_split[0];
	scene->objects[1].normals = &normals_plane_split[0];
	scene->objects[1].uvs = &uvs_plane_split[0];
	scene->objects[1].iCount = points_count_plane_split;
	scene->objects[1].indices = &points_plane_split[0];
	scene->objects[1].position[0] = 0.f;
	scene->objects[1].position[1] = -10.f;
	scene->objects[1].position[2] = 0.f;
	scene->objects[1].angle = 0.0f;
	scene->objects[1].shininess = 20.0f;
	scene->objects[1].tex.data = &texture_floor;
	scene->objects[1].type = REGULAR;

	// Dragon
	scene->objects[2].vCount = vertex_count_dragon;
	scene->objects[2].vertices = &vertices_dragon[0];
	scene->objects[2].normals = &normals_dragon[0];
	scene->objects[2].uvs = &uvs_dragon[0];
	scene->objects[2].iCount = points_count_dragon;
	scene->objects[2].indices = &points_dragon[0];
	scene->objects[2].position[0] = -11.f;
	scene->objects[2].position[1] = 5.f;
	scene->objects[2].position[2] = -11.f;
	scene->objects[2].angle = 0.0f;
	scene->objects[2].shininess = 40.0f;
	scene->objects[2].tex.data = &texture_dragon;
	scene->objects[2].type = REGULAR;

	// Skybox
	scene->objects[3].vCount = vertex_count_cube;
	scene->objects[3].vertices = &vertices_cube[0];
	scene->objects[3].normals = &normals_cube[0];
	scene->objects[3].uvs = &uvs_cube[0];
	scene->objects[3].iCount = points_count_cube;
	scene->objects[3].indices = &points_cube[0];
	scene->objects[3].position[0] = 0.f;
	scene->objects[3].position[1] = 0.f;
	scene->objects[3].position[2] = 0.f;
	scene->objects[3].angle = 0.0f;
	scene->objects[3].shininess = 0.0f;
	scene->objects[3].tex.data = &texture_cubemap;
	scene->objects[3].type = UNLIT;

	scene->maxVertexCount = 0;
	for(unsigned int i = 0; i < scene->count; ++i){
		scene->maxVertexCount = MAX(scene->maxVertexCount, scene->objects[i].vCount);
	}
}

#define TEX_2MB_LIMIT 2097152

void sceneUploadTextures(Scene* scene, FxU32 minAdress, FxU32 maxAdress){
	FxU32 currentAdress = minAdress;

	// Transfer texture data.
	for(unsigned int i = 0; i < 4; ++i){
		scene->objects[i].tex.smallLodLog2 = GR_LOD_LOG2_256;
		scene->objects[i].tex.largeLodLog2 = GR_LOD_LOG2_256;
		scene->objects[i].tex.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
		scene->objects[i].tex.format = GR_TEXFMT_RGB_565;

		if(currentAdress > maxAdress){
			break;
		}
		FxU32 textureSize = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &(scene->objects[i].tex));
		// No texture over the 2MB boundary, move forward.
		if(currentAdress < TEX_2MB_LIMIT && (currentAdress + textureSize) > TEX_2MB_LIMIT){
			currentAdress = TEX_2MB_LIMIT + 1;
		} 
		scene->objects[i].texAdress = currentAdress;

		grTexDownloadMipMap(GR_TMU0, scene->objects[i].texAdress, GR_MIPMAPLEVELMASK_BOTH, &(scene->objects[i].tex));

		currentAdress += textureSize;
	}
}

void sceneUpdate(Scene* scene){
	// Update monkey head rotation.
	scene->objects[0].angle -= 0.020f;
}

void sceneTransformAndShadeObject(Object* obj, Matrix* viewProj, Vec4* cameraPos, Vec3* lightDir, GrVertex vertices[]){
	Matrix model;
	createModel(&model, obj->position, obj->angle);
	Matrix mvp;
	mulMat(*viewProj, model, &mvp);

	// Compute local light dir.
	Matrix invModel;
	createInvModel(&invModel, obj->position, obj->angle);
	Vec3 localLightDir;
	mulDir(invModel, *lightDir, &localLightDir);
	normalize3(&localLightDir);

	Vec4 localCamPos;
	mulVec4(invModel, *cameraPos, &localCamPos);

	for(unsigned int vid = 0; vid < obj->vCount; ++vid){
		mulVec3(mvp, obj->vertices[vid], &vertices[vid].xyzw);

		float diffuse = 1.f;
		float specular = 0.f;
		// Don't light the skybox.
		if(obj->type != UNLIT){
			// Lambertian lighting.
			diffuse = MAX(-dot3(localLightDir, obj->normals[vid]), 0.0f);
			// Ambient lighting.
			diffuse += 0.1f;
			// Specular: reflect
			Vec3 reflectedLight;
			reflect(localLightDir, obj->normals[vid], &reflectedLight);
			Vec3 localViewDir;
			localViewDir[0] = localCamPos[0] - obj->vertices[vid][0];
			localViewDir[1] = localCamPos[1] - obj->vertices[vid][1];
			localViewDir[2] = localCamPos[2] - obj->vertices[vid][2];
			normalize3(&localViewDir);
			specular = pow(MAX(dot3(reflectedLight, localViewDir), 0.0f), obj->shininess);

		}

		diffuse *= 255.0f;
		specular *= 255.0f;
		
		vertices[vid].r = diffuse;
		vertices[vid].g = diffuse;
		vertices[vid].b = diffuse;
		vertices[vid].a = specular;

		vertices[vid].sow = obj->uvs[vid][0] * 255.0f;
		vertices[vid].tow = obj->uvs[vid][1] * 255.0f;
	}
}

void sceneCleanup(Scene* scene){
	free(scene->objects);
	scene->count = 0;
	scene->maxVertexCount = 0;
}