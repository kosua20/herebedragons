#ifndef MeshUtilities_h
#define MeshUtilities_h

#include "../common.hpp"
#include <array>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 binormal;
	glm::vec2 texCoord;
	
	// Binding location, rate of input.
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
	
	// Attribute layout.
	static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};
		// Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		// Normal
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);
		// Tangent
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, tangent);
		// Binormal
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, binormal);
		// UV
		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(Vertex, texCoord);
		return attributeDescriptions;
	}
	
};

typedef struct {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
} Mesh;



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

#endif 
