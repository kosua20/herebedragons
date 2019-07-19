#version 330

// Attributes
layout(location = 0) in vec3 v;

// Uniform
uniform mat4 mvp;

// Output: position in model space
out INTERFACE {
	vec3 position;
} Out ;


void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	Out.position = v;
	
}
