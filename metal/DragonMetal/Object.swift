//
//  Object.swift
//  DragonMetal
//
//  Created by Simon Rodriguez on 28/10/2016.
//  Copyright © 2016 Simon Rodriguez. All rights reserved.
//

import Foundation
import MetalKit
import ModelIO


class Object {
	
	let name: String!
	
	// Parameters.
	var model = matrix_identity_float4x4
	private var parameters = ObjectConstants()
	private var material = MaterialConstants()
	
	// Buffers.
	private var vertexBuffer: MTLBuffer
	private var indexBuffer: MTLBuffer
	private var indexCount: Int
	
	
	init(name: String, device: MTLDevice, shininess: Int = 0) {
		self.name = name
		
		// Load the mesh from disk.
		let url = Bundle.main.url(forResource: name, withExtension: "obj")
		let mesh = Mesh(url: url!)!
		
		// Process the mesh.
		mesh.centerAndUnit()
		mesh.computeTangentFrame()
		
		// Send mesh data to the GPU.
		mesh.setupBuffers(device: device)
		vertexBuffer = mesh.vertexBuffer!
		indexBuffer = mesh.indexBuffer!
		indexCount = mesh.indexCount
		
		// Material setup and textures.
		material.shininess = shininess
		loadTextures(device: device)
	}
	
	func update(camera: Camera, vpLight: matrix_float4x4){
		// Compute all transformations for this frame.
		parameters.mv = matrix_multiply(camera.viewMatrix, self.model)
		parameters.invmv = parameters.mv.inverse.transpose
		parameters.mvp = matrix_multiply(camera.projectionMatrix, parameters.mv)
		parameters.mvpLight = matrix_multiply(vpLight, self.model)
	}
	
	func encode(renderEncoder: MTLRenderCommandEncoder, constants: GlobalConstants){
		renderEncoder.pushDebugGroup("Draw " + name)
		
		// Set buffers.
		renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
		// Uniforms for the vertex shader.
		renderEncoder.setVertexBytes(&parameters, length: MemoryLayout<ObjectConstants>.stride, index: 1)
		// Uniforms for the fragment shader.
		var localConstants = constants
		renderEncoder.setFragmentBytes(&localConstants, length: MemoryLayout<GlobalConstants>.stride, index: 0)
		renderEncoder.setFragmentBytes(&material, length: MemoryLayout<MaterialConstants>.stride, index: 1)
		// Textures.
		setFragmentTextures(encoder: renderEncoder)
		// Draw.
		renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: indexCount, indexType: .uint32, indexBuffer: indexBuffer, indexBufferOffset: 0)
		
		renderEncoder.popDebugGroup()

	}
	
	func encodeShadow(renderEncoder: MTLRenderCommandEncoder){
		renderEncoder.pushDebugGroup("Shadow " + name)
	
		// Set buffers.
		renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
		// Uniforms for the fragment shader.
		renderEncoder.setVertexBytes(&parameters, length: MemoryLayout<ObjectConstants>.stride, index: 1)
		// Draw.
		renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: indexCount, indexType: .uint32, indexBuffer: indexBuffer, indexBufferOffset: 0)
		
		renderEncoder.popDebugGroup()

	}
	
	func loadTextures(device: MTLDevice){
		// Has to be overriden by subclasses.
		fatalError("Use a subclass implementing texture management.")
	}
	
	func setFragmentTextures(encoder: MTLRenderCommandEncoder){
		// Has to be overriden by subclasses.
		fatalError("Use a subclass implementing texture management.")
	}
}



