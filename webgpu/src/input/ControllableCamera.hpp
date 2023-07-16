#ifndef ControllableCamera_h
#define ControllableCamera_h

#include "Camera.hpp"
#include "../common.hpp"

class ControllableCamera : public Camera {
	
public:
	
	ControllableCamera();

	~ControllableCamera();
	
	/// Reset the position of the camera.
	void reset();

	/// Update one-shot parameters.
	void update();
	
	/// Update the view matrix.
	void physics(double frameTime);

	
private:
	
	void updateUsingKeyboard(double frameTime);
	
	void updateUsingTurnTable(double frameTime);
	
	float _speed;
	float _angularSpeed;
	
	// TurnTable parameters.
	float _verticalAngle;
	float _horizontalAngle;
	float _radius;
	
	enum CameraMode {
		FPS, TurnTable, Trackball
	};
	
	CameraMode _mode;
};

#endif
