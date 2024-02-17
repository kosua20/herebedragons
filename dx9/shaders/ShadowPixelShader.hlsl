struct VertexOut
{
	float4 pos : SV_POSITION;
	float4 zPos : TEXCOORD0;
};

float4 main(VertexOut vOut) : SV_TARGET
{
	return vOut.zPos.zzzz / vOut.zPos.wwww;
}