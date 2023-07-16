
@group(0) @binding(0) var srcLevel: texture_2d<f32>;
@group(0) @binding(1) var dstLevel: texture_storage_2d<rgba8unorm, write>;

@compute @workgroup_size(8,8)
fn mainCompute(@builtin(global_invocation_id) id: vec3<u32>)  {
	let shift = vec2u(0,1);
	var color = vec4f(0.0);
	color += textureLoad(srcLevel, 2 * id.xy + shift.xx, 0);
	color += textureLoad(srcLevel, 2 * id.xy + shift.xy, 0);
	color += textureLoad(srcLevel, 2 * id.xy + shift.yx, 0);
	color += textureLoad(srcLevel, 2 * id.xy + shift.yy, 0);
	color /= 4.0;
	textureStore(dstLevel, id.xy, color);
}

