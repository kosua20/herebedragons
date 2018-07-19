#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragViewSpacePos;
layout(location = 1) in vec2 fragUv;
layout(location = 2) in vec4 fragLightSpacePos;
layout(location = 3) in mat3 fragTbn;

layout(binding = 1) uniform sampler2D colorMap;
layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 4) uniform sampler2D shadowMap;

layout(binding = 3) uniform LightInfos {
	mat4 viewproj;
	vec3 viewSpaceDir;
} light;

layout(push_constant) uniform ModelInfos {
	mat4 model;
	float shininess;
} object;

layout(location = 0) out vec4 outColor;

float estimateShadowing(){
	vec3 lightSpaceNdc = fragLightSpacePos.xyz/fragLightSpacePos.w;
	if(any(greaterThan(abs(lightSpaceNdc), vec3(1.0)))){
		return 1.0;
	}
	vec2 shadowUV = lightSpaceNdc.xy * 0.5 + 0.5;
	// Read both depths.
	float lightDepth = texture(shadowMap, shadowUV).r;
	// Compare depth.
	if(lightSpaceNdc.z < lightDepth){
		// We are not in shadow is the point is closer than the corresponding point in the shadow map.
		return 1.0;
	}
	return 0.0;
}


void main() {
	// Base color.
	vec3 albedo = texture(colorMap, fragUv).rgb;
	
	// Compute normal in view space.
	vec3 n = normalize(2.0 * texture(normalMap, fragUv).rgb - 1.0);
	n = normalize(fragTbn * n);
	// Light dir.
	vec3 l = vec3(normalize(light.viewSpaceDir));
	
	// Shadowing
	float shadowFactor = estimateShadowing();
	
	// Phong lighting.
	// Ambient term.
	vec3 color = 0.1 * albedo;
	// Diffuse term.
	float diffuse = max(0.0, dot(n, l));
	color += shadowFactor * diffuse * albedo;
	// Specular term.
	if(diffuse > 0.0){
		vec3 v = normalize(-fragViewSpacePos);
		vec3 r = reflect(-l, n);
		float specular = pow(max(dot(r, v), 0.0), object.shininess);
		color += shadowFactor*specular;
	}
	outColor = vec4(color,1.0);
	
}
