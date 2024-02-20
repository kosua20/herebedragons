
struct VertexOut
{
	float4 pos : SV_POSITION;
	float3 uv : TEXCOORD0;
};

TextureCube cubeTexture : register(t0);
SamplerState objectSampler: register(s0);

float4 main(VertexOut vOut) : SV_TARGET
{
	return float4(cubeTexture.Sample(objectSampler, normalize(vOut.uv)).rgb, 1.0f);
}