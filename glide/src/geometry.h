#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <math.h>

#define CLAMP(A, B0, B1) (A <= B0 ? B0 : (A >= B1 ? B1 : A))
#define MAX(A, B) (A <= B ? B : A)
#define MIN(A, B) (A <= B ? A : B)

typedef float Vec3[3];
typedef float Vec4[4];

// Row, column
typedef Vec4 Matrix[4];

float norm3(const Vec3 v);

float normXYZ4(const Vec4 v);

void normalize3(Vec3* v);

void normalizeXYZ4(Vec4* v);

float dot3(const Vec3 a, const Vec3 b);

float dotXYZ4(const Vec4 a, const Vec4 b);

float dot4(const Vec4 a, const Vec4 b);

void crossXYZ3(const Vec4 a, const Vec4 b, Vec4* c);

void reflect(const Vec3 i, const Vec3 n, Vec3* r);

void createLookAtCenter(Matrix* view, const Vec4 eyePos);

void createProjection(float aspect, float left, float right, float bottom, float top, float near, float far, Matrix* proj);

void mulMat(const Matrix a, const Matrix b, Matrix* c);

void mulVec4(const Matrix a, const Vec4 b, Vec4* c);

void mulVec3(const Matrix a, const Vec3 b, Vec4* c);

void mulDir(const Matrix a, const Vec3 b, Vec3* c);

void createModel(Matrix* model, const Vec3 position, float angleY);

void createInvModel(Matrix* invModel, const Vec3 position, float angleY);

void createDirectionalInvModel(Matrix* invModel, float angleY);

#endif