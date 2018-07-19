#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragUv;

layout(binding = 1) uniform samplerCube colorMap;

layout(location = 0) out vec4 outColor;

void main(){
	vec3 color = texture(colorMap, fragUv).rgb;
	outColor = vec4(color, 1.0);
}
