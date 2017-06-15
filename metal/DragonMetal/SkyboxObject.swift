//
//  SkyboxObject.swift
//  DragonMetal
//
//  Created by Simon Rodriguez on 10/06/2017.
//  Copyright © 2017 Simon Rodriguez. All rights reserved.
//

import Foundation
import MetalKit

class SkyboxObject : Object {
	
	private var textureColor: MTLTexture!
	
	override func loadTextures(device: MTLDevice) {
		// Only one texture, the cubemap. Faces are stacked vertically.
		let urlTexColor = Bundle.main.url(forResource: name, withExtension: ".png")!
		let textureLoader = MTKTextureLoader(device: device)
		textureColor = textureLoader.newTextureCubemap(withContentsOf: urlTexColor, srgb: false)
	}
	
	override func setFragmentTextures(encoder: MTLRenderCommandEncoder) {
		encoder.setFragmentTexture(textureColor, at: 0)
	}
	
}
