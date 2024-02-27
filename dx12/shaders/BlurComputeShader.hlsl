
Texture2D<float2> inputTexture : register(t0);
RWTexture2D<float2> outputTexture : register(u0);

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
	float2 color = 0.0f.xx;
	for(int y = -2; y <= 2; ++y){
		for(int x = -2; x <= 2; ++x){
			color += inputTexture[(int2)id.xy + int2(x, y)];
		}
	}
	color /= 25.0f;
	outputTexture[id.xy] = color;
}