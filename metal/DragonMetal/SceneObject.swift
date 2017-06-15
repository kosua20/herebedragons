//
//  SceneObject.swift
//  DragonMetal
//
//  Created by Simon Rodriguez on 10/06/2017.
//  Copyright © 2017 Simon Rodriguez. All rights reserved.
//

import Foundation
import MetalKit


class SceneObject : Object {

	private var textureColor: MTLTexture!
	private var textureNormal: MTLTexture!
	private var shadowMap: MTLTexture!
	
	init(name: String, device: MTLDevice, shininess: Int, shadowMap: MTLTexture) {
		super.init(name: name, device: device, shininess: shininess)
		// Store the sahdow map.
		self.shadowMap = shadowMap
	}
	
	override func loadTextures(device: MTLDevice) {
		let textureLoader = MTKTextureLoader(device: device)
		// Two textures: color and normals.
		let urlTexColor = Bundle.main.url(forResource: name + "_texture_color", withExtension: ".png")!
		let urlTexNormal = Bundle.main.url(forResource: name + "_texture_normal", withExtension: ".png")!
		textureColor = textureLoader.newTexture(withContentsOf: urlTexColor, srgb: false)
		textureNormal = textureLoader.newTexture(withContentsOf: urlTexNormal, srgb: false)
	}
	
	override func setFragmentTextures(encoder: MTLRenderCommandEncoder) {
		encoder.setFragmentTexture(textureColor, at: 0)
		encoder.setFragmentTexture(textureNormal, at: 1)
		encoder.setFragmentTexture(shadowMap, at: 2)
	}
	
}
