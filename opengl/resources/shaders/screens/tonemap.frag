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

vec3 reinhard(vec3 hdrColor){
	return hdrColor / (1.0 + hdrColor);
}

vec3 simpleExposure(vec3 hdrColor, float exposure){
	return 1.0 - exp(-hdrColor * exposure);
}

vec3 aces(vec3 hdrColor){
	return clamp(((0.9036 * hdrColor + 0.018) * hdrColor) / ((0.8748 * hdrColor + 0.354) * hdrColor + 0.14), 0.0, 1.0);
}

vec3 cineon(vec3 hdrColor){
	vec3 shiftedColor = max(vec3(0.0), hdrColor - 0.004);
	return (shiftedColor * (6.2 * shiftedColor + 0.5)) / (shiftedColor * (6.2 * shiftedColor + 1.7) + 0.06);
}

vec3 uncharted2(vec3 hdrColor){
	vec3 x = 2.0 * hdrColor; // Exposure bias.
	vec3 newColor = ((x * (0.15 * x + 0.05) + 0.004) / (x * (0.15 * x + 0.5) + 0.06)) - 0.02/0.3;
	return newColor * 1.3790642467; // White scale
}

void main(){
	
	vec3 finalColor = texture(screenTexture,In.uv).rgb;
	fragColor = uncharted2(finalColor);
	
	// Test if any component is still > 1, for demo purposes.
	//fragColor = any(greaterThan(fragColor, vec3(1.0))) ? vec3(1.0,0.0,0.0) : fragColor;
	
}
