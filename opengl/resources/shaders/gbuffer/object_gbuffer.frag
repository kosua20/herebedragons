#version 330

// Input: tangent space matrix, position (view space) and uv coming from the vertex shader
in INTERFACE {
    mat3 tbn;
	vec3 tangentSpacePosition;
	vec3 viewSpacePosition;
	vec2 uv;
} In ;

uniform sampler2D textureColor;
uniform sampler2D textureNormal;
uniform sampler2D textureEffects;
uniform mat4 p;
uniform int materialId;

#define PARALLAX_MIN 8
#define PARALLAX_MAX 32
#define PARALLAX_SCALE 0.04

// Output: the fragment color
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 fragEffects;


vec2 parallax(vec2 uv, vec3 vTangentDir, out vec2 positionShift){
	
	// We can adapt the layer count based on the view direction. If we are straight above the surface, we don't need many layers.
	float layersCount = mix(PARALLAX_MAX, PARALLAX_MIN, abs(vTangentDir.z));
	// Depth will vary between 0 and 1.
	float layerHeight = 1.0 / layersCount;
	float currentLayer = 0.0;
	// Initial depth at the given position.
	float currentDepth = texture(textureEffects, uv).z;
	
	// Step vector: in tangent space, we walk on the surface, in the (X,Y) plane.
	vec2 shift = PARALLAX_SCALE * vTangentDir.xy;
	// This shift corresponds to a UV shift, scaled depending on the height of a layer and the vertical coordinate of the view direction.
	vec2 shiftUV = shift / vTangentDir.z * layerHeight;
	vec2 newUV = uv;
	
	// While the current layer is above the surface (ie smaller than depth), we march.
	while (currentLayer < currentDepth) {
		// We update the UV, going further away from the viewer.
		newUV -= shiftUV;
		// Update current depth.
		currentDepth = texture(textureEffects,newUV).z;
		// Update current layer.
		currentLayer += layerHeight;
	}
	
	// Perform interpolation between the current depth layer and the previous one to refine the UV shift.
	vec2 previousNewUV = newUV + shiftUV;
	// The local depth is the gap between the current depth and the current depth layer.
	float currentLocalDepth = currentDepth - currentLayer;
	float previousLocalDepth = texture(textureEffects,previousNewUV).z - (currentLayer - layerHeight);
	
	
	// Interpolate between the two local depths to obtain the correct UV shift.
	vec2 finalUV = mix(newUV,previousNewUV,currentLocalDepth / (currentLocalDepth - previousLocalDepth));
	positionShift = (uv - finalUV) * vTangentDir.z / layerHeight;
	return finalUV;
}

void main(){
	
	vec2 localUV = In.uv;
	vec2 positionShift;
	
	// If parallax mapping is enabled, compute the new uvs, and use them for the remaining steps.
	if(materialId == 2){
		vec3 vTangentDir = normalize(- In.tangentSpacePosition);
		localUV = parallax(localUV, vTangentDir, positionShift);
		// If UV are outside the texture ([0,1]), we discard the fragment.
		if(localUV.x > 1.0 || localUV.y  > 1.0 || localUV.x < 0.0 || localUV.y < 0.0){
			discard;
		}
	}
	
	// Compute the normal at the fragment using the tangent space matrix and the normal read in the normal map.
	vec3 n = texture(textureNormal,localUV).rgb;
	n = normalize(n * 2.0 - 1.0);
	
	// Store values.
	fragColor.rgb = texture(textureColor, localUV).rgb;
	fragColor.a = float(materialId)/255.0;
	fragNormal.rgb = normalize(In.tbn * n)*0.5+0.5;
	fragEffects.rgb = texture(textureEffects,localUV).rgb;
	
	// Store depth manually (see below).
	gl_FragDepth = gl_FragCoord.z;
	// If parallax mapping is enabled, update the depth using the heightmap and the displacement applied.
	if(materialId == 2){
		fragEffects.g = 1.0;
		// Read the depth.
		float localDepth = fragEffects.r;
		// Convert the 3D shift applied from tangent space to view space.
		vec3 shift = In.tbn * vec3(positionShift.xy, -PARALLAX_SCALE * localDepth);
		// Update the depth in view space.
		vec3 newViewSpacePosition = In.viewSpacePosition - vec3(0.0,0.0, shift.z);
		// Back to clip space.
		vec4 clipPos = p * vec4(newViewSpacePosition,1.0);
		// Perpsective division.
		float newDepth = clipPos.z / clipPos.w;
		// Update the fragment depth, taking into account the depth range parameters.
		gl_FragDepth = ((gl_DepthRange.diff * newDepth) + gl_DepthRange.near + gl_DepthRange.far)/2.0;
	}
	
}
