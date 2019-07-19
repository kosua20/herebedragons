#version 330

// Attributes
layout(location = 0) in vec3 v;

// Output: UV coordinates
out INTERFACE {
	vec2 uv;
} Out ;


void main(){
	
	// We directly output the position.
	gl_Position = vec4(v, 1.0);
	
	// Output the UV coordinates computed from the positions.
	Out.uv = v.xy * 0.5 + 0.5;
	
}
