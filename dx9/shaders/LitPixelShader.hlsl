cbuffer cbPerFrame : register(c0)
{
	float4 lightDirViewSpace : register(c0);
	float shininess : register(c1);
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

sampler2D diffuseTexture : register(s0);
sampler2D normalTexture : register(s1);
sampler2D shadowMap : register(s2);

float estimateShadowing(float4 lightSpacePos) {
	float3 lightSpaceNdc = lightSpacePos.xyz / lightSpacePos.w;
	float2 shadowUV = lightSpaceNdc.xy * 0.5 + 0.5;
	shadowUV.y = 1.0 - shadowUV.y;

	// Sample depth
	float lightDepth = tex2D(shadowMap, shadowUV).r;
	if (any(abs(lightSpaceNdc) > 1.f)) {
		return 1.f;
	}
	const float bias = 0.005f;
	return (lightSpaceNdc.z <= lightDepth + bias) ? 1.f : 0.f;
}

float4 main(VertexOut vOut) : SV_TARGET
{
	float3 albedo = tex2D(diffuseTexture, vOut.uv).rgb;

	float3 n = tex2D(normalTexture, vOut.uv).rgb * 2.f - 1.f;
	n = normalize(n);
	float3x3 tbn = float3x3(vOut.t, vOut.b, vOut.n);
	n = normalize(mul(n, tbn));
	
	float3 l = normalize(lightDirViewSpace.xyz);

	float shadowFactor = estimateShadowing(vOut.lightPos);
	
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