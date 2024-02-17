
struct VertexOut
{
	float4 pos : SV_POSITION;
	float3 uv : TEXCOORD0;
};

samplerCUBE cubeTexture : register(s0);

float4 main(VertexOut vOut) : SV_TARGET
{
	return float4(texCUBE(cubeTexture, normalize(vOut.uv)).rgb, 1.0f);
}