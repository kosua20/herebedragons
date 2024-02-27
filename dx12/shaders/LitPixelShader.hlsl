cbuffer cbPerObject : register(b0)
{
    float4x4 MV;
    float4x4 invMV;
    float4x4 lightMVP;
	float shininess;
};

cbuffer cbPerFrame : register(b1)
{
	float4x4 P;
	float4 lightDirViewSpace;
}; 

struct VertexOut
{
	float4 pos : SV_POSITION;
	float3 t : TEXCOORD0;
	float3 b : TEXCOORD1;
	float3 n : TEXCOORD2;
	float3 viewPos : TEXCOORD3;
	float4 lightPos : TEXCOORD4;
	float2 uv : TEXCOORD5;
};

Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D shadowMap : register(t2);
SamplerState objectSampler: register(s0);
SamplerState shadowSampler: register(s1);

float estimateShadowingVSM(float4 lightSpacePos) {
	float3 lightSpaceNdc = lightSpacePos.xyz / lightSpacePos.w;
	if (any(abs(lightSpaceNdc) > 1.f)) {
		return 1.f;
	}

	float2 shadowUV = lightSpaceNdc.xy * 0.5 + 0.5;
	shadowUV.y = 1.0 - shadowUV.y;
	// Read first and second moment from shadow map.
	float2 moments = shadowMap.Sample(shadowSampler, shadowUV).rg;

	// Initial probability of light.
	float probability = float(lightSpacePos.z <= moments.x);
	// Compute variance.
	float variance = moments.y - (moments.x * moments.x);
	variance = max(variance, 0.00008f);
	// Delta of depth.
	float d = lightSpacePos.z - moments.x;
	// Use Chebyshev to estimate bound on probability.
	float probabilityMax = variance / (variance + d*d);
	probabilityMax = max(probability, probabilityMax);
	// Limit light bleeding by rescaling and clamping the probability factor.
	probabilityMax = clamp( (probabilityMax - 0.1) / (1.0 - 0.1), 0.0, 1.0);
	return probabilityMax;
}

float4 main(VertexOut vOut) : SV_TARGET
{
	float3 albedo = diffuseTexture.Sample(objectSampler, vOut.uv).rgb;

	float3 n = normalTexture.Sample(objectSampler, vOut.uv).rgb * 2.f - 1.f;
	n = normalize(n);
	float3x3 tbn = float3x3(vOut.t, vOut.b, vOut.n);
	n = normalize(mul(n, tbn));
	
	float3 l = normalize(lightDirViewSpace.xyz);

	float shadowFactor = estimateShadowingVSM(vOut.lightPos);
	
	// Phong lighting.
	// Ambient term.
	float3 color = 0.1f * albedo;
	// Diffuse term.
	float diffuse = max(0.0f, dot(n, l));
	color += shadowFactor * diffuse * albedo;
	// Specular term.
	if (diffuse > 0.0f) {
		float3 v = normalize(-vOut.viewPos);
		float3 r = reflect(-l, n);
		float3 specular = pow(max(dot(r, v), 0.0f), shininess);
		color += shadowFactor * specular;
	}
	
	return float4(color, 1.0f);
}