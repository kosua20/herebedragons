
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
	@location(1) normal: vec3f,
	@location(2) tangent: vec3f,
	@location(3) bitangent: vec3f,
	@location(4) uv: vec2f,
};

struct VertexShadowIn {
	@location(0) pos: vec3f,
};

struct VertexOut {
	@builtin(position) projPos : vec4f,
	@location(0) lightSpacePos : vec4f,
	@location(1) viewSpacePos : vec3f,
	@location(3) t: vec3f, 
	@location(4) b: vec3f, 
	@location(5) n: vec3f, 
	@location(6) uv: vec2f,	
};

@group(0) @binding(0) var<uniform> frame: FrameInfos;
@group(0) @binding(1) var<uniform> model: ModelInfos;
@group(1) @binding(0) var albedoMap: texture_2d<f32>;
@group(1) @binding(1) var normalMap: texture_2d<f32>;
@group(2) @binding(0) var linearSampler: sampler;
@group(2) @binding(1) var compareSampler: sampler_comparison;
@group(2) @binding(2) var shadowMap: texture_depth_2d;

@vertex
fn mainVertex(IN: VertexIn) -> VertexOut {
	var OUT : VertexOut;

	// Positions
	var viewPos = model.MV * vec4f(IN.pos, 1.);
	OUT.projPos = frame.proj * viewPos;
	OUT.viewSpacePos = viewPos.xyz;
	OUT.lightSpacePos = frame.lightVP * model.M * vec4f(IN.pos, 1.);

	// Texcoords
	OUT.uv = IN.uv;

	// Normal matrix
	let normalMat = mat3x3f(model.MVinverse[0].xyz, model.MVinverse[1].xyz, model.MVinverse[2].xyz);
	OUT.t = normalize(normalMat * IN.tangent);
	OUT.b = normalize(normalMat * IN.bitangent);
	OUT.n = normalize(normalMat * IN.normal);
	
	return OUT;
}

@vertex
fn mainVertexShadow(IN: VertexShadowIn) -> @builtin(position) vec4f {
	return frame.lightVP * model.M * vec4(IN.pos, 1.0);
}

fn estimateShadowing(lightSpacePos: vec4f) -> f32 {
	let lightSpaceNdc = lightSpacePos.xyz/lightSpacePos.w;
	var shadowUV = lightSpaceNdc.xy * 0.5 + 0.5;
	shadowUV.y = 1.0 - shadowUV.y;
	// Compare depth (has to be outside non-uniform flow).
	let lightDepth = textureSampleCompareLevel(shadowMap, compareSampler, shadowUV, lightSpaceNdc.z);
	// Exit if outside.
	if(any(abs(lightSpaceNdc) > vec3f(1.0))){
		return 1.0;
	}
	return lightDepth;
}

@fragment
fn mainFragment(IN: VertexOut) -> @location(0) vec4f {
	let albedo = textureSample(albedoMap, linearSampler, IN.uv).rgb;

	// Compute normal in view space.
	var n = textureSample(normalMap, linearSampler, IN.uv).rgb * 2.0 - 1.0;
	let TBN = mat3x3f(IN.t, IN.b, IN.n);
	n = normalize(TBN * normalize(n));
	// Light dir.
	let l = normalize(frame.lightDirViewSpace);
	
	// Shadowing
	let shadowFactor = estimateShadowing(IN.lightSpacePos);

	// Phong lighting.
	// Ambient term.
	var color = 0.1 * albedo;
	// Diffuse term.
	let diffuse = max(0.0, dot(n, l));
	color += shadowFactor * diffuse * albedo;
	// Specular term.
	if(diffuse > 0.0){
		let v = normalize(-IN.viewSpacePos);
		let r = reflect(-l, n);
		let specular = pow(max(dot(r, v), 0.0), model.shininess);
		color += shadowFactor*specular;
	}
	return vec4f(color, 1.0);
}