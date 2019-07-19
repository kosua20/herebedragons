#version 330

// Attributes
layout(location = 0) in vec3 v;
layout(location = 1) in vec3 n;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tang;
layout(location = 4) in vec3 binor;

// Uniform: the MVP, MV and normal matrices
uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 normalMatrix;

// Output: tangent space matrix, position in view space and uv.
out INTERFACE {
    mat3 tbn;
	vec3 tangentSpacePosition;
	vec3 viewSpacePosition;
	vec2 uv;
} Out ;


void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	Out.uv = uv;

	// Compute the TBN matrix (from tangent space to view space).
	vec3 T = normalize(normalMatrix * tang);
	vec3 B = normalize(normalMatrix * binor);
	vec3 N = normalize(normalMatrix * n);
	Out.tbn = mat3(T, B, N);
	
	Out.viewSpacePosition = (mv * vec4(v,1.0)).xyz;
	Out.tangentSpacePosition = transpose(Out.tbn) * Out.viewSpacePosition;
	
}
