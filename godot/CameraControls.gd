extends Camera3D

var _eye = Vector3(0.0,2.0,5.0)
var _center = Vector3(0.0,1.0,0.0)
var _up = Vector3(0.0,1.0,0.0)
var _right = Vector3(1.0,0.0,0.0)
var _verticalAngle = 0.0
var _horizontalAngle = PI*0.5
var _radius = 5.0
const _speed = 3.0
const _angularSpeed = 3.5

func updateCam():
	look_at_from_position(_eye, _center, _up)

# Called when the node enters the scene tree for the first time.
func _ready():
	Input.mouse_mode = Input.MOUSE_MODE_HIDDEN
	updateCam()

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	# We need the direction of the camera, normalized.
	var look = (_center - _eye).normalized();
	# One step forward or backward.
	var deltaLook =  _speed * delta * look;
	# One step laterally horizontal.
	var deltaLateral = _speed * delta * _right
	# One step laterally vertical.
	var deltaVertical = _speed * delta * _up
	
	if Input.is_action_pressed("move_forward"):
		_center += deltaLook
	
	if Input.is_action_pressed("move_back"):
		_center -= deltaLook
	
	if Input.is_action_pressed("move_left"):
		_center -= deltaLateral
	
	if Input.is_action_pressed("move_right"):
		_center += deltaLateral
		
	if Input.is_action_pressed("move_down"):
		_center -= deltaVertical
		
	if Input.is_action_pressed("move_up"):
		_center += deltaVertical
		
	# Radius of the turntable.
	var scroll = 0.0
	if Input.is_action_pressed("zoom_in"):
		scroll += 1.0
	if Input.is_action_pressed("zoom_out"):
		scroll -= 1.0
		
		
	_radius = max(0.001, _radius - scroll * delta *_speed)
	
	# Angles update for the turntable.
	var deltaMouse = Input.get_last_mouse_velocity() * 0.001
	if Input.is_action_pressed("rotate_left"):
		deltaMouse.x -= 0.01
	if Input.is_action_pressed("rotate_right"):
		deltaMouse.x += 0.01
	if Input.is_action_pressed("rotate_down"):
		deltaMouse.y -= 0.01
	if Input.is_action_pressed("rotate_up"):
		deltaMouse.y += 0.01
	
	_horizontalAngle += deltaMouse[0] * delta * _angularSpeed
	_verticalAngle = clamp(_verticalAngle + deltaMouse[1] * delta * _angularSpeed, -1.57, 1.57)
	
	# Compute new look direction.
	var newLook = - Vector3( cos(_verticalAngle) * cos(_horizontalAngle), sin(_verticalAngle),  cos(_verticalAngle) * sin(_horizontalAngle))
	# Update the camera position around the center.
	_eye =  _center - _radius * newLook
	
	# Recompute right as the cross product of look and up.
	_right = newLook.cross(Vector3(0.0,1.0,0.0)).normalized()
	# Recompute up as the cross product of  right and look.
	_up = _right.cross(newLook).normalized()
	updateCam()
