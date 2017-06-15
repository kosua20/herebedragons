//
//  GameViewController.swift
//  DragonMetal
//
//  Created by Simon Rodriguez on 27/10/2016.
//  Copyright (c) 2016 Simon Rodriguez. All rights reserved.
//

import Cocoa
import MetalKit
import simd

let kMaxBuffers = 3

struct ViewParameters {
	let width: CGFloat
	let height: CGFloat
	let sampleCount: Int
	let colorPixelFormat: MTLPixelFormat
	let depthStencilPixelFormat: MTLPixelFormat
}

class GameViewController: NSViewController, MTKViewDelegate {
	
	// The GPU device.
    private var device: MTLDevice! = nil
	// Global app command queue.
    private var commandQueue: MTLCommandQueue! = nil
	// We send commands for 3 frames ahead, using a semaphore for sync.
    private let semaphore = DispatchSemaphore(value: kMaxBuffers)
    private var bufferIndex = 0
	
	private var renderer: Renderer!
	
	
    override func viewDidLoad() {
        
        super.viewDidLoad()
		
        // Setup the view.
        let view = self.view as! MTKView
		// Clear to solid white.
		view.clearColor = MTLClearColorMake(1, 1, 1, 1)
		// Use a BGRA 8-bit normalized texture for the drawable.
		view.colorPixelFormat = .bgra8Unorm
		// Use a 32-bit depth buffer.
		view.depthStencilPixelFormat = .depth32Float
		
		// First, get a reference to the GPU device.
		device = MTLCreateSystemDefaultDevice()
		guard device != nil else {
			print("Metal is not supported on this device")
			self.view = NSView(frame: self.view.frame)
			return
		}
		
		// Create the command queue.
		commandQueue = device.makeCommandQueue()
		
		// Store view setup parameters.
		let viewParams = ViewParameters(width: view.frame.width, height: view.frame.height, sampleCount: view.sampleCount, colorPixelFormat: view.colorPixelFormat, depthStencilPixelFormat: view.depthStencilPixelFormat)
		// Create the renderer.
		renderer = Renderer(device: device, parameters: viewParams)
		
		// This controller will be the delegate that should handle graphics updates.
		view.delegate = self
		view.device = device
		
		// Disable color clearing thanks to the skybox (covering the full screen at any time).
		if let finalRenderDescriptor = view.currentRenderPassDescriptor {
			finalRenderDescriptor.colorAttachments[0].loadAction = .dontCare
		}
    }
	
	
    func draw(in view: MTKView) {
		
        // Use semaphore to encode 3 frames ahead.
        let _ = semaphore.wait(timeout: DispatchTime.distantFuture)
		
		// Time step (lazy).
		let step = 1.0 / TimeInterval((self.view as!MTKView).preferredFramesPerSecond)
		renderer.update(timeStep : step)
		
		// Create a command buffer.
        let commandBuffer = commandQueue.makeCommandBuffer()
        commandBuffer.label = "Frame command buffer"
        
        // Semaphore magic (signal when the command buffer has been processed by the GPU).
        commandBuffer.addCompletedHandler{ [weak self] commandBuffer in
            if let strongSelf = self {
                strongSelf.semaphore.signal()
            }
            return
        }
		
		// Get the final render pass descriptor (linked to the view) and the drawable.
        if let renderPassDescriptor = view.currentRenderPassDescriptor {
			// Register rendering commands.
			renderer.encode(commandBuffer: commandBuffer, finalPass: renderPassDescriptor)
			// End of frame.
			if let currentDrawable = view.currentDrawable {
				commandBuffer.present(currentDrawable)
			}
        }
		
		// Update commandBuffer index.
        bufferIndex = (bufferIndex + 1) % kMaxBuffers
		// Commit commands, render !
        commandBuffer.commit()
		
    }
	
	
    // Pass resize event to the camera.
	
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
		renderer.camera.resize(width: size.width, height: size.height)
    }
	
	
	// Pass mouse events to the camera.
	
	override func mouseDown(with event: NSEvent) {
		renderer.camera.startMove(point: event.locationInWindow)
	}
	
	override func mouseUp(with event: NSEvent) {
		renderer.camera.endMove()
	}
	
	override func mouseDragged(with event: NSEvent) {
		if(renderer.camera.isMoving){
			renderer.camera.move(point: event.locationInWindow)
		}
	}
	
	override func scrollWheel(with event: NSEvent) {
		renderer.camera.scroll(amount: event.scrollingDeltaY)
	}
	
}
