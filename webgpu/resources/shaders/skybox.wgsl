
struct FrameInfos {
	proj: mat4x4f,
	lightVP: mat4x4f,
	lightDirViewSpace: vec3f,
};

struct ModelInfos {
	MV: mat4x4f,
	MVinverse: mat4x4f,
	M: mat4x4f,
	shininess: f32,
};

struct VertexIn {
	@location(0) pos: vec3f,
};

struct VertexOut {
	@builtin(position) projPos : vec4f,
	@location(0) uv: vec3f,
};

@group(0) @binding(0) var<uniform> frame: FrameInfos;
@group(0) @binding(1) var<uniform> model: ModelInfos;
@group(1) @binding(0) var cubeMap: texture_cube<f32>;
@group(2) @binding(0) var linearSampler: sampler;

@vertex
fn mainVertex(IN: VertexIn) -> VertexOut {
	var OUT : VertexOut;

	OUT.uv = IN.pos;

	// Remove the translation from the model-view matrix, so that the skybox stays centered on the viewer.
	var adjustedMV = model.MV;
	adjustedMV[3] = vec4f(0.0, 0.0, 0.0, 1.0);

	OUT.projPos = frame.proj * adjustedMV * vec4f(IN.pos, 1.0);

	// Send the skybox to the back of the scene (see skybox pipeline creation for more details).
	OUT.projPos.z = OUT.projPos.w;

	return OUT;
}


@fragment
fn mainFragment(IN: VertexOut) -> @location(0) vec4f {
	let color = textureSample(cubeMap, linearSampler, IN.uv).rgb;
	return vec4f(color, 1.0);
}