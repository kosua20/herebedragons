#version 330

// Input: position in model space
in INTERFACE {
	vec3 position; 
} In ;

uniform samplerCube textureCubeMap;

// Output: the fragment color
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 fragEffects;

void main(){
	
	fragColor.rgb = texture(textureCubeMap,In.position).rgb;
	fragColor.a = 0.0;
	fragNormal = vec3(0.5);
	fragEffects = vec3(0.0);

}
