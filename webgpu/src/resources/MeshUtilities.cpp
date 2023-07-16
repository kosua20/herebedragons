#include "MeshUtilities.hpp"
#include "Resources.hpp"

#include <fstream>
#include <sstream>
#include <cstddef>
#include <map>

using namespace std;

void MeshUtilities::loadObj(const std::string & path, Mesh & mesh, MeshUtilities::LoadMode mode){
	
	std::stringstream in(Resources::loadStringFromExternalFile(path));
	
	//Init the mesh.
	mesh.indices.clear();
	mesh.vertices.clear();
	// Init temporary vectors.
	vector<glm::vec3> positions_temp;
	vector<glm::vec3> normals_temp;
	vector<glm::vec2> texcoords_temp;
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
			glm::vec3 pos = glm::vec3(stof(tokens[1],NULL),stof(tokens[2],NULL),stof(tokens[3],NULL));
			positions_temp.push_back(pos);
			
		} else if (tokens[0] == "vn"){ // Vertex normal
			// We need 3 coordinates.
			if(tokens.size() < 4){
				continue;
			}
			glm::vec3 nor = glm::vec3(stof(tokens[1],NULL),stof(tokens[2],NULL),stof(tokens[3],NULL));
			normals_temp.push_back(nor);
			
		} else if (tokens[0] == "vt") { // Vertex UV
			// We need 2 coordinates.
			if(tokens.size() < 3){
				continue;
			}
			glm::vec2 uv = glm::vec2(stof(tokens[1],NULL),stof(tokens[2],NULL));
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
	std::cout << "Mesh loaded with " << mesh.indices.size()/3 << " faces, " << mesh.vertices.size() << " vertices." << std::endl;
	
	
}

void MeshUtilities::centerAndUnitMesh(Mesh & mesh){
	// Compute the centroid.
	glm::vec3 centroid = glm::vec3(0.0);
	float maxi = mesh.vertices[0].pos.x;
	for(size_t i = 0; i < mesh.vertices.size(); i++){
		centroid += mesh.vertices[i].pos;
	}
	centroid /= mesh.vertices.size();

	for(size_t i = 0; i < mesh.vertices.size(); i++){
		// Translate  the vertex.
		mesh.vertices[i].pos -= centroid;
		// Find the maximal distance from a vertex to the center.
		maxi = abs(mesh.vertices[i].pos.x) > maxi ? abs(mesh.vertices[i].pos.x) : maxi;
		maxi = abs(mesh.vertices[i].pos.y) > maxi ? abs(mesh.vertices[i].pos.y) : maxi;
		maxi = abs(mesh.vertices[i].pos.z) > maxi ? abs(mesh.vertices[i].pos.z) : maxi;
	}
	maxi = maxi == 0.0f ? 1.0f : maxi;
	
	// Scale the mesh.
	for(size_t i = 0; i < mesh.vertices.size(); i++){
		mesh.vertices[i].pos /= maxi;
	}

}

void MeshUtilities::computeTangentsAndBinormals(Mesh & mesh){
	if(mesh.indices.size() * mesh.vertices.size() == 0){
		// Missing data, or not the right mode (Points).
		return;
	}
	// Start by filling everything with 0 (as we want to accumulate tangents and binormals coming from different faces for each vertex).
	for(size_t pid = 0; pid < mesh.vertices.size(); ++pid){
		mesh.vertices[pid].tangent = glm::vec3(0.0f);
		mesh.vertices[pid].binormal = glm::vec3(0.0f);
	}
	// Then, compute both vectors for each face and accumulate them.
	for(size_t fid = 0; fid < mesh.indices.size(); fid += 3){

		// Get the vertices of the face.
		glm::vec3 & v0 = mesh.vertices[mesh.indices[fid]].pos;
		glm::vec3 & v1 = mesh.vertices[mesh.indices[fid+1]].pos;
		glm::vec3 & v2 = mesh.vertices[mesh.indices[fid+2]].pos;
		// Get the uvs of the face.
		glm::vec2 & uv0 = mesh.vertices[mesh.indices[fid]].texCoord;
		glm::vec2 & uv1 = mesh.vertices[mesh.indices[fid+1]].texCoord;
		glm::vec2 & uv2 = mesh.vertices[mesh.indices[fid+2]].texCoord;

		// Delta positions and uvs.
		glm::vec3 deltaPosition1 = v1 - v0;
		glm::vec3 deltaPosition2 = v2 - v0;
		glm::vec2 deltaUv1 = uv1 - uv0;
		glm::vec2 deltaUv2 = uv2 - uv0;

		// Compute tangent and binormal for the face.
		float det = 1.0f / (deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x);
    	glm::vec3 tangent = det * (deltaPosition1 * deltaUv2.y   - deltaPosition2 * deltaUv1.y);
    	glm::vec3 binormal = det * (deltaPosition2 * deltaUv1.x   - deltaPosition1 * deltaUv2.x);

    	// Accumulate them. We don't normalize to get a free weighting based on the size of the face.
    	mesh.vertices[mesh.indices[fid]].tangent += tangent;
    	mesh.vertices[mesh.indices[fid+1]].tangent += tangent;
    	mesh.vertices[mesh.indices[fid+2]].tangent += tangent;

    	mesh.vertices[mesh.indices[fid]].binormal += binormal;
    	mesh.vertices[mesh.indices[fid+1]].binormal += binormal;
    	mesh.vertices[mesh.indices[fid+2]].binormal += binormal;
	}
	// Finally, enforce orthogonality and good orientation of the basis.
	for(size_t tid = 0; tid < mesh.vertices.size(); ++tid){
		mesh.vertices[tid].tangent = normalize(mesh.vertices[tid].tangent - mesh.vertices[tid].normal * dot(mesh.vertices[tid].normal, mesh.vertices[tid].tangent));
		if(dot(cross(mesh.vertices[tid].normal, mesh.vertices[tid].tangent), mesh.vertices[tid].binormal) < 0.0f){
			mesh.vertices[tid].tangent *= -1.0f;
 		}
	}
	std::cout << "Mesh: " << mesh.vertices.size() << " tangents and binormals computed." << std::endl;
}

