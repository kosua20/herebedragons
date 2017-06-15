//
//  Camera.swift
//  DragonMetal
//
//  Created by Simon Rodriguez on 28/10/2016.
//  Copyright © 2016 Simon Rodriguez. All rights reserved.
//

import Foundation
import simd


class Camera {
	
	public var viewMatrix : matrix_float4x4
	public var projectionMatrix : matrix_float4x4
	public var isMoving = false
	private var clickPoint = NSPoint.zero
	
	private var eye : float3
	private var center : float3 = float3(0.0,0.0,0.0)
	private var up : float3 = float3(0.0,1.0,0.0)
	
	private var horizontalAngle : Float = 0.0
	private var verticalAngle : Float = 0.0
	private var radius : Float = 3.0
	private let speed : Float = 0.01
	
	
	init(width: CGFloat, height: CGFloat ){
		// Setup view matrix.
		eye = radius*float3(cos(verticalAngle)*cos(horizontalAngle), sin(verticalAngle), cos(verticalAngle)*sin(horizontalAngle))
		viewMatrix = lookAtMatrix(eye: eye, target: center, up: up)
		// Create projection matrix.
		projectionMatrix = perspectiveMatrix(fov: 1.3, aspect: Float(width)/Float(height), near: 0.01, far: 100.0)
	}
	
	
	func update(step : TimeInterval) {
		// Update the camera postion, and the view matrix.
		eye = radius*float3(cos(verticalAngle)*cos(horizontalAngle), sin(verticalAngle), -cos(verticalAngle)*sin(horizontalAngle))
		viewMatrix = lookAtMatrix(eye: eye, target: center, up: up)
	}
	
	
	// Handle resize.
	func resize(width newWidth: CGFloat, height newHeight: CGFloat){
		// Update projection matrix.
		projectionMatrix = perspectiveMatrix(fov: 1.3, aspect: Float(newWidth)/Float(newHeight), near: 0.01, far: 100.0)
	}
	
	// Handle mouse interactions.
	
	func startMove(point: NSPoint){
		isMoving = true
		clickPoint = point
	}
	
	func move(point: NSPoint){
		let dx = Float(point.x - clickPoint.x)
		let dy = Float(point.y - clickPoint.y)
		horizontalAngle -= dx * speed
		verticalAngle -= dy * speed
		verticalAngle = min(max(verticalAngle, -1.57),1.57)
		clickPoint = point
	}
	
	func endMove(){
		isMoving = false
	}
	
	func scroll(amount: CGFloat){
		radius += speed*Float(amount)
		radius = min(max(0.01, radius), 8.0)
	}
	
}
