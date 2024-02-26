#include "MeshUtilities.hpp"
#include "Resources.hpp"
#include "Log.hpp"

#include <fstream>
#include <sstream>
#include <cstddef>
#include <map>

using namespace std;
using namespace DirectX;

void MeshUtilities::loadObj(const std::string & path, Mesh & mesh, MeshUtilities::LoadMode mode){
	
	std::stringstream in(Resources::loadStringFromExternalFile(path));
	
	//Init the mesh.
	mesh.indices.clear();
	mesh.vertices.clear();
	// Init temporary vectors.
	vector<XMFLOAT3> positions_temp;
	vector<XMFLOAT3> normals_temp;
	vector<XMFLOAT2> texcoords_temp;
	vector<string> faces_temp;

	string res;

	// Iterate over the lines of the file.
	while(!in.eof()){
		getline(in,res);

		// Ignore the line if it is too short or a comment.
		if(res.empty() || res[0] == '#' || res.size()<2){
			continue;
		}
		//We want to split the content of the line at spaces, use a stringstream directly
		stringstream ss(res);
		vector<string> tokens;
		string token;
		while(ss >> token){
			tokens.push_back(token);
		}
		if(tokens.size() < 1){
			continue;
		}
		// Check what kind of element the line represent.
		if (tokens[0] == "v") { // Vertex position
			// We need 3 coordinates.
			if(tokens.size() < 4){
				continue;
			}
			XMFLOAT3 pos = XMFLOAT3(stof(tokens[1],NULL),stof(tokens[2],NULL),stof(tokens[3],NULL));
			positions_temp.push_back(pos);
			
		} else if (tokens[0] == "vn"){ // Vertex normal
			// We need 3 coordinates.
			if(tokens.size() < 4){
				continue;
			}
			XMFLOAT3 nor = XMFLOAT3(stof(tokens[1],NULL),stof(tokens[2],NULL),stof(tokens[3],NULL));
			normals_temp.push_back(nor);
			
		} else if (tokens[0] == "vt") { // Vertex UV
			// We need 2 coordinates.
			if(tokens.size() < 3){
				continue;
			}
			XMFLOAT2 uv = XMFLOAT2(stof(tokens[1],NULL),stof(tokens[2],NULL));
			texcoords_temp.push_back(uv);
			
		} else if (tokens[0] == "f") { // Face indices.
			// We need 3 elements, each containing at most three indices.
			if(tokens.size() < 4){
				continue;
			}
			faces_temp.push_back(tokens[1]);
			faces_temp.push_back(tokens[2]);
			faces_temp.push_back(tokens[3]);

		} else { // Ignore s, l, g, matl or others
			continue;
		}
	}

	// If no vertices, end.
	if(positions_temp.size() == 0){
			return;
	}

	// Does the mesh have UV or normal coordinates ?
	bool hasUV = texcoords_temp.size()>0;
	bool hasNormals = normals_temp.size()>0;

	// Depending on the chosen extraction mode, we fill the mesh arrays accordingly.
	if (mode == MeshUtilities::Points){
		// Mode: Points
		// In this mode, we don't care about faces. We simply associate each vertex/normal/uv in the same order.
		mesh.vertices.resize(positions_temp.size());
		for(size_t vid = 0; vid < positions_temp.size(); ++vid){
			mesh.vertices[vid].pos = positions_temp[vid];
			if(hasNormals){
				mesh.vertices[vid].normal = normals_temp[vid];
			}
			if(hasUV){
				mesh.vertices[vid].texCoord = texcoords_temp[vid];
			}
		}

	} else if(mode == MeshUtilities::Expanded){
		// Mode: Expanded
		// In this mode, vertices are all duplicated. Each face has its set of 3 vertices, not shared with any other face.
		
		// For each face, query the needed positions, normals and uvs, and add them to the mesh structure.
		for(size_t i = 0; i < faces_temp.size(); i++){
			string str = faces_temp[i];
			size_t foundF = str.find_first_of("/");
			size_t foundL = str.find_last_of("/");
			
			// Positions (we are sure they exist).
			long ind1 = stol(str.substr(0,foundF))-1;
			mesh.vertices.emplace_back();
			mesh.vertices.back().pos = positions_temp[ind1];

			// UVs (second index).
			if(hasUV){
				long ind2 = stol(str.substr(foundF+1,foundL))-1;
				mesh.vertices.back().texCoord = texcoords_temp[ind2];
			}

			// Normals (third index, in all cases).
			if(hasNormals){
				long ind3 = stol(str.substr(foundL+1))-1;
				mesh.vertices.back().normal = normals_temp[ind3];
			}
			
			//Indices (simply a vector of increasing integers).
			mesh.indices.push_back((unsigned int)i);
		}

	} else if (mode == MeshUtilities::Indexed){
		// Mode: Indexed
		// In this mode, vertices are only duplicated if they were already used in a previous face with a different set of uv/normal coordinates.
		
		// Keep track of previously encountered (position,uv,normal).
		map<string,unsigned int> indices_used;

		//Positions
		unsigned int maxInd = 0;
		for(size_t i = 0; i < faces_temp.size(); i++){
			
			string str = faces_temp[i];

			//Does the association of attributs already exists ?
			if(indices_used.count(str)>0){
				// Just store the index in the indices vector.
				mesh.indices.push_back(indices_used[str]);
				// Go to next face.
				continue;
			}

			// else, query the associated position/uv/normal, store it, update the indices vector and the list of used elements.
			size_t foundF = str.find_first_of("/");
			size_t foundL = str.find_last_of("/");
			
			//Positions (we are sure they exist)
			unsigned int ind1 = stoi(str.substr(0,foundF))-1;
			mesh.vertices.emplace_back();
			mesh.vertices.back().pos = positions_temp[ind1];

			//UVs (second index)
			if(hasUV){
				unsigned int ind2 = stoi(str.substr(foundF+1,foundL))-1;
				mesh.vertices.back().texCoord = texcoords_temp[ind2];
			}
			//Normals (third index, in all cases)
			if(hasNormals){
				unsigned int ind3 = stoi(str.substr(foundL+1))-1;
				mesh.vertices.back().normal = normals_temp[ind3];
			}

			mesh.indices.push_back(maxInd);
			indices_used[str] = maxInd;
			maxInd++;
		}
		indices_used.clear();
	}

	positions_temp.clear();
	normals_temp.clear();
	texcoords_temp.clear();
	faces_temp.clear();

	logInfo("Mesh loaded with %llu faces and %llu vertices.\n", mesh.indices.size() / 3, mesh.vertices.size());
	
}

void MeshUtilities::centerAndUnitMesh(Mesh & mesh){
	// Compute the centroid.
	XMVECTOR centroid = XMVectorZero();
	for(size_t i = 0; i < mesh.vertices.size(); i++){
		centroid = XMVectorAdd(centroid, XMLoadFloat3(&mesh.vertices[i].pos));
	}
	centroid /= (float)mesh.vertices.size() ;

	float maxi = mesh.vertices[ 0 ].pos.x;
	for(size_t i = 0; i < mesh.vertices.size(); i++){
		// Translate  the vertex.
		XMStoreFloat3( &mesh.vertices[ i ].pos, XMLoadFloat3( &mesh.vertices[ i ].pos) - centroid );
		// Find the maximal distance from a vertex to the center.
		maxi = abs(mesh.vertices[i].pos.x) > maxi ? abs(mesh.vertices[i].pos.x) : maxi;
		maxi = abs(mesh.vertices[i].pos.y) > maxi ? abs(mesh.vertices[i].pos.y) : maxi;
		maxi = abs(mesh.vertices[i].pos.z) > maxi ? abs(mesh.vertices[i].pos.z) : maxi;
	}
	maxi = maxi == 0.0f ? 1.0f : maxi;
	
	// Scale the mesh.
	for( size_t i = 0; i < mesh.vertices.size(); i++ )
	{
		XMStoreFloat3( &mesh.vertices[ i ].pos, XMLoadFloat3( &mesh.vertices[ i ].pos ) / maxi );
	}

}

void MeshUtilities::computeTangentsAndBinormals(Mesh & mesh){
	if(mesh.indices.size() * mesh.vertices.size() == 0){
		// Missing data, or not the right mode (Points).
		return;
	}
	// Start by filling everything with 0 (as we want to accumulate tangents and binormals coming from different faces for each vertex).
	std::vector<XMVECTOR> tangents; 
	tangents.resize( mesh.vertices.size(), XMVectorZero() );
	std::vector<XMVECTOR> binormals; 
	binormals.resize( mesh.vertices.size(), XMVectorZero() );
	// Then, compute both vectors for each face and accumulate them.
	
	for(size_t fid = 0; fid < mesh.indices.size(); fid += 3){

		// Get the vertices of the face.
		XMVECTOR v0 = XMLoadFloat3( &mesh.vertices[ mesh.indices[ fid ] ].pos );
		XMVECTOR v1 = XMLoadFloat3( &mesh.vertices[ mesh.indices[ fid + 1 ] ].pos );
		XMVECTOR v2 = XMLoadFloat3( &mesh.vertices[ mesh.indices[ fid + 2 ] ].pos );
		// Get the uvs of the face.
		XMVECTOR uv0 = XMLoadFloat2(&mesh.vertices[mesh.indices[fid]].texCoord);
		XMVECTOR uv1 = XMLoadFloat2(&mesh.vertices[mesh.indices[fid+1]].texCoord);
		XMVECTOR uv2 = XMLoadFloat2(&mesh.vertices[mesh.indices[fid+2]].texCoord);

		// Delta positions and uvs.
		XMVECTOR deltaPosition1 = v1 - v0;
		XMVECTOR deltaPosition2 = v2 - v0;
		XMVECTOR deltaUv1 = uv1 - uv0;
		XMVECTOR deltaUv2 = uv2 - uv0;

		// Compute tangent and binormal for the face.
		float det = 1.0f / (XMVectorGetX(deltaUv1) * XMVectorGetY(deltaUv2) - XMVectorGetY(deltaUv1) * XMVectorGetX(deltaUv2));

		XMVECTOR tangent = det * ( XMVectorGetY( deltaUv2 ) * deltaPosition1 - XMVectorGetY( deltaUv1 ) * deltaPosition2);
		XMVECTOR binormal = det * ( XMVectorGetX( deltaUv1 ) * deltaPosition2 - XMVectorGetX( deltaUv2 ) * deltaPosition1);

    	// Accumulate them. We don't normalize to get a free weighting based on the size of the face.
		tangents[ mesh.indices[ fid ] ] += tangent;
		tangents[ mesh.indices[ fid + 1 ] ] += tangent;
		tangents[ mesh.indices[ fid + 2 ] ] += tangent;

    	binormals[mesh.indices[fid]] += binormal;
    	binormals[mesh.indices[fid+1]] += binormal;
    	binormals[mesh.indices[fid+2]] += binormal;
	}

	// Finally, enforce orthogonality and good orientation of the basis.
	for(size_t tid = 0; tid < mesh.vertices.size(); ++tid){
		XMVECTOR normal = XMLoadFloat3( &mesh.vertices[ tid ].normal );
		XMVECTOR tangent = tangents[ tid ] - XMVector3Dot( normal, tangents[ tid ] ) * normal;

		if( XMVectorGetX(XMVector3Dot(XMVector3Cross(normal, tangent), binormals[tid])) < 0.0f){
			tangent *= -1.0f;
 		}
		XMStoreFloat3( &mesh.vertices[ tid ].tangent, tangent );
		XMStoreFloat3( &mesh.vertices[ tid ].binormal, binormals[tid] );
	}

	logInfo("Mesh: %llu tangents and binormals computed.\n", mesh.vertices.size() );

}

