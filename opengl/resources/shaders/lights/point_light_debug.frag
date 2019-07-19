#version 330

uniform vec3 lightColor;

// Output: the fragment color
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 fragEffects;

void main(){
	
	// Store values.
	fragColor.rgb = lightColor;
	fragColor.a = 0.0;
	fragNormal.rgb = vec3(0.5);
	fragEffects.rgb = vec3(0.0);
	
}
