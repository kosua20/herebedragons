
Texture2D<float4> inputTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
	uint2 shift = uint2(0,1);
	float4 color = 0.0f.xxxx;
	color += inputTexture[2 * id.xy + shift.xx];
	color += inputTexture[2 * id.xy + shift.xy];
	color += inputTexture[2 * id.xy + shift.yx];
	color += inputTexture[2 * id.xy + shift.yy];
	color /= 4.0f;
	outputTexture[id.xy] = color;
}