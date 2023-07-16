#ifndef Camera_h
#define Camera_h

#include "../common.hpp"

class Camera {
	
public:
	
	Camera();

	~Camera();
	
	/// Update the view matrix.
	void physics(double frameTime);
	
	/// Update all projection parameters.
	void projection(float ratio, float fov, float near, float far);
	
	/// Update the frustum near and far planes.
	void frustum(float near, float far);
	
	/// Update the aspect ratio.
	void ratio(float ratio);
	
	/// Update the FOV (in radians).
	void fov(float fov);
	
	float fov() const { return _fov; }
	
	const glm::mat4 view() const { return _view; }
	const glm::mat4 projection() const { return _projection; }
	
protected:
	
	/// Update the projection matrice parameters.
	void updateProjection();
	void updateView();
	
	/// The view matrix.
	glm::mat4 _view;
	/// The projection matrix.
	glm::mat4 _projection;
	
	/// Vectors defining the view frame.
	glm::vec3 _eye;
	glm::vec3 _center;
	glm::vec3 _up;
	glm::vec3 _right;
	
	float _fov;
	float _ratio;
	float _near;
	float _far;
	
};

#endif
