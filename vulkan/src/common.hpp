//
//  common.hpp
//  DragonVulkan
//
//  Created by Simon Rodriguez on 11/07/2018.
//  Copyright © 2018 Simon Rodriguez. All rights reserved.
//

#ifndef common_h
#define common_h

#ifdef _WIN32
#define NOMINMAX
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef _WIN32
#define M_PI	3.14159265358979323846
#endif

#include <iostream>
#include <vector>
#include <string>

// Uniforms structs.
struct CameraInfos {
	glm::mat4 view;
	glm::mat4 proj;
};

struct LightInfos {
	glm::mat4 mvp;
	glm::vec3 viewSpaceDir;
};

struct ObjectInfos {
	glm::mat4 model;
	float shininess;
};

#endif /* common_h */
