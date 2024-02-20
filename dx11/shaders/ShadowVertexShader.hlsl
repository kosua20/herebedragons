cbuffer cbPerObject : register(b0)
{
	float4x4 MVP;
};

struct VertexIn
{
	float3 pos : POSITION;
};

struct VertexOut
{
	float4 pos : SV_POSITION;
	float4 zPos : TEXCOORD0;
};

VertexOut main(VertexIn vIn)
{
	VertexOut vOut;
	vOut.pos = mul(float4(vIn.pos, 1.f), MVP);
	vOut.zPos = vOut.pos;
	return vOut;
}