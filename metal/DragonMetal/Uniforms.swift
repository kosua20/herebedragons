//
//  Uniforms.swift
//  DragonMetal
//
//  Created by Simon Rodriguez on 15/06/2017.
//  Copyright © 2017 Simon Rodriguez. All rights reserved.
//

import Foundation
import simd

// Transformation matrices specific to the object.
struct ObjectConstants {
	var mvp = matrix_identity_float4x4
	var invmv = matrix_identity_float4x4
	var mv = matrix_identity_float4x4
	var mvpLight = matrix_identity_float4x4
}


// Material parameters.
struct MaterialConstants {
	var shininess : Int = 0
}


// Scene parameters.
struct GlobalConstants {
	var lightDir = float4(0.0)
}
