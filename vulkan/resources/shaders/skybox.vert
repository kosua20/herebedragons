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

layout(push_constant) uniform ModelInfos {
	mat4 model;
	float shininess;
} object;


layout(location = 0) out vec3 fragUv;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	fragUv = inPosition;
	// Remove the translation from the view matrix, so that the skybox stays centered on the viewer.
	mat4 fixedView = cam.view;
	fixedView[3] = vec4(0.0,0.0,0.0,1.0);
	gl_Position = cam.proj * fixedView * object.model * vec4(inPosition, 1.0);
	// Send the skybox to the back of the scene (see skybox pipeline creation for more details).
	gl_Position.z = gl_Position.w; 
}
