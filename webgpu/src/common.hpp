#ifndef common_h
#define common_h

#ifdef _WIN32
#define NOMINMAX
#endif

#include <GLFW/glfw3.h>
#include <dawn/webgpu.h>

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
#include <array>


#endif /* common_h */
