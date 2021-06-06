#include "geometry.h"

float norm3(const Vec3 v){
	return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void normalize3(Vec3* v){
	float n = norm3(*v);
	if(n != 0.f){
		(*v)[0] /= n; 
		(*v)[1] /= n;
		(*v)[2] /= n;
	}
}

float normXYZ4(const Vec4 v){
	return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void normalizeXYZ4(Vec4* v){
	float n = normXYZ4(*v);
	if(n != 0.f){
		(*v)[0] /= n; 
		(*v)[1] /= n;
		(*v)[2] /= n;
	}
}

float dot3(const Vec3 a, const Vec3 b){
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

float dotXYZ4(const Vec4 a, const Vec4 b){
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

float dot4(const Vec4 a, const Vec4 b){
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
}

void crossXYZ4(const Vec4 a, const Vec4 b, Vec4* c){
	(*c)[0] = a[1] * b[2] - a[2] * b[1];
	(*c)[1] = a[2] * b[0] - a[0] * b[2];
	(*c)[2] = a[0] * b[1] - a[1] * b[0];
	(*c)[3] = 0.f;
}

void reflect(const Vec3 i, const Vec3 n, Vec3* r){
	float proj = 2.f * dot3(i, n);
	(*r)[0] = i[0] - proj * n[0];
	(*r)[1] = i[1] - proj * n[1];
	(*r)[2] = i[2] - proj * n[2];
	normalize3(r);
}

void createLookAtCenter(Matrix* view, const Vec4 eyePos) {
	// Center = (0,0,0).
	Vec4 f;
	f[0] = -eyePos[0];
	f[1] = -eyePos[1];
	f[2] = -eyePos[2];
	f[3] = 0.f;
	normalizeXYZ4(&f);

	Vec4 s;
	Vec4 up = {0.0f,1.0f,0.0f,0.0f};
	crossXYZ4(f, up, &s);
	normalizeXYZ4(&s);

	Vec4 u;
	crossXYZ4(s, f, &u);
	
	(*view)[0][0] = s[0];
	(*view)[0][1] = s[1];
	(*view)[0][2] = s[2];
	(*view)[0][3] = -dotXYZ4(s, eyePos);
	
	(*view)[1][0] = u[0];
	(*view)[1][1] = u[1];
	(*view)[1][2] = u[2];
	(*view)[1][3] = -dotXYZ4(u, eyePos);
	
	(*view)[2][0] = -f[0];
	(*view)[2][1] = -f[1];
	(*view)[2][2] = -f[2];
	(*view)[2][3] = dotXYZ4(f, eyePos);
	
	(*view)[3][0] = 0.f;
	(*view)[3][1] = 0.f;
	(*view)[3][2] = 0.f;
	(*view)[3][3] = 1.f;
}

void createProjection(float aspect, float left, float right, float bottom, float top, float near, float far, Matrix* proj) {

	// Apply the aspect ratio adjustment.
	left = (left * aspect); 
	right = (right * aspect);

	// Create the projection matrix.
	(*proj)[0][0] = (2.f * near) / (right - left);
	(*proj)[0][1] = 0.00f;
	(*proj)[0][2] = (right + left) / (right - left);
	(*proj)[0][3] = 0.00f;

	(*proj)[1][0] = 0.00f;
	(*proj)[1][1] = -(2.f * near) / (top - bottom);
	(*proj)[1][2] = -(top + bottom) / (top - bottom);
	(*proj)[1][3] = 0.00f;
	
	(*proj)[2][0] = 0.00f;
	(*proj)[2][1] = 0.00f;
	(*proj)[2][2] = -(far + near) / (far - near);
	(*proj)[2][3] = -(2.f * far * near) / (far - near);
	
	(*proj)[3][0] = 0.00f;
	(*proj)[3][1] = 0.00f;
	(*proj)[3][2] = -1.00f;
	(*proj)[3][3] = 0.00f;

}


void mulMat(const Matrix a, const Matrix b, Matrix* c){
	(*c)[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0] + a[0][3] * b[3][0];
	(*c)[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1] + a[0][3] * b[3][1];
	(*c)[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2] + a[0][3] * b[3][2];
	(*c)[0][3] = a[0][0] * b[0][3] + a[0][1] * b[1][3] + a[0][2] * b[2][3] + a[0][3] * b[3][3];

	(*c)[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0] + a[1][3] * b[3][0];
	(*c)[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1] + a[1][3] * b[3][1];
	(*c)[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2] + a[1][3] * b[3][2];
	(*c)[1][3] = a[1][0] * b[0][3] + a[1][1] * b[1][3] + a[1][2] * b[2][3] + a[1][3] * b[3][3];

	(*c)[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0] + a[2][3] * b[3][0];
	(*c)[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1] + a[2][3] * b[3][1];
	(*c)[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2] + a[2][3] * b[3][2];
	(*c)[2][3] = a[2][0] * b[0][3] + a[2][1] * b[1][3] + a[2][2] * b[2][3] + a[2][3] * b[3][3];

	(*c)[3][0] = a[3][0] * b[0][0] + a[3][1] * b[1][0] + a[3][2] * b[2][0] + a[3][3] * b[3][0];
	(*c)[3][1] = a[3][0] * b[0][1] + a[3][1] * b[1][1] + a[3][2] * b[2][1] + a[3][3] * b[3][1];
	(*c)[3][2] = a[3][0] * b[0][2] + a[3][1] * b[1][2] + a[3][2] * b[2][2] + a[3][3] * b[3][2];
	(*c)[3][3] = a[3][0] * b[0][3] + a[3][1] * b[1][3] + a[3][2] * b[2][3] + a[3][3] * b[3][3];

}

void mulVec4(const Matrix a, const Vec4 b, Vec4* c){
	(*c)[0] = a[0][0] * b[0] + a[0][1] * b[1] + a[0][2] * b[2] + a[0][3] * b[3];
	(*c)[1] = a[1][0] * b[0] + a[1][1] * b[1] + a[1][2] * b[2] + a[1][3] * b[3];
	(*c)[2] = a[2][0] * b[0] + a[2][1] * b[1] + a[2][2] * b[2] + a[2][3] * b[3];
	(*c)[3] = a[3][0] * b[0] + a[3][1] * b[1] + a[3][2] * b[2] + a[3][3] * b[3];
}

void mulVec3(const Matrix a, const Vec3 b, Vec4* c){
	(*c)[0] = a[0][0] * b[0] + a[0][1] * b[1] + a[0][2] * b[2] + a[0][3];
	(*c)[1] = a[1][0] * b[0] + a[1][1] * b[1] + a[1][2] * b[2] + a[1][3];
	(*c)[2] = a[2][0] * b[0] + a[2][1] * b[1] + a[2][2] * b[2] + a[2][3];
	(*c)[3] = a[3][0] * b[0] + a[3][1] * b[1] + a[3][2] * b[2] + a[3][3];
}

void mulDir(const Matrix a, const Vec3 b, Vec3* c){
	(*c)[0] = a[0][0] * b[0] + a[0][1] * b[1] + a[0][2] * b[2];
	(*c)[1] = a[1][0] * b[0] + a[1][1] * b[1] + a[1][2] * b[2];
	(*c)[2] = a[2][0] * b[0] + a[2][1] * b[1] + a[2][2] * b[2];
}


void createModel(Matrix* model, const Vec3 position, float angleY){
	// Rotation matrix.
	Matrix rot;
	float cosine = cos(angleY);
	float sine = sin(angleY);
	rot[0][0] = cosine;
	rot[0][1] = 0.0f;
	rot[0][2] = sine;
	rot[0][3] = 0.f;

	rot[1][0] = 0.0f;
	rot[1][1] = 1.f;
	rot[1][2] = 0.0f;
	rot[1][3] = 0.f;

	rot[2][0] = -sine;
	rot[2][1] = 0.0f;
	rot[2][2] = cosine;
	rot[2][3] = 0.f;

	rot[3][0] = 0.0f;
	rot[3][1] = 0.0f;
	rot[3][2] = 0.0f;
	rot[3][3] = 1.0f;


	Matrix trans;
	trans[0][0] = trans[1][1] = trans[2][2] = trans[3][3] = 1.f;
	trans[0][1] = trans[0][2] = 0.0f;
	trans[1][0] = trans[1][2] = 0.0f;
	trans[2][0] = trans[2][1] = 0.0f;
	trans[3][0] = trans[3][1] = trans[3][2] = 0.0f;

	trans[0][3] = position[0];
	trans[1][3] = position[1];
	trans[2][3] = position[2];

	mulMat(trans, rot, model);
}


void createInvModel(Matrix* invModel, const Vec3 position, float angleY){
	// Rotation matrix.
	Matrix rot;
	float cosine = cos(-angleY);
	float sine = sin(-angleY);
	rot[0][0] = cosine;
	rot[0][1] = 0.0f;
	rot[0][2] = sine;
	rot[0][3] = 0.f;

	rot[1][0] = 0.0f;
	rot[1][1] = 1.f;
	rot[1][2] = 0.0f;
	rot[1][3] = 0.f;

	rot[2][0] = -sine;
	rot[2][1] = 0.0f;
	rot[2][2] = cosine;
	rot[2][3] = 0.f;

	rot[3][0] = 0.0f;
	rot[3][1] = 0.0f;
	rot[3][2] = 0.0f;
	rot[3][3] = 1.0f;


	Matrix trans;
	trans[0][0] = trans[1][1] = trans[2][2] = trans[3][3] = 1.f;
	trans[0][1] = trans[0][2] = 0.0f;
	trans[1][0] = trans[1][2] = 0.0f;
	trans[2][0] = trans[2][1] = 0.0f;
	trans[3][0] = trans[3][1] = trans[3][2] = 0.0f;

	trans[0][3] = -position[0];
	trans[1][3] = -position[1];
	trans[2][3] = -position[2];

	mulMat(rot, trans, invModel);
}

void createDirectionalInvModel(Matrix* invModel, float angleY){
	// Rotation matrix in opposite direction.
	float cosine = cos(-angleY);
	float sine = sin(-angleY);
	(*invModel)[0][0] = cosine;
	(*invModel)[0][1] = 0.0f;
	(*invModel)[0][2] = sine;
	(*invModel)[0][3] = 0.f;
	(*invModel)[1][0] = 0.0f;
	(*invModel)[1][1] = 1.f;
	(*invModel)[1][2] = 0.0f;
	(*invModel)[1][3] = 0.f;
	(*invModel)[2][0] = -sine;
	(*invModel)[2][1] = 0.0f;
	(*invModel)[2][2] = cosine;
	(*invModel)[2][3] = 0.f;
	(*invModel)[3][0] = 0.0f;
	(*invModel)[3][1] = 0.0f;
	(*invModel)[3][2] = 0.0f;
	(*invModel)[3][3] = 1.0f;

}
