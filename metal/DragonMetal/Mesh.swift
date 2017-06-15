//
//  MeshLoader.swift
//
//  Created by Simon Rodriguez on 21/09/2015.
//  Copyright © 2015 Simon Rodriguez. All rights reserved.
//

import Foundation
import simd
import MetalKit

struct FaceIndices : Hashable, Equatable {
	var v : Int
	var t : Int
	var n : Int
	var hashValue : Int {
		return "\(v),\(t),\(n)".hash
	}
}

func ==(lhs: FaceIndices, rhs: FaceIndices) -> Bool {
	return (lhs.v == rhs.v && lhs.n == rhs.n && lhs.t == rhs.t)
}

struct Vertex {
	var position = float3(0.0,0.0,0.0)
	var normal = float3(0.0,0.0,0.0)
	var tangent = float3(0.0,0.0,0.0)
	var bitangent = float3(0.0,0.0,0.0)
	var texcoords = float2(0.0,0.0)
}

class Mesh {
	
	private var vertices : [Vertex] = []
	private var indices : [uint32] = []
	public var vertexBuffer : MTLBuffer? = nil
	public var indexBuffer : MTLBuffer? = nil
	public var indexCount : Int {
		get {
			return indices.count
		}
	}
	
	
	init?(url : URL) {
		guard let stringContent = try? String(contentsOf: url) else {
			print("Error")
			return nil
		}
		let lines = stringContent.components(separatedBy: CharacterSet.newlines)
		
		var positions : [float3] = []
		var normals : [float3] = []
		var uvs : [float2] = []
		var faces : [FaceIndices] = []
		
		for line in lines {
			if (line.hasPrefix("v ")){//Vertex
				var components = line.components(separatedBy: CharacterSet.whitespaces)
				positions.append( float3( Float(components[1])!, Float(components[2])!, Float(components[3])! ) )
			} else if (line.hasPrefix("vt ")) {//UV coords
				var components = line.components(separatedBy: CharacterSet.whitespaces)
				uvs.append( float2( Float(components[1])!, Float(components[2])! ) )
				
			} else if (line.hasPrefix("vn ")) {//Normal coords
				var components = line.components(separatedBy: CharacterSet.whitespaces)
				normals.append( float3( Float(components[1])!, Float(components[2])!, Float(components[3])! ) )
			} else if (line.hasPrefix("f ")) {//Face with vertices/uv/normals
				let components = line.components(separatedBy: CharacterSet.whitespaces)
				// Split each face 3 indices
				let splittedComponents = components.map({$0.components(separatedBy: "/")})
				
				for i in 1..<4 {
					let intComps = splittedComponents[i].map({ comp -> Int in
						return comp == "" ? 0 : Int(comp)!
						})
					faces.append(FaceIndices(v: intComps[0], t: intComps[1], n: intComps[2]))
				}
			}
		}
		
		if positions.isEmpty || faces.isEmpty || faces[0].v == 0 {
			print("Missing data")
			return nil
		}
			
		
		var facesDone : [FaceIndices : uint32] = [:]
		var currentIndex : uint32 = 0
		
		for faceItem in faces {
			if let indice = facesDone[faceItem]{
				indices.append(indice)
			} else {
				//New
				var vertex = Vertex()
				vertex.position = positions[faceItem.v - 1]
				
				if(faceItem.t > 0){
					vertex.texcoords = uvs[faceItem.t - 1]
				}
				
				if(faceItem.n > 0){
					vertex.normal = normals[faceItem.n - 1]
				}
				
				vertices.append(vertex)
				
				
				indices.append(currentIndex)
				facesDone[faceItem] = currentIndex
				currentIndex += 1
			}
		}
		print("OBJ: loaded. \(indices.count/3) faces, \(positions.count) vertices, \(normals.count) normals, \(uvs.count) texcoords.")
	}
	
	public func centerAndUnit(){
		var centroid = float3(0.0,0.0,0.0)
		
		for i in 0..<vertices.count {
			centroid += vertices[i].position;
		}
		
		centroid *= (1.0/Float(vertices.count))
		
		var maxi = vertices[0].position.x;
		for i in 0..<vertices.count {
			
			vertices[i].position -= centroid
			
			maxi = abs(vertices[i].position.x) > maxi ? abs(vertices[i].position.x) : maxi;
			maxi = abs(vertices[i].position.y) > maxi ? abs(vertices[i].position.y) : maxi;
			maxi = abs(vertices[i].position.z) > maxi ? abs(vertices[i].position.z) : maxi;
		}
		
		maxi = (maxi == 0.0 ? 1.0 : maxi)
		
		// Scale the mesh.
		for i in 0..<vertices.count {
			vertices[i].position *= (1.0/maxi)
		}
	}
	
	public func computeTangentFrame(){
		if indices.isEmpty || vertices.isEmpty {
			// Missing data, or not the right mode (Points).
			return;
		}
		
		// Then, compute both vectors for each face and accumulate them.
		for face in 0..<(indices.count/3) {
			let v0 = vertices[Int(indices[3*face  ])]
			let v1 = vertices[Int(indices[3*face+1])]
			let v2 = vertices[Int(indices[3*face+2])]
			
			
			// Delta positions and uvs.
			let deltaPosition1 = v1.position - v0.position
			let deltaPosition2 = v2.position - v0.position
			let deltaUv1 = v1.texcoords - v0.texcoords
			let deltaUv2 = v2.texcoords - v0.texcoords
			
			// Compute tangent and binormal for the face.
			let det = 1.0 / (deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x)
			let tangent = det * (deltaPosition1 * deltaUv2.y   - deltaPosition2 * deltaUv1.y)
			let bitangent = det * (deltaPosition2 * deltaUv1.x   - deltaPosition1 * deltaUv2.x)
			
			// Accumulate them. We don't normalize to get a free weighting based on the size of the face.
			vertices[Int(indices[3*face  ])].tangent += tangent
			vertices[Int(indices[3*face+1])].tangent += tangent
			vertices[Int(indices[3*face+2])].tangent += tangent
			
			vertices[Int(indices[3*face  ])].bitangent += bitangent
			vertices[Int(indices[3*face+1])].bitangent += bitangent
			vertices[Int(indices[3*face+2])].bitangent += bitangent
		}
		// Finally, enforce orthogonality and good orientation of the basis.
		for i in 0..<vertices.count {
			vertices[i].tangent = normalize(vertices[i].tangent - vertices[i].normal * dot(vertices[i].normal, vertices[i].tangent))
			if dot(cross(vertices[i].normal, vertices[i].tangent), vertices[i].bitangent) < 0.0 {
				vertices[i].tangent *= -1.0
			}
			vertices[i].bitangent = normalize(vertices[i].bitangent)
		}

	}
	
	public func setupBuffers(device : MTLDevice){
		vertexBuffer = device.makeBuffer(bytes: &vertices, length: vertices.count * MemoryLayout<Vertex>.stride, options: [])
		indexBuffer = device.makeBuffer(bytes: &indices, length: indices.count * MemoryLayout<uint32>.stride, options: [])
	}
}






