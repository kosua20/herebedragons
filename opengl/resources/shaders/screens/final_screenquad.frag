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
	vec3 down = textureOffset(screenTexture,In.uv,ivec2(0,-1)).rgb;
	vec3 up = textureOffset(screenTexture,In.uv,ivec2(0,1)).rgb;
	vec3 left = textureOffset(screenTexture,In.uv,ivec2(-1,0)).rgb;
	vec3 right = textureOffset(screenTexture,In.uv,ivec2(1,0)).rgb;

	fragColor = clamp(finalColor + 0.4*(4 * finalColor - down - up - left - right),0.0,1.0);
	
}
