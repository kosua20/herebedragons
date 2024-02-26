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
	float3 uv : TEXCOORD0;
};

VertexOut main(VertexIn vIn)
{
	VertexOut vOut;
	vOut.uv = vIn.pos;

	vOut.pos = mul(float4(vIn.pos, 1.f), MVP);
	
	// Send the skybox to the back of the scene.
	vOut.pos.z = vOut.pos.w;

	return vOut;
}