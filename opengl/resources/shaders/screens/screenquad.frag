#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the texture, inverse of the screen size, FXAA flag.
uniform sampler2D screenTexture;
uniform vec2 inverseScreenSize;

// Output: the fragment color
out vec3 fragColor;


void main(){
	
	vec3 finalColor = texture(screenTexture,In.uv).rgb;
	fragColor = finalColor;
	
}
