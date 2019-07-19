#include <stdio.h>
#include <iostream>
#include <vector>
// glm additional header to generate transformation matrices directly.
#include <glm/gtc/matrix_transform.hpp>
#include <cstring> // For memcopy depending on the platform.

#include "Renderer.h"


Renderer::~Renderer(){}

Renderer::Renderer(int width, int height){

	// Initialize the timer.
	_timer = glfwGetTime();
	// Initialize random generator;
	Random::seed();
	// Setup projection matrix.
	_camera.screen(width, height);
	
	const int renderWidth = (int)_camera.renderSize()[0];
	const int renderHeight = (int)_camera.renderSize()[1];
	const int renderHalfWidth = (int)(0.5f * _camera.renderSize()[0]);
	const int renderHalfHeight = (int)(0.5f * _camera.renderSize()[1]);
	_gbuffer = std::make_shared<Gbuffer>(renderWidth, renderHeight);
	_ssaoFramebuffer = std::make_shared<Framebuffer>(renderHalfWidth, renderHalfHeight, GL_RED, GL_UNSIGNED_BYTE, GL_RED, GL_LINEAR, GL_CLAMP_TO_EDGE);
	_ssaoBlurFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RED, GL_UNSIGNED_BYTE, GL_RED, GL_LINEAR, GL_CLAMP_TO_EDGE);
	_sceneFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RGBA, GL_FLOAT, GL_RGBA16F, GL_LINEAR,GL_CLAMP_TO_EDGE);
	_toneMappingFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA, GL_LINEAR,GL_CLAMP_TO_EDGE);
	_fxaaFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RGBA,GL_UNSIGNED_BYTE, GL_RGBA, GL_LINEAR,GL_CLAMP_TO_EDGE);
	
	// Create directional light.
	_directionalLights.emplace_back(glm::vec3(0.0f), glm::vec3(2.0f), glm::ortho(-0.75f,0.75f,-0.75f,0.75f,2.0f,6.0f));
	
	// Create point lights.
	const float lI = 6.0; // Light intensity.
	std::vector<glm::vec3> colors = { glm::vec3(lI,0.0,0.0), glm::vec3(0.0,lI,0.0), glm::vec3(0.0,0.0,lI), glm::vec3(lI,lI,0.0)};
	
	PointLight::loadProgramAndGeometry();
	
	// Query the renderer identifier, and the supported OpenGL version.
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	std::cout << "Renderer: " << renderer << std::endl;
	std::cout << "OpenGL version supported: " << version << std::endl;
	checkGLError();

	// GL options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glBlendEquation (GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	checkGLError();

	// Initialize objects.
	_suzanne.init( "suzanne", {"suzanne_texture_color", "suzanne_texture_normal", "suzanne_texture_ao_specular_reflection"}, 1);
	
	_dragon.init("dragon", {"dragon_texture_color", "dragon_texture_normal", "dragon_texture_ao_specular_reflection" },  1);
	
	_plane.init("plane", { "plane_texture_color", "plane_texture_normal", "plane_texture_depthmap" },  2);
	
	_skybox.init();
	
	std::map<std::string, GLuint> ambientTextures = _gbuffer->textureIds({ TextureType::Albedo, TextureType::Normal, TextureType::Depth });
	ambientTextures["ssaoTexture"] = _ssaoBlurFramebuffer->textureId();
	_ambientScreen.init(ambientTextures);
	
	const std::vector<TextureType> includedTextures = { TextureType::Albedo, TextureType::Depth, TextureType::Normal, TextureType::Effects };
	for(auto& dirLight : _directionalLights){
		dirLight.init(_gbuffer->textureIds(includedTextures));
	}
	
	for(auto& pointLight : _pointLights){
		pointLight.init(_gbuffer->textureIds(includedTextures));
	}
	
	_ssaoBlurScreen.init(_ssaoFramebuffer->textureId(), "boxblur_float");
	_toneMappingScreen.init(_sceneFramebuffer->textureId(), "tonemap");
	_fxaaScreen.init(_toneMappingFramebuffer->textureId(), "fxaa");
	_finalScreen.init(_fxaaFramebuffer->textureId(), "final_screenquad");
	checkGLError();
	
	
	// Position fixed objects.
	const glm::mat4 dragonModel = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-0.1,-0.05,-0.25)),glm::vec3(0.5f));
	const glm::mat4 planeModel = glm::scale(glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-0.35f,-0.5f)), glm::vec3(2.0f));
	
	_dragon.update(dragonModel);
	_plane.update(planeModel);
	
}


void Renderer::draw() {
	
	// Compute the time elapsed since last frame
	double elapsed = glfwGetTime() - _timer;
	_timer = glfwGetTime();
	
	// Physics simulation
	physics(elapsed);
	
	
	glm::vec2 invRenderSize = 1.0f / _camera.renderSize();
	
	// --- Light pass -------
	
	// Draw the scene inside the framebuffer.
	for(auto& dirLight : _directionalLights){
		
		dirLight.bind();
		
		// Draw objects.
		_suzanne.drawDepth(dirLight.mvp());
		_dragon.drawDepth(dirLight.mvp());
		//_plane.drawDepth(planeModel, _light._mvp);
		
		dirLight.blurAndUnbind();
	
	}
	
	// ----------------------
	
	// --- Scene pass -------
	// Bind the full scene framebuffer.
	_gbuffer->bind();
	// Set screen viewport
	glViewport(0,0,_gbuffer->width(),_gbuffer->height());
	
	// Clear the depth buffer (we know we will draw everywhere, no need to clear color.
	glClear(GL_DEPTH_BUFFER_BIT);
	
	// Draw objects
	_suzanne.draw(_camera.view(), _camera.projection());
	_dragon.draw(_camera.view(), _camera.projection());
	_plane.draw(_camera.view(), _camera.projection());
	
	for(auto& pointLight : _pointLights){
		pointLight.drawDebug(_camera.view(), _camera.projection());
	}
	
	_skybox.draw(_camera.view(), _camera.projection());
	
	// Unbind the full scene framebuffer.
	_gbuffer->unbind();
	// ----------------------
	
	glDisable(GL_DEPTH_TEST);
	
	// --- SSAO pass
	_ssaoFramebuffer->bind();
	glViewport(0,0,_ssaoFramebuffer->width(), _ssaoFramebuffer->height());
	_ambientScreen.drawSSAO( 2.0f * invRenderSize, _camera.view(), _camera.projection());
	_ssaoFramebuffer->unbind();
	
	// --- SSAO blurring pass
	_ssaoBlurFramebuffer->bind();
	glViewport(0,0,_ssaoBlurFramebuffer->width(), _ssaoBlurFramebuffer->height());
	_ssaoBlurScreen.draw( invRenderSize );
	_ssaoBlurFramebuffer->unbind();
	
	// --- Gbuffer composition pass
	_sceneFramebuffer->bind();
	
	glViewport(0,0,_sceneFramebuffer->width(), _sceneFramebuffer->height());
	
	_ambientScreen.draw( invRenderSize, _camera.view(), _camera.projection());
	
	glEnable(GL_BLEND);
	for(auto& dirLight : _directionalLights){
		dirLight.draw( invRenderSize, _camera.view(), _camera.projection());
	}
	glCullFace(GL_FRONT);
	for(auto& pointLight : _pointLights){
		pointLight.draw( invRenderSize, _camera.view(), _camera.projection());
	}
	glDisable(GL_BLEND);
	glCullFace(GL_BACK);
	_sceneFramebuffer->unbind();
	
	_toneMappingFramebuffer->bind();
	glViewport(0,0,_toneMappingFramebuffer->width(), _toneMappingFramebuffer->height());
	_toneMappingScreen.draw( invRenderSize );
	_toneMappingFramebuffer->unbind();
	
	// --- FXAA pass -------
	// Bind the post-processing framebuffer.
	_fxaaFramebuffer->bind();
	// Set screen viewport.
	glViewport(0,0,_fxaaFramebuffer->width(), _fxaaFramebuffer->height());
	
	// Draw the fullscreen quad
	_fxaaScreen.draw( invRenderSize );
	
	_fxaaFramebuffer->unbind();
	// ----------------------
	
	
	// --- Final pass -------
	// We now render a full screen quad in the default framebuffer, using sRGB space.
	glEnable(GL_FRAMEBUFFER_SRGB);
	
	// Set screen viewport.
	glViewport(0, 0, GLsizei(_camera.screenSize()[0]), GLsizei(_camera.screenSize()[1]));
	
	// Draw the fullscreen quad
	_finalScreen.draw( 1.0f / _camera.screenSize());
	
	glDisable(GL_FRAMEBUFFER_SRGB);
	// ----------------------
	glEnable(GL_DEPTH_TEST);
	
	// Update timer
	_timer = glfwGetTime();
}

void Renderer::physics(double elapsedTime){
	
	_camera.update(elapsedTime);
	
	// Update lights.
	_directionalLights[0].update(glm::vec3(2.0f, 1.5f, 2.0f), _camera.view());
	
	for(size_t i = 0; i <_pointLights.size(); ++i){
		auto& pointLight = _pointLights[i];
		glm::vec4 newPosition = glm::rotate(glm::mat4(1.0f), (float)elapsedTime, glm::vec3(0.0f, 1.0f, 0.0f))*glm::vec4(pointLight.local(), 1.0f);
		pointLight.update(glm::vec3(newPosition), _camera.view());
	}
	
	// Update objects.
	
	const glm::mat4 suzanneModel = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.2,0.0,0.0)),float(_timer),glm::vec3(0.0f,1.0f,0.0f)),glm::vec3(0.25f));
	_suzanne.update(suzanneModel);
	
}


void Renderer::clean() const {
	// Clean objects.
	_suzanne.clean();
	_dragon.clean();
	_plane.clean();
	_skybox.clean();
	for(auto& dirLight : _directionalLights){
		dirLight.clean();
	}
	_ambientScreen.clean();
	_fxaaScreen.clean();
	_ssaoBlurScreen.clean();
	_toneMappingScreen.clean();
	_finalScreen.clean();
	_gbuffer->clean();
	_ssaoFramebuffer->clean();
	_ssaoBlurFramebuffer->clean();
	_sceneFramebuffer->clean();
	_toneMappingFramebuffer->clean();
	_fxaaFramebuffer->clean();
}


void Renderer::resize(int width, int height){
	//Update the size of the viewport.
	glViewport(0, 0, width, height);
	// Update the projection matrix.
	_camera.screen(width, height);
	// Resize the framebuffer.
	_gbuffer->resize(_camera.renderSize());
	_ssaoFramebuffer->resize(0.5f * _camera.renderSize());
	_ssaoBlurFramebuffer->resize(_camera.renderSize());
	_sceneFramebuffer->resize(_camera.renderSize());
	_toneMappingFramebuffer->resize(_camera.renderSize());
	_fxaaFramebuffer->resize(_camera.renderSize());
}

void Renderer::keyPressed(int key, int action){
	if(action == GLFW_PRESS){
		_camera.key(key, true);
	} else if(action == GLFW_RELEASE) {
		_camera.key(key, false);
	}
}

void Renderer::buttonPressed(int button, int action, double x, double y){
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			_camera.mouse(MouseMode::Start,(float)x, (float)y);
		} else if (action == GLFW_RELEASE) {
			_camera.mouse(MouseMode::End, 0.0, 0.0);
		}
	} else {
		std::cout << "Button: " << button << ", action: " << action << std::endl;
	}
}

void Renderer::mousePosition(double x, double y, bool leftPress, bool rightPress){
	if (leftPress){
		_camera.mouse(MouseMode::Move, float(x), float(y));
	}
}



