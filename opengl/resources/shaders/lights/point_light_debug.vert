#version 330

// Attributes
layout(location = 0) in vec3 v;

uniform mat4 mvp;
uniform vec3 lightWorldPosition;
uniform float radius;

void main(){
	
	// We directly output the position.
	gl_Position = mvp*vec4(radius*v+lightWorldPosition, 1.0);

}
