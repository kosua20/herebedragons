//
//  Renderer.swift
//  DragonMetal
//
//  Created by Simon Rodriguez on 11/06/2017.
//  Copyright © 2017 Simon Rodriguez. All rights reserved.
//

import Foundation
import MetalKit


class Renderer {

	private var constants = GlobalConstants()
	private var time : TimeInterval = 0
	public var camera: Camera!
	
	// Objects
	private var dragon: Object! = nil
	private var monkey: Object! = nil
	private var plane: Object! = nil
	private var skybox: Object! = nil
	
	// Light parameters.
	private var lightMatrix = matrix_float4x4()
	private let worldLightDir = normalize(float4(1.0,1.0,1.0,0.0))
	
	// Final pass states.
	private var objectPipeline: MTLRenderPipelineState! = nil
	private var skyboxPipeline: MTLRenderPipelineState! = nil
	private var depthStencilState : MTLDepthStencilState! = nil
	// Shadow states.
	private var shadowPipeline: MTLRenderPipelineState! = nil
	private var shadowRenderDescriptor: MTLRenderPassDescriptor! = nil
	private var shadowDepthState : MTLDepthStencilState! = nil
	
	
	init(device: MTLDevice, parameters: ViewParameters){
		
		// Create depth descriptors for both passes.
		let depthStencilDescriptor = MTLDepthStencilDescriptor()
		depthStencilDescriptor.depthCompareFunction = .less
		depthStencilDescriptor.isDepthWriteEnabled = true
		depthStencilState = device.makeDepthStencilState(descriptor: depthStencilDescriptor)
		// Reuse the descriptor for the shadow pass depth state.
		depthStencilDescriptor.depthCompareFunction = .lessEqual
		shadowDepthState = device.makeDepthStencilState(descriptor: depthStencilDescriptor)
		
		// Load shaders from the library.
		let defaultLibrary = device.makeDefaultLibrary()!
		let objectVertexProgram = defaultLibrary.makeFunction(name: "objectVertex")!
		let objectFragmentProgram = defaultLibrary.makeFunction(name: "objectFragment")!
		let skyboxVertexProgram = defaultLibrary.makeFunction(name: "skyboxVertex")!
		let skyboxFragmentProgram = defaultLibrary.makeFunction(name: "skyboxFragment")!
		let shadowVertexProgram = defaultLibrary.makeFunction(name: "shadowVertex")!
		
		// Create a pipeline descriptor for the main pass objects.
		let objectDescriptor = MTLRenderPipelineDescriptor()
		objectDescriptor.sampleCount = parameters.sampleCount
		objectDescriptor.vertexFunction = objectVertexProgram
		objectDescriptor.fragmentFunction = objectFragmentProgram
		objectDescriptor.colorAttachments[0].pixelFormat = parameters.colorPixelFormat
		objectDescriptor.depthAttachmentPixelFormat = parameters.depthStencilPixelFormat
		
		// Create a pipeline descriptor for the skybox.
		let skyboxDescriptor = MTLRenderPipelineDescriptor()
		skyboxDescriptor.sampleCount = parameters.sampleCount
		skyboxDescriptor.vertexFunction = skyboxVertexProgram
		skyboxDescriptor.fragmentFunction = skyboxFragmentProgram
		skyboxDescriptor.colorAttachments[0].pixelFormat = parameters.colorPixelFormat
		skyboxDescriptor.depthAttachmentPixelFormat = parameters.depthStencilPixelFormat
		
		// Create texture for the shadow map
		let shadowTextureDescriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .depth32Float, width: 1024, height: 1024, mipmapped: false)
		// Store it on the GPU only.
		shadowTextureDescriptor.storageMode = .private
		// Usage: as a render target destination and as an input texture in shaders.
		shadowTextureDescriptor.usage = MTLTextureUsage.renderTarget.union(MTLTextureUsage.shaderRead)
		guard let shadowTexture = device.makeTexture(descriptor: shadowTextureDescriptor) else {
			print("Couldn't make shadow texture.")
			return
		}
		
		// Create the shadow render pass descriptor.
		shadowRenderDescriptor = MTLRenderPassDescriptor()
		if let shadowAttachment = shadowRenderDescriptor!.depthAttachment {
			shadowAttachment.texture = shadowTexture
			shadowAttachment.loadAction = .clear
			shadowAttachment.storeAction = .store
			shadowAttachment.clearDepth = 1.0
		}
		
		// Create a pipeline descriptor for the shadow pass
		let shadowDescriptor = MTLRenderPipelineDescriptor()
		shadowDescriptor.depthAttachmentPixelFormat = shadowTexture.pixelFormat
		shadowDescriptor.vertexFunction = shadowVertexProgram
		// No color attachment, no fragment shader.
		shadowDescriptor.fragmentFunction = nil
		shadowDescriptor.colorAttachments[0].pixelFormat = .invalid
	
		// Obtain the pipeline states from the descriptors and store them.
		do {
			try objectPipeline = device.makeRenderPipelineState(descriptor: objectDescriptor)
			try skyboxPipeline = device.makeRenderPipelineState(descriptor: skyboxDescriptor)
			try shadowPipeline = device.makeRenderPipelineState(descriptor: shadowDescriptor)
		} catch let error {
			print("Failed to create pipeline state, error \(error)")
		}
		
		// Create camera and light.
		camera = Camera(width: parameters.width, height: parameters.height)
		let lightProj = orthographyMatrix(left: -0.5, right: 0.5, bottom: -0.5, top: 0.5, near: 0.5, far: 2.0)
		let lightView = lookAtMatrix(eye: float3(2.0,2.0,2.0), target: float3(0.0,0.0,0.0), up: float3(0.0,1.0,0.0))
		lightMatrix = matrix_multiply(lightProj, lightView)
		
		// Load objects.
		dragon = SceneObject(name: "dragon", device: device, shininess: 64, shadowMap: shadowTexture)
		monkey = SceneObject(name: "suzanne", device: device, shininess: 8, shadowMap: shadowTexture)
		plane = SceneObject(name: "plane", device: device, shininess: 32, shadowMap: shadowTexture)
		skybox = SkyboxObject(name: "cubemap", device: device)
		// Place objects.
		dragon.model = matrix_model(scale: 1.2, t: float3(-0.5,0.0,-0.5))
		monkey.model = matrix_model(scale: 0.65, t: float3(0.5, 0.0, 0.5))
		plane.model = matrix_model(scale: 2.75, t: float3(0.0,-0.8,0.0))
		skybox.model = matrix_model(scale: 15.0, t: float3(0.0,0.0,0.0))
		
	}
	
	
	func update(timeStep : TimeInterval){
		time += timeStep
		// Update camera position/orientation.
		camera.update(step: timeStep)
		// Update all objects accordingly.
		monkey.model = matrix_model(scale: 0.65, t: float3(0.5, 0.0, 0.5), angle: Float(time), axis: float3(0.0,1.0,0.0))
		dragon.update(camera: camera, vpLight: lightMatrix)
		monkey.update(camera: camera, vpLight: lightMatrix)
		plane.update(camera: camera, vpLight: lightMatrix)
		skybox.update(camera: camera, vpLight: lightMatrix)
		// Update light view-space direction.
		constants.lightDir = normalize(matrix_multiply(camera.viewMatrix, worldLightDir))
	}
	
	
	func encode(commandBuffer: MTLCommandBuffer, finalPass renderPassDescriptor: MTLRenderPassDescriptor){
		
		// Shadow pass.
		// Create a render command encoder.
		guard let renderEncoderShadow = commandBuffer.makeRenderCommandEncoder(descriptor: shadowRenderDescriptor) else {
			print("Error encoding shadow pass.")
			return
		}
		renderEncoderShadow.label = "Shadow pass"
		// Set states.
		renderEncoderShadow.setFrontFacing(.counterClockwise)
		renderEncoderShadow.setCullMode(.back)
		renderEncoderShadow.setDepthStencilState(shadowDepthState)
		renderEncoderShadow.setRenderPipelineState(shadowPipeline)
		// Render.
		dragon.encodeShadow(renderEncoder: renderEncoderShadow)
		monkey.encodeShadow(renderEncoder: renderEncoderShadow)
		// Pass finished.
		renderEncoderShadow.endEncoding()
		
		// Final pass.
		// New encoder.
		guard let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor) else {
			print("Error encoding main pass.")
			return
		}
		renderEncoder.label = "Main pass"
		// Set states.
		renderEncoder.setFrontFacing(.counterClockwise)
		renderEncoder.setCullMode(.back)
		renderEncoder.setDepthStencilState(depthStencilState)
		renderEncoder.setRenderPipelineState(objectPipeline)
		// Render objects.
		dragon.encode(renderEncoder: renderEncoder, constants: constants)
		monkey.encode(renderEncoder: renderEncoder, constants: constants)
		plane.encode(renderEncoder: renderEncoder, constants: constants)
		// Render skybox.
		renderEncoder.setCullMode(.front)
		renderEncoder.setRenderPipelineState(skyboxPipeline)
		skybox.encode(renderEncoder: renderEncoder, constants: constants)
		// Pass finished.
		renderEncoder.endEncoding()

	}
	
}
