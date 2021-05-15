#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <set>
#include <map>

struct Object {
	std::vector<uint> edges;
	std::vector<float> vertices;
	unsigned int vCount = 0;
	unsigned int eCount = 0;
};

int main(int argc, char** argv){
	
	std::vector<Object> objects;

	// Inset an edge while ensuring uniqueness.
	auto insertEdge = [](std::set<std::pair<int, int>>& e, int v0, int v1){
		if (v0 < v1){
			e.insert(std::make_pair(v0, v1));
		} else {
			e.insert(std::make_pair(v1, v0));
		}
	};

	// Bounding box init.
	float mini[3];
	float maxi[3];

	for(int j = 0; j < 3; ++j){
		mini[j] = std::numeric_limits<float>::max();
		maxi[j] = -mini[j];
	}

	// Parse all OBJs.
	for(int i = 1; i < argc; ++i){
		const std::string path(argv[i]);
		std::ifstream file(path);

		if(!file.is_open()){
			std::cerr << "Unable to open file at path " << path << "." << std::endl;
			continue;
		}

		// Create new object.
		objects.emplace_back();
		Object & obj = objects.back();

		std::set<std::pair<int, int>> localEdges;

		std::string line;

		while(std::getline(file, line)){
			if(line.empty()){
				continue;
			}

			if(line[0] == 'v'){
				std::stringstream lineS(line.substr(2));
				float v[3];
				lineS >> v[0] >> v[1] >> v[2];
				// Store vertex and extend bounding box.
				for(int j = 0; j < 3; ++j){
					obj.vertices.push_back(v[j]);
					mini[j] = std::min(mini[j], v[j]);
					maxi[j] = std::max(maxi[j], v[j]);
				}
				++obj.vCount;

			} else if(line[0] == 'f'){
				std::stringstream lineS(line.substr(2));
				// Not necessarily a triangle.
				std::vector<int> ids;
				std::string ll;
				while(std::getline(lineS, ll, ' ')){
					ids.push_back(std::stoi(ll) - 1);
				}
				// Add all edges in set.
				for(unsigned int eid = 0; eid < ids.size(); ++eid){
					const unsigned int eidNext = (eid+1) % ids.size();
					insertEdge(localEdges, ids[eid], ids[eidNext]);
				}

			}
		}
		file.close();

		// Edges have been de-duplicated
		obj.eCount = localEdges.size();

		for(const auto& edge : localEdges){
			obj.edges.push_back(edge.first);
			obj.edges.push_back(edge.second);
		}

	}

	const size_t angleCount = 128;
	const float defaultRadius = 384.0f;

	for(size_t k = 0; k < objects.size(); ++k){
		
		const Object& obj = objects[k];

		const unsigned int vertex3DCount = obj.vCount * 3;
		const unsigned int vertex2DCount = obj.vCount * 2;

		std::vector<std::vector<int>> perAngleScreenVertices(angleCount);

		// For each angle, generate screen space vertices.
		for(size_t aId = 0; aId < angleCount; ++aId){
			// Precompute rotation.
			const float angle = float(aId) / float(angleCount) * 2.0 * M_PI; 
			const float co = cos(angle);
			const float si = sin(angle);

			const float cx = defaultRadius * co;
			const float cz = defaultRadius * si;

			perAngleScreenVertices[aId].resize( vertex2DCount );
			size_t d2Index = 0;

			for(size_t i = 0; i < vertex3DCount; i+=3){
				// Normalize vertex coordinates in bouding box, and move to -127,127.
				float xyz[3];
				for(int j = 0; j < 3; ++j){
					xyz[j] = (obj.vertices[i+j] - mini[j]) / (maxi[j] - mini[j]);
					xyz[j] = 127.0f * (2.f * xyz[j] - 1.f);
				}
				// Move to camera space.
				xyz[0] = xyz[0] - cx;
				xyz[2] = -(xyz[2] - cz);

				// Apply rotation.
				const float rx =  si * xyz[0] + co * xyz[2];
				const float rz = -co * xyz[0] + si * xyz[2];

				// Perspective division.
				const float dx = 127.0f * rx / rz;
				const float dy = 127.0f * xyz[1] / rz;

				// Ensure we stay in -127, 127 and convert to char.
				perAngleScreenVertices[aId][d2Index++] = std::min( std::max( int(std::round(dx)), -127), 127);
				perAngleScreenVertices[aId][d2Index++] = std::min( std::max( int(std::round(dy)), -127), 127);
				
			}

		}

		// Output vertices.
		std::cout << "const int8_t vertices" << k << "[][" << vertex2DCount << "] = { " << "\n";

		for(size_t aId = 0; aId < angleCount; ++aId){
			// For each angle.
			std::cout << "\t{" << "\n";

			const auto& screenVerts = perAngleScreenVertices[aId];
			for(size_t v = 0; v < vertex2DCount; v += 2){
				// Y first.
				std::cout << "\t\t" << screenVerts[v+1] << ", " << screenVerts[v] << ", "  << "\n";
			}

			std::cout << "\t}," << "\n";
		}

		std::cout << "};" << "\n";

		// Output edge vertex indices.
		std::cout << "const uint16_t edges" << k << "[] = { " << "\n";
		
		std::cout << std::hex;
		for(size_t i = 0; i < obj.edges.size(); i+=2){
			// Premultiply by 2.
			const unsigned int doubleId0 = 2 * obj.edges[i];
			const unsigned int doubleId1 = 2 * obj.edges[i+1];
			// Pack in 16 bits
			const unsigned int packedIds = (doubleId0 << 8) | doubleId1;
			// Store as hex.
			std::cout << "\t0x" << packedIds << ", \n";
		}
		std::cout << std::dec;

		std::cout << "};" << "\n\n";

	}

	
	std::cout << "const uint8_t edgeCounts[] = {\n";
	for(const auto & obj : objects){
		std::cout << "\t" << obj.eCount << "," << std::endl;
	}
	std::cout << "};\n\n";
	
	
	
	return 0;
}
