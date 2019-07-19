#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniform: the light structure (position in view space)
layout (std140) uniform Light {
	vec4 position;
	vec4 Ia;
	vec4 Id;
	vec4 Is;
	float shininess;
} light;


// Uniforms
uniform sampler2D albedoTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;
uniform sampler2D effectsTexture;
uniform samplerCube textureCubeMap;
uniform samplerCube textureCubeMapSmall;
uniform sampler2D shadowMap;

uniform vec2 inverseScreenSize;
uniform vec4 projectionMatrix;
uniform mat4 inverseV;
uniform mat4 lightVP;

// Output: the fragment color
out vec3 fragColor;

vec3 positionFromDepth(float depth){
	float depth2 = 2.0 * depth - 1.0 ;
	vec2 ndcPos = 2.0 * In.uv - 1.0;
	// Linearize depth -> in view space.
	float viewDepth = - projectionMatrix.w / (depth2 + projectionMatrix.z);
	// Compute the x and y components in view space.
	return vec3(- ndcPos * viewDepth / projectionMatrix.xy , viewDepth);
}


// Compute the light shading.

vec3 shading(vec3 diffuseColor, vec3 n, vec3 v, vec3 position, vec3 lightPosition, float lightShininess, vec3 lightColor, float specularOcclusion, out vec3 ambient){
	
	// Compute the direction from the point to the light
	// light.position.w == 0 if the light is directional, 1 else.
	vec3 d = normalize(lightPosition - light.position.w * position);
	
	
	vec3 worldNormal = vec3(inverseV * vec4(n,0.0));
	vec3 ambientLightColor = texture(textureCubeMapSmall,normalize(worldNormal)).rgb;
	diffuseColor = mix(diffuseColor, diffuseColor * ambientLightColor, 0.5);
	
	// The ambient factor
	ambient = 0.3 * diffuseColor;
	
	// Compute the diffuse factor
	float diffuse = max(0.0, dot(d,n));
	
	// Compute the specular factor
	float specular = 0.0;
	if(diffuse > 0.0){
		vec3 r = reflect(-d,n);
		specular = pow(max(dot(r,v),0.0),lightShininess);
		specular *= specularOcclusion;
	}
	
	return diffuse * diffuseColor + specular * lightColor;
}


// Compute the shadow multiplicator based on shadow map.

float shadow(vec3 lightSpacePosition){
	float probabilityMax = 1.0;
	if (lightSpacePosition.z < 1.0){
		// Read first and second moment from shadow map.
		vec2 moments = texture(shadowMap, lightSpacePosition.xy).rg;
		// Initial probability of light.
		float probability = float(lightSpacePosition.z <= moments.x);
		// Compute variance.
		float variance = moments.y - (moments.x * moments.x);
		variance = max(variance, 0.00001);
		// Delta of depth.
		float d = lightSpacePosition.z - moments.x;
		// Use Chebyshev to estimate bound on probability.
		probabilityMax = variance / (variance + d*d);
		probabilityMax = max(probability, probabilityMax);
		// Limit light bleeding by rescaling and clamping the probability factor.
		probabilityMax = clamp( (probabilityMax - 0.1) / (1.0 - 0.1), 0.0, 1.0);
	}
	return probabilityMax;
}


void main(){
	
	vec4 albedo =  texture(albedoTexture,In.uv);
	// If this is the skybox, simply output the color.
	if(albedo.a == 0.0){
		fragColor = albedo.rgb;
		return;
	}
	
	vec3 diffuseColor = albedo.rgb;
	vec3 n = 2.0 * texture(normalTexture,In.uv).rgb - 1.0;
	float depth = texture(depthTexture,In.uv).r;
	vec3 position = positionFromDepth(depth);
	vec3 effects = texture(effectsTexture,In.uv).rgb;
	// If this is the plane, the effects texture contains the depth, manually define values.
	if(albedo.a == 2.0/255.0){
		effects = vec3(1.0,1.0,0.25*(1.0-effects.r));
	}
	
	vec3 v = normalize(-position);
	
	vec3 ambient;
	vec3 lightShading = shading(diffuseColor, n, v, position, light.position.xyz, light.shininess, light.Is.rgb, effects.g, ambient);
	
	// Compute the position in light space from the light VP matrix and the view-space position.
	vec3 lightSpacePosition = 0.5*(lightVP * inverseV * vec4(position,1.0)).xyz + 0.5;
	float shadowMultiplicator = shadow(lightSpacePosition);
	
	// Mix the ambient color (always present) with the light contribution, weighted by the shadow factor.
	fragColor = ambient * effects.r * light.Ia.rgb + shadowMultiplicator * lightShading;
	
	vec3 reflectionColor = vec3(0.0);
	if(effects.b > 0.0){
		vec3 rCubeMap = reflect(-v, n);
		rCubeMap = vec3(inverseV * vec4(rCubeMap,0.0));
		reflectionColor = texture(textureCubeMap,rCubeMap).rgb;
	}
	// Mix with the reflexion color.
	fragColor = mix(fragColor,reflectionColor,0.5*effects.b);
	
}

