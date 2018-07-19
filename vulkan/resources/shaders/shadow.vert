#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inTexCoord;

layout(binding = 0) uniform LightInfos {
	mat4 viewproj;
	vec3 viewSpaceDir;
} light;

layout(push_constant) uniform ModelInfos {
	mat4 model;
	float shininess;
} object;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = light.viewproj * object.model * vec4(inPosition, 1.0);
}
