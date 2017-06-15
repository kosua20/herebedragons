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
		return try! self.newTexture(withContentsOf: url, options: [MTKTextureLoaderOptionGenerateMipmaps: NSNumber(booleanLiteral: true) ,MTKTextureLoaderOptionAllocateMipmaps: NSNumber(booleanLiteral: true), MTKTextureLoaderOptionOrigin : NSString(string: MTKTextureLoaderOriginFlippedVertically), MTKTextureLoaderOptionSRGB : NSNumber(booleanLiteral : srgb)])
	}
	
	func newTextureCubemap(withContentsOf url: URL, srgb: Bool)-> MTLTexture {
		return try! self.newTexture(withContentsOf: url, options: [MTKTextureLoaderOptionGenerateMipmaps: NSNumber(booleanLiteral: true) ,MTKTextureLoaderOptionAllocateMipmaps: NSNumber(booleanLiteral: true), MTKTextureLoaderOptionOrigin : NSString(string: MTKTextureLoaderOriginFlippedVertically), MTKTextureLoaderOptionSRGB : NSNumber(booleanLiteral : srgb), MTKTextureLoaderOptionCubeLayout : NSString(string: MTKTextureLoaderCubeLayoutVertical) ])
	}
	
}
