//
//  TextureUtilities.swift
//  DragonMetal
//
//  Created by Simon Rodriguez on 15/06/2017.
//  Copyright © 2017 Simon Rodriguez. All rights reserved.
//

import Foundation
import MetalKit


// Simplify texture loading syntax.

extension MTKTextureLoader {
	
	func newTexture(withContentsOf url: URL, srgb: Bool)-> MTLTexture {
		return try! self.newTexture(URL: url,
		                            options: [
										MTKTextureLoader.Option.generateMipmaps: true,
										MTKTextureLoader.Option.allocateMipmaps: true,
										MTKTextureLoader.Option.origin: MTKTextureLoader.Origin.flippedVertically,
										MTKTextureLoader.Option.SRGB : srgb
									])
	}
	
	func newTextureCubemap(withContentsOf url: URL, srgb: Bool)-> MTLTexture {
		return try! self.newTexture(URL: url,
		                            options: [
										MTKTextureLoader.Option.generateMipmaps: true,
										MTKTextureLoader.Option.allocateMipmaps: true,
										MTKTextureLoader.Option.origin : MTKTextureLoader.Origin.flippedVertically,
										MTKTextureLoader.Option.SRGB : srgb,
										MTKTextureLoader.Option.cubeLayout :  MTKTextureLoader.CubeLayout.vertical
									])
	}
	
}
