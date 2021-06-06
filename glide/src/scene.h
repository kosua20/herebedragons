#ifndef SCENE_H
#define SCENE_H

#include "geometry.h"
#include "glide_helpers.h"
#include <glide.h>

enum Type {
	REGULAR, UNLIT
};

typedef struct {
	GrTexInfo tex;
	Vec3 position;
	float (*vertices)[3];
	float (*normals)[3];
	float (*uvs)[2];
	int* indices;
	enum Type type;
	unsigned int vCount;
	unsigned int iCount;
	unsigned int texAdress;
	float angle;
	float shininess;
} Object;

typedef struct {
	Object* objects;
	Vec3 light;
	unsigned int count;
	unsigned int maxVertexCount;
} Scene;

void sceneInit(Scene* scene);

void sceneUploadTextures(Scene* scene, FxU32 minAdress, FxU32 maxAdress);

void sceneUpdate(Scene* scene);

void sceneTransformAndShadeObject(Object* obj, Matrix* viewProj, Vec4* cameraPos, Vec3* lightDir, GrVertex vertices[]);

void sceneCleanup(Scene* scene);

#endif