#pragma once

#include "Resources.hpp"
#include "../common.hpp"


class MeshUtilities {

public:
	
	// Three load modes: load the vertices without any connectivity (Points),
	// 					 load them with all vertices duplicated for each face (Expanded),
	//					 load them after duplicating only the ones that are shared between faces with multiple attributes (Indexed).
	enum LoadMode {
		Expanded, Points, Indexed
	};

	/// Load an obj file from disk into the mesh structure.
	static void loadObj(const std::string & path, Mesh & mesh, LoadMode mode);

	/// Center the mesh and scale it to fit in the [-1,1] box.
	static void centerAndUnitMesh(Mesh & mesh);

	/// Compute the tangents and binormal vectors for each vertex.
	static void computeTangentsAndBinormals(Mesh & mesh);
	
};
