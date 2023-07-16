cmake_minimum_required(VERSION 3.1...3.25)
project(
	DragonWebGPU
	VERSION 1.0.0
	LANGUAGES CXX C
)

# Disable extra GLFW targets.
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)

# Add libraries
add_subdirectory(libs/glfw)
add_subdirectory(libs/webgpu)
add_subdirectory(libs/glfw3webgpu)

file(GLOB CORE_SOURCES "src/*.cpp")
file(GLOB CORE_HEADERS "src/*.hpp")

file(GLOB SUBDIR_SOURCES "src/*/*.cpp")
file(GLOB SUBDIR_HEADERS "src/*/*.hpp")

# Application
add_executable(DragonWebGPU
	${CORE_SOURCES}
	${CORE_HEADERS}
	${SUBDIR_SOURCES}
	${SUBDIR_HEADERS}
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src/ FILES
	${CORE_SOURCES}
	${CORE_HEADERS}
	${SUBDIR_SOURCES}
	${SUBDIR_HEADERS}
 )

target_include_directories(DragonWebGPU PRIVATE libs/glm)
# Add the 'webgpu' target as a dependency of our project
target_link_libraries(DragonWebGPU PRIVATE glfw webgpu glfw3webgpu)

set_target_properties(DragonWebGPU PROPERTIES CXX_STANDARD 17)
set_target_properties(DragonWebGPU PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
# Helper from the Dawn redistribution by Elie Michel 
# (technically not needed but keeping it if people want to substitute other WebGPU backends)
# The application's binary must find wgpu.dll or libwgpu.so at runtime,
# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
# next to the binary.
target_copy_webgpu_binaries(DragonWebGPU)