struct VertexOut
{
	float4 pos : SV_POSITION;
};

float2 main(VertexOut vIn) : SV_TARGET
{
	float z = vIn.pos.z;
	return float2(z, z * z);
}