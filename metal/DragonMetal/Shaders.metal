//
//  Shaders.metal
//  DragonMetal
//
//  Created by Simon Rodriguez on 27/10/2016.
//  Copyright (c) 2016 Simon Rodriguez. All rights reserved.
//

#include <metal_stdlib>
using namespace metal;


// Constants.

struct GlobalConstants {
	float4 lightDir;
};

struct ObjectConstants {
	float4x4 mvp;
	float4x4 invmv;
	float4x4 mv;
	float4x4 mvpLight;
};

struct MaterialConstants {
	int shininess;
};


// Inputs/outputs.

struct VertexIn {
	float3 position;
	float3 normal;
	float3 tangent;
	float3 bitangent;
	float2 textureCoordinate;
};

struct VertexOut {
    float4 position [[position]];
	float4 lightSpacePos;
	float3 t;
	float3 b;
	float3 n;
	float3 viewSpacePos;
	float2 uv;
};

struct CubemapOut {
	float4 position [[position]];
	float3 uv;
};


// Samplers.

constexpr sampler samplerTexture(coord::normalized, address::clamp_to_edge, filter::linear);
constexpr sampler samplerDepth(coord::normalized, address::clamp_to_edge, filter::linear, compare_func::less);


// Shadow pass shaders.

vertex float4 shadowVertex(device VertexIn *vertices [[buffer(0)]],
						   constant ObjectConstants &objectUniforms [[buffer(1)]],
						   uint vid [[vertex_id]]) {
	// Output a simple float4 position.
	float4 position = objectUniforms.mvpLight * float4(vertices[vid].position, 1.0);
	return position;
}


// Objects pass shaders.

vertex VertexOut objectVertex(device VertexIn *vertices [[buffer(0)]],
							  constant ObjectConstants &objectUniforms [[buffer(1)]],
							  uint vid [[vertex_id]]) {
    VertexOut outVertex;
	
	// Position transformations.
    outVertex.position = objectUniforms.mvp * float4(vertices[vid].position, 1.0);
	outVertex.viewSpacePos = (objectUniforms.mv * float4(vertices[vid].position, 1.0)).xyz;
	outVertex.lightSpacePos = objectUniforms.mvpLight * float4(vertices[vid].position, 1.0);
	
	// Matrice to transform the normal map from tangent space to view space.
	float3x3 normalMatrix = float3x3(objectUniforms.invmv[0].xyz,
									 objectUniforms.invmv[1].xyz,
									 objectUniforms.invmv[2].xyz );
	outVertex.n = normalize(normalMatrix * float3(vertices[vid].normal));
	outVertex.t = normalize(normalMatrix * float3(vertices[vid].tangent));
	outVertex.b = normalize(normalMatrix * float3(vertices[vid].bitangent));
	
	// Texture coordinates.
	outVertex.uv = vertices[vid].textureCoordinate;
	
	return outVertex;
};




fragment float4 objectFragment(VertexOut fragmentIn [[stage_in]],
							   constant GlobalConstants &globalUniforms [[buffer(0)]],
							   constant MaterialConstants &materialUniforms [[buffer(1)]],
							   texture2d<half, access::sample> colorMap [[texture(0)]],
							   texture2d<float, access::sample> normalMap [[texture(1)]],
							   depth2d<float, access::sample> shadowMap [[texture(2)]]) {
	// Base color.
	half3 albedo = colorMap.sample(samplerTexture, fragmentIn.uv).rgb;
	
	// Compute normal in view space.
	float3 n = normalize(2.0 * normalMap.sample(samplerTexture, fragmentIn.uv).rgb - 1.0);
	float3x3 tbn = float3x3(fragmentIn.t, fragmentIn.b, fragmentIn.n);
	n = normalize(tbn * n);
	// Light dir.
	float3 l = float3(normalize(globalUniforms.lightDir.xyz));
	
	// Shadow map.
	// Compute UV in shadow map.
	float2 shadowUV = fragmentIn.lightSpacePos.xy / fragmentIn.lightSpacePos.w * 0.5 + 0.5;
	shadowUV.y = 1.0 - shadowUV.y;
	// Read both depths.
	float lightDepth = shadowMap.sample(samplerDepth, shadowUV);
	float currentDepth = fragmentIn.lightSpacePos.z / fragmentIn.lightSpacePos.w;
	// Compare depth.
	float shadowFactor = 0.2;
	if(currentDepth - 0.001 < lightDepth){
		// We are not in shadow is the point is closer than the corresponding point in the shadow map.
		shadowFactor = 1.0;
	}

	// Phong lighting.
	// Ambient term.
	float3 color = 0.1 * float3(albedo);
	// Diffuse term.
	float diffuse = max(0.0, dot(n, l));
	color += shadowFactor * diffuse * float3(albedo);
	// Specular term.
	if(diffuse > 0.0){
		float3 v = normalize(-fragmentIn.viewSpacePos);
		float3 r = reflect(-l, n);
		float specular = pow(max(dot(r, v), 0.0), materialUniforms.shininess);
		color += shadowFactor*specular;
	}
	
	return float4(color,1.0);
};


// Cubemap pass shaders.

vertex CubemapOut skyboxVertex(device VertexIn *vertices [[buffer(0)]],
							   constant ObjectConstants &objectUniforms [[buffer(1)]],
							   uint vid [[vertex_id]]) {
	CubemapOut outVertex;
	outVertex.position = objectUniforms.mvp * float4(vertices[vid].position, 1.0);
	outVertex.uv = vertices[vid].position;
	return outVertex;
};

fragment float4 skyboxFragment(CubemapOut fragmentIn [[stage_in]],
							   constant GlobalConstants &globalUniforms [[buffer(0)]],
							   texturecube<float, access::sample> tex [[texture(0)]]) {
	// Access cubemap using 3D texture coordinates.
	float3 color = tex.sample(samplerTexture, fragmentIn.uv).rgb;
	return float4(color,1.0);
};
