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

struct VertexIn
{
	float3 pos : POSITION;
	float3 n : NORMAL;
	float3 t : TANGENT;
	float3 b : BINORMAL;
	float2 uv : TEXCOORD;
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

VertexOut main( VertexIn vIn )
{
	VertexOut vOut;

	float4 viewPos = mul(float4(vIn.pos, 1.f), MV);
	vOut.viewPos = viewPos.xyz;
	vOut.pos = mul(viewPos, P);

	vOut.lightPos = mul(float4(vIn.pos, 1.f), lightMVP);

	vOut.uv = vIn.uv;

	float3x3 nMat = (float3x3)invMV;
	vOut.t = normalize(mul(vIn.t, nMat));
	vOut.b = normalize(mul(vIn.b, nMat));
	vOut.n = normalize(mul(vIn.n, nMat));
	
	return vOut;
}