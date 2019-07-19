#include <GLFW/glfw3.h>
#include <stdio.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#include "Keyboard.h"

Keyboard::Keyboard(glm::vec3 & eye, glm::vec3 & center, glm::vec3 & up, glm::vec3 & right) : _eye(eye), _center(center), _up(up), _right(right) {
	_speed = 1.2f;
	_angularSpeed = 75.0f;
	reset();
} 

Keyboard::~Keyboard(){}

void Keyboard::reset(){
	_keys[0] = _keys[1] = _keys[2] = _keys[3] = _keys[4] = _keys[5] = _keys[6] = false;
	_previousPosition = glm::vec2(0.0);
	_deltaPosition = glm::vec2(0.0);
}

void Keyboard::update(double elapsedTime){
	
	// We need the direction of the camera, normalized.
	glm::vec3 look = normalize(_center - _eye);
	// One step forward or backward.
	glm::vec3 deltaLook =  _speed * (float)elapsedTime * look;
	// One step laterally horizontal.
	glm::vec3 deltaLateral = _speed * (float)elapsedTime * _right;
	// One step laterally vertical.
	glm::vec3 deltaVertical = _speed * (float)elapsedTime * _up;

	
	if(_keys[0]){ // Forward
		_eye = _eye + deltaLook;
	}

	if(_keys[1]){ // Backward
  		_eye = _eye - deltaLook;
	}

	if(_keys[2]){ // Left
  		_eye = _eye - deltaLateral;
	}

	if(_keys[3]){ // Right
  		_eye = _eye + deltaLateral;
	}

	if(_keys[4]){ // Down
  		_eye = _eye - deltaVertical;
	}

	if(_keys[5]){ // Up
  		_eye = _eye + deltaVertical;
	}
	if(_keys[6]){
  		_center = _center + (_deltaPosition.x * _right + _deltaPosition.y * _up) * (float)elapsedTime * _angularSpeed;
  		look = normalize(_center - _eye);
	}

	// Update center (eye-center stays constant).
	_center = _eye + look;
  	
  	// Recompute right as the cross product of look and up.
	_right = normalize(cross(look,_up));
	// Recompute up as the cross product of  right and look.
	_up = normalize(cross(_right,look));

}

void Keyboard::registerMove(int direction, bool flag){
	// Depending on the direction, we update the corresponding flag. 
	// This will allow for smooth movements.
	switch(direction){
		case GLFW_KEY_W:
			_keys[0] = flag;
			break;
		case GLFW_KEY_S:
			_keys[1] = flag;
			break;
		case GLFW_KEY_A:
			_keys[2] = flag;
			break;
		case GLFW_KEY_D:
			_keys[3] = flag;
			break;
		case GLFW_KEY_Q:
			_keys[4] = flag;
			break;
		case GLFW_KEY_E:
			_keys[5] = flag;
			break;
		default:
			break;
	}
}

void Keyboard::startLeftMouse(double x, double y){
	_previousPosition = glm::vec2(x,-y);
}

void Keyboard::leftMouseTo(double x, double y){
	_keys[6] = true;
	_deltaPosition = glm::vec2(x, -y) - _previousPosition;
	_previousPosition =  glm::vec2(x, -y) ;
}

void Keyboard::endLeftMouse(){
	_keys[6] = false;
}


