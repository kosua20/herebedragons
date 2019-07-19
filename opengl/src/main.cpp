#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h> // to set up the OpenGL context and manage window lifecycle and inputs

#include <stdio.h>
#include <iostream>
#include <memory>

#include "Renderer.h"

#define INITIAL_SIZE_WIDTH 800
#define INITIAL_SIZE_HEIGHT 600



/// Callbacks

void resize_callback(GLFWwindow* window, int width, int height){
	Renderer *rendererPtr = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	rendererPtr->resize(width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	// Handle quitting
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){ 
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}
	Renderer *rendererPtr = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	rendererPtr->keyPressed(key, action);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
	double x, y;
    glfwGetCursorPos(window, &x, &y);
	Renderer *rendererPtr = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	rendererPtr->buttonPressed(button, action, x, y);
}


void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos){
	bool left = glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
	bool right = glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
	Renderer *rendererPtr = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	rendererPtr->mousePosition(xpos, ypos, left, right);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	// Do nothing for now
	// ...
}


/// The main function

int main () {
	// Initialize glfw, which will create and setup an OpenGL context.
	if (!glfwInit()) {
		std::cerr << "ERROR: could not start GLFW3" << std::endl;
		return 1;
	}

	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	

	// Create a window with a given size. Width and height are macros as we will need them again.
	GLFWwindow* window = glfwCreateWindow(INITIAL_SIZE_WIDTH, INITIAL_SIZE_HEIGHT,"GL_Template", NULL, NULL);
	if (!window) {
		std::cerr << "ERROR: could not open window with GLFW3" << std::endl;
		glfwTerminate();
		return 1;
	}

	// Bind the OpenGL context and the new window.
	glfwMakeContextCurrent(window);

	if (gl3wInit()) {
		std::cerr << "Failed to initialize OpenGL" << std::endl;
		return -1;
	}
	if (!gl3wIsSupported(3, 2)) {
		std::cerr << "OpenGL 3.2 not supported\n" << std::endl;
		return -1;
	}

	// Create the renderer.
	Renderer renderer = Renderer(INITIAL_SIZE_WIDTH,INITIAL_SIZE_HEIGHT);
	
	glfwSetWindowUserPointer(window, &renderer);
	// Setup callbacks for various interactions and inputs.
	glfwSetFramebufferSizeCallback(window, resize_callback);	// Resizing the window
	glfwSetKeyCallback(window,key_callback);					// Pressing a key
	glfwSetMouseButtonCallback(window,mouse_button_callback);	// Clicking the mouse buttons
	glfwSetCursorPosCallback(window,cursor_pos_callback);		// Moving the cursor
	glfwSetScrollCallback(window,scroll_callback);				// Scrolling
	
	// On HiDPI screens, we might have to initially resize the framebuffers size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	renderer.resize(width, height);
	
	// Start the display/interaction loop.
	while (!glfwWindowShouldClose(window)) {

		// Update the content of the window.
		renderer.draw();
		
		//Display the result fo the current rendering loop.
		glfwSwapBuffers(window);

		// Update events (inputs,...).
		glfwPollEvents();
	}

	// Remove the window.
	glfwDestroyWindow(window);
	// Clean other resources
	renderer.clean();
	// Close GL context and any other GLFW resources.
	glfwTerminate();
	return 0;
}


