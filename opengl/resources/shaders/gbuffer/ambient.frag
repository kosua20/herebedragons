#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the texture, inverse of the screen size, FXAA flag.
uniform sampler2D albedoTexture;
uniform sampler2D normalTexture;
uniform sampler2D ssaoTexture;
uniform samplerCube textureCubeMapSmall;

uniform vec2 inverseScreenSize;
uniform mat4 inverseV;

// Output: the fragment color
out vec3 fragColor;


void main(){
	
	vec4 albedo = texture(albedoTexture,In.uv);
	
	if(albedo.a == 0){
		// If background (skybox), use directly the diffuse color.
		fragColor = albedo.rgb;
		return;
	}
	
	// Read AO.
	float ao = texture(ssaoTexture, In.uv).r;
	
	// Compute world  normal and use it to read into the convolved envmap.
	vec3 n = 2.0 * texture(normalTexture,In.uv).rgb - 1.0;
	vec3 worldNormal = vec3(inverseV * vec4(n,0.0));
	vec3 ambientLightColor = texture(textureCubeMapSmall,normalize(worldNormal)).rgb;
	
	fragColor = ao * ambientLightColor * albedo.rgb;
	
}
