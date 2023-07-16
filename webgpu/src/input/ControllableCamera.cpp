#include "ControllableCamera.hpp"
#include "Input.hpp"
#include <stdio.h>
#include <algorithm>


ControllableCamera::ControllableCamera() : Camera() {
	//_verticalResolution = 720;
	_speed = 1.2f;
	_angularSpeed = 4.0f;
	_mode = TurnTable;
	reset();
}

ControllableCamera::~ControllableCamera(){}

void ControllableCamera::reset(){
	_eye = glm::vec3(0.0,0.0,1.0);
	_center = glm::vec3(0.0,0.0,0.0);
	_up = glm::vec3(0.0,1.0,0.0);
	_right = glm::vec3(1.0,0.0,0.0);
	_view = glm::lookAt(_eye, _center, _up);
	_verticalAngle = 0.0f;
	_horizontalAngle = (float)M_PI*0.5f;
	_radius = 1.0;
}

void ControllableCamera::update(){
	if(Input::manager().triggered(Input::KeyR)){
		reset();
	}
	if(Input::manager().triggered(Input::KeyF)){
		_mode = FPS;
	}
	if(Input::manager().triggered(Input::KeyG)){
		_mode = TurnTable;
		_radius = glm::length(_eye - _center);
	}
}

void ControllableCamera::physics(double frameTime){
	
	if (_mode == FPS) {
		updateUsingKeyboard(frameTime);
	} else if (_mode == TurnTable){
		updateUsingTurnTable(frameTime);
	}
	
	updateView();
}


void ControllableCamera::updateUsingKeyboard(double frameTime){
	// We need the direction of the camera, normalized.
	glm::vec3 look = normalize(_center - _eye);
	// One step forward or backward.
	const glm::vec3 deltaLook =  _speed * (float)frameTime * look;
	// One step laterally horizontal.
	const glm::vec3 deltaLateral = _speed * (float)frameTime * _right;
	// One step laterally vertical.
	const glm::vec3 deltaVertical = _speed * (float)frameTime * _up;
	
	
	if(Input::manager().pressed(Input::KeyW)){ // Forward
		_eye += deltaLook;
	}
	
	if(Input::manager().pressed(Input::KeyS)){ // Backward
		_eye -= deltaLook;
	}
	
	if(Input::manager().pressed(Input::KeyA)){ // Left
		_eye -= deltaLateral;
	}
	
	if(Input::manager().pressed(Input::KeyD)){ // Right
		_eye += deltaLateral;
	}
	
	if(Input::manager().pressed(Input::KeyQ)){ // Down
		_eye -= deltaVertical;
	}
	
	if(Input::manager().pressed(Input::KeyE)){ // Up
		_eye += deltaVertical;
	}
	
	// Update center (eye-center stays constant).
	_center = _eye + look;
	
	// Recompute right as the cross product of look and up.
	_right = normalize(cross(look,_up));
	// Recompute up as the cross product of  right and look.
	_up = normalize(cross(_right,look));
	
}

void ControllableCamera::updateUsingTurnTable(double frameTime){
	// We need the direction of the camera, normalized.
	const glm::vec3 look = normalize(_center - _eye);
	// One step forward or backward.
	const glm::vec3 deltaLook =  _speed * (float)frameTime * look;
	// One step laterally horizontal.
	const glm::vec3 deltaLateral = _speed * (float)frameTime * _right;
	// One step laterally vertical.
	const glm::vec3 deltaVertical = _speed * (float)frameTime * _up;
	
	
	if(Input::manager().pressed(Input::KeyW)){ // Forward
		_center += deltaLook;
	}
	
	if(Input::manager().pressed(Input::KeyS)){ // Backward
		_center -= deltaLook;
	}
	
	if(Input::manager().pressed(Input::KeyA)){ // Left
		_center -= deltaLateral;
	}
	
	if(Input::manager().pressed(Input::KeyD)){ // Right
		_center += deltaLateral;
	}
	
	if(Input::manager().pressed(Input::KeyQ)){ // Down
		_center -= deltaVertical;
	}
	
	if(Input::manager().pressed(Input::KeyE)){ // Up
		_center += deltaVertical;
	}
	
	// Radius of the turntable.
	float scroll = Input::manager().scroll()[1];
	_radius = (std::max)(0.0001f, _radius - scroll * (float)frameTime*_speed);
	
	// Angles update for the turntable.
	const glm::vec2 delta = Input::manager().moved(Input::MouseLeft);
	_horizontalAngle += delta[0] * (float)frameTime*_angularSpeed;
	_verticalAngle = (std::max)(-1.57f, (std::min)(1.57f, _verticalAngle + delta[1] * (float)frameTime*_angularSpeed));
	
	// Compute new look direction.
	const glm::vec3 newLook = - glm::vec3( cos(_verticalAngle) * cos(_horizontalAngle), sin(_verticalAngle),  cos(_verticalAngle) * sin(_horizontalAngle));
	
	// Update the camera position around the center.
	_eye =  _center - _radius * newLook;
	
	// Recompute right as the cross product of look and up.
	_right = normalize(cross(newLook,glm::vec3(0.0f,1.0f,0.0f)));
	// Recompute up as the cross product of  right and look.
	_up = normalize(cross(_right,newLook));
	
}
