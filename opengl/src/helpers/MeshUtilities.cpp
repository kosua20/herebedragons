#include "MeshUtilities.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstddef>
#include <map>

using namespace std;

void MeshUtilities::loadObj(const std::string & filename, Mesh & mesh, MeshUtilities::LoadMode mode){
	// Open the file.
	ifstream in;
	in.open(filename.c_str());
	if (!in) {
		cerr << filename + " is not a valid file." << endl;
		return;
	}
	//Init the mesh.
	mesh.indices.clear();
	mesh.positions.clear();
	mesh.normals.clear();
	mesh.texcoords.clear();
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
	in.close();

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
		
		mesh.positions = positions_temp;
		if(hasNormals){
			mesh.normals = normals_temp;
		}
		if(hasUV){
			mesh.texcoords = texcoords_temp;
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
			mesh.positions.push_back(positions_temp[ind1]);

			// UVs (second index).
			if(hasUV){
				long ind2 = stol(str.substr(foundF+1,foundL))-1;
				mesh.texcoords.push_back(texcoords_temp[ind2]);
			}

			// Normals (third index, in all cases).
			if(hasNormals){
				long ind3 = stol(str.substr(foundL+1))-1;
				mesh.normals.push_back(normals_temp[ind3]);
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
			mesh.positions.push_back(positions_temp[ind1]);

			//UVs (second index)
			if(hasUV){
				unsigned int ind2 = stoi(str.substr(foundF+1,foundL))-1;
				mesh.texcoords.push_back(texcoords_temp[ind2]);
			}
			//Normals (third index, in all cases)
			if(hasNormals){
				unsigned int ind3 = stoi(str.substr(foundL+1))-1;
				mesh.normals.push_back(normals_temp[ind3]);
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
	//cout << "OBJ: loaded. " << mesh.indices.size()/3 << " faces, " << mesh.positions.size() << " vertices, " << mesh.normals.size() << " normals, " << mesh.texcoords.size() << " texcoords." <<  endl;
	return;
}

void MeshUtilities::centerAndUnitMesh(Mesh & mesh){
	// Compute the centroid.
	glm::vec3 centroid = glm::vec3(0.0);
	float maxi = mesh.positions[0].x;
	for(size_t i = 0; i < mesh.positions.size(); i++){
		centroid += mesh.positions[i];
	}
	centroid /= mesh.positions.size();

	for(size_t i = 0; i < mesh.positions.size(); i++){
		// Translate  the vertex.
		mesh.positions[i] -= centroid;
		// Find the maximal distance from a vertex to the center.
		maxi = abs(mesh.positions[i].x) > maxi ? abs(mesh.positions[i].x) : maxi;
		maxi = abs(mesh.positions[i].y) > maxi ? abs(mesh.positions[i].y) : maxi;
		maxi = abs(mesh.positions[i].z) > maxi ? abs(mesh.positions[i].z) : maxi;
	}
	maxi = maxi == 0.0 ? 1.0 : maxi;
	
	// Scale the mesh.
	for(size_t i = 0; i < mesh.positions.size(); i++){
		mesh.positions[i] /= maxi;
	}

}

void MeshUtilities::computeTangentsAndBinormals(Mesh & mesh){
	if(mesh.indices.size() * mesh.positions.size() * mesh.texcoords.size() == 0){
		// Missing data, or not the right mode (Points).
		return;
	}
	// Start by filling everything with 0 (as we want to accumulate tangents and binormals coming from different faces for each vertex).
	for(size_t pid = 0; pid < mesh.positions.size(); ++pid){
		mesh.tangents.push_back(glm::vec3(0.0f));
		mesh.binormals.push_back(glm::vec3(0.0f));
	}
	// Then, compute both vectors for each face and accumulate them.
	for(size_t fid = 0; fid < mesh.indices.size(); fid += 3){

		// Get the vertices of the face.
		glm::vec3 & v0 = mesh.positions[mesh.indices[fid]];
		glm::vec3 & v1 = mesh.positions[mesh.indices[fid+1]];
		glm::vec3 & v2 = mesh.positions[mesh.indices[fid+2]];
		// Get the uvs of the face.
		glm::vec2 & uv0 = mesh.texcoords[mesh.indices[fid]];
		glm::vec2 & uv1 = mesh.texcoords[mesh.indices[fid+1]];
		glm::vec2 & uv2 = mesh.texcoords[mesh.indices[fid+2]];

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
    	mesh.tangents[mesh.indices[fid]] += tangent;
    	mesh.tangents[mesh.indices[fid+1]] += tangent;
    	mesh.tangents[mesh.indices[fid+2]] += tangent;

    	mesh.binormals[mesh.indices[fid]] += binormal;
    	mesh.binormals[mesh.indices[fid+1]] += binormal;
    	mesh.binormals[mesh.indices[fid+2]] += binormal;
	}
	// Finally, enforce orthogonality and good orientation of the basis.
	for(size_t tid = 0; tid < mesh.tangents.size(); ++tid){
		mesh.tangents[tid] = normalize(mesh.tangents[tid] - mesh.normals[tid] * dot(mesh.normals[tid], mesh.tangents[tid]));
		if(dot(cross(mesh.normals[tid], mesh.tangents[tid]), mesh.binormals[tid]) < 0.0f){
			mesh.tangents[tid] *= -1.0f;
 		}
	}
	//cout << "OBJ: " << mesh.tangents.size() << " tangents and binormals computed." << endl;
}

