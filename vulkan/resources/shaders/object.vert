#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inTexCoord;

layout(binding = 0) uniform CameraInfos {
    mat4 view;
    mat4 proj;
} cam;

layout(binding = 3) uniform LightInfos {
	mat4 viewproj;
	vec3 viewSpaceDir;
} light;

layout(push_constant) uniform ModelInfos {
	mat4 model;
	float shininess;
} object;

layout(location = 0) out vec3 fragViewSpacePos;
layout(location = 1) out vec2 fragUv;
layout(location = 2) out vec4 fragLightSpacePos;
layout(location = 3) out mat3 fragTbn;


out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	
	mat4 modelView = cam.view * object.model;
	mat3 normalMat = transpose(inverse(mat3(modelView)));
	vec3 T = normalize(normalMat * inTangent);
	vec3 B = normalize(normalMat * inBitangent);
	vec3 N = normalize(normalMat * inNormal);
	
	vec4 viewSpacePos = modelView * vec4(inPosition, 1.0);
	fragViewSpacePos = viewSpacePos.xyz;
	fragUv = inTexCoord;
	fragTbn = mat3(T, B, N);
	
	fragLightSpacePos = light.viewproj * object.model * vec4(inPosition, 1.0);
	
	gl_Position = cam.proj * viewSpacePos;
	
}
