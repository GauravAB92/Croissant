#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#include <glm.hpp>
#include <core/log.h>

namespace croissant
{
	constexpr uint32_t INVALID = 0xFFFFFFFF;

	//Vertex layout
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
	};

	struct HalfEdge; // Forward declaration

	struct Face
	{
		std::vector<uint32_t> halfEdges; // Half-edges making up this face
	};

	struct HalfEdge
	{
		uint32_t vert;
		uint32_t twin;	// Index of the twin half-edge
		uint32_t next;  // Index of the next half-edge
		uint32_t face;  // Index of the face this half-edge belongs to

		bool isBoundary(const std::vector<HalfEdge>& halfEdges) const
		{
			return twin == INVALID;
		}
	};

	struct Edge 
	{
		uint32_t a, b;                        // canonical (a < b)
		bool operator==(const Edge& o) const noexcept { return a == o.a && b == o.b; }
	};

	struct EdgeHash 
	{
		size_t operator()(const Edge& e) const noexcept {
			return (uint64_t(e.a) << 32) ^ e.b; 
		}
	};

	// Helper: Create a unique key for an edge (order-independent)
	struct EdgeKey
	{
		uint32_t v0, v1;

		EdgeKey(uint32_t a, uint32_t b) {
			v0 = std::min(a, b);
			v1 = std::max(a, b);
		}

		bool operator<(const EdgeKey& other) const {
			return (v0 < other.v0) || (v0 == other.v0 && v1 < other.v1);
		}

		bool operator==(const EdgeKey& other) const {
			return v0 == other.v0 && v1 == other.v1;
		}
	};

	struct EdgeKeyHash
	{
		size_t operator()(const EdgeKey& key) const
		{
			return std::hash<uint32_t>()(key.v0) ^ (std::hash<uint32_t>()(key.v1) << 1);
		}
	};

	// Helper structure to track half-edges for an edge
	struct EdgeInfo {
		uint32_t halfEdgeIdx;     // Index of one half-edge
		uint32_t fromVert;        // Which vertex it goes FROM
	};


	struct Mesh
	{
		std::vector<Vertex>       vertices;
		std::vector<uint32_t>     indices;
		std::vector<uint32_t>	  adjacencyIndices;
		std::vector<HalfEdge>     halfEdges;
		std::vector<Face>         faces;

		glm::vec3 minBounds = glm::vec3(0.0f);
		glm::vec3 maxBounds = glm::vec3(0.0f);

		glm::vec3 GetBBoxCenter() const
		{
			return (minBounds + maxBounds) * 0.5f;
		}
	};

	class MeshOperations
	{
	public:
		MeshOperations() = default;
		~MeshOperations() = default;

		
		static bool GenerateHalfEdgeData(Mesh* outMesh);
		static bool GenerateAdjacencyIndices(Mesh* outMesh);
		static void ProcessEdge(Mesh* outMesh, std::unordered_map<EdgeKey, EdgeInfo, EdgeKeyHash>& edgeMap, uint32_t fromVert, uint32_t toVert, uint32_t halfEdgeIdx);
		static bool PlanarSubdivide(const Mesh* inMesh, Mesh* outMesh);
	};
};