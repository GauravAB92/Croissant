#include <engine/MeshOperations.h>
#include <core/log.h>
;


namespace croissant
{

	bool MeshOperations::GenerateHalfEdgeData(Mesh* outMesh)
	{
		if (outMesh->vertices.empty() || outMesh->indices.empty()) { return false; }

		assert(outMesh->indices.size() % 3 == 0 && "Mesh indices must be a multiple of 3 for triangles.");

		const uint32_t triangleCount = static_cast<uint32_t>(outMesh->indices.size() / 3);

		//Pre-allocate memory
		outMesh->halfEdges.clear();
		outMesh->halfEdges.reserve(outMesh->indices.size());
		outMesh->faces.clear();
		outMesh->faces.reserve(triangleCount);

		// Map Edge to HalfEdgeInfo
		std::unordered_map<EdgeKey, EdgeInfo, EdgeKeyHash> edgeMap;
		edgeMap.reserve(outMesh->indices.size());


		// For each triangle we create half-edges
		for (uint32_t triIdx = 0; triIdx < triangleCount; ++triIdx)
		{
			uint32_t v0 = outMesh->indices[triIdx * 3 + 0];
			uint32_t v1 = outMesh->indices[triIdx * 3 + 1];
			uint32_t v2 = outMesh->indices[triIdx * 3 + 2];

			// Create face
			Face face;
			uint32_t he0Idx = outMesh->halfEdges.size();
			uint32_t he1Idx = he0Idx + 1;
			uint32_t he2Idx = he0Idx + 2;
			face.halfEdges = { he0Idx, he1Idx, he2Idx };

			uint32_t faceIdx = outMesh->faces.size();
			outMesh->faces.push_back(face);

			// Create half-edges
			HalfEdge he0, he1, he2;

			//he0: v0 -> v1
			he0.vert = v1;
			he0.next = face.halfEdges[1];
			he0.face = faceIdx;
			he0.twin = INVALID; //To be set later

			//he1: v1 -> v2
			he1.vert = v2;
			he1.next = face.halfEdges[2];
			he1.face = faceIdx;
			he1.twin = INVALID; //To be set later

			//he2: v2 -> v0
			he2.vert = v0;
			he2.next = face.halfEdges[0];
			he2.face = faceIdx;
			he2.twin = INVALID; //To be set later

			outMesh->halfEdges.push_back(he0);
			outMesh->halfEdges.push_back(he1);
			outMesh->halfEdges.push_back(he2);

			//Register edges and find twins
			ProcessEdge(outMesh, edgeMap, v0, v1, face.halfEdges[0]);
			ProcessEdge(outMesh, edgeMap, v1, v2, face.halfEdges[1]);
			ProcessEdge(outMesh, edgeMap, v2, v0, face.halfEdges[2]);

		}

		return true;
	}

	uint32_t getAdjVertexIndex(Mesh* mesh, uint32_t halfEdgeIdx)
	{
		uint32_t twinIdx = mesh->halfEdges[halfEdgeIdx].twin;
		uint32_t vertIdx = INVALID;
		if (twinIdx != INVALID)
		{
			uint32_t nextIdx = mesh->halfEdges[twinIdx].next;
			vertIdx = mesh->halfEdges[nextIdx].vert;
		}
		else
		{
			//Boundary edge, use the original vertex
			vertIdx = mesh->halfEdges[halfEdgeIdx].vert;
		}
		return vertIdx;
	}

	bool MeshOperations::GenerateAdjacencyIndices(Mesh* outMesh)
	{
		if (outMesh->vertices.empty() || outMesh->indices.empty()) { return false; }

		if (outMesh->halfEdges.empty() || outMesh->faces.empty())
		{
			logger::warning("MeshOperations::GenerateAdjacencyIndices: Half-edge data not found. Please generate half-edge data before generating adjacency indices.");
			return false;
		}

		outMesh->adjacencyIndices.clear();

		for (auto& face : outMesh->faces)
		{
			uint32_t he0 = face.halfEdges[0];
			uint32_t he1 = face.halfEdges[1];
			uint32_t he2 = face.halfEdges[2];

			// push v0 v0 adj v1 v1 adj v2 v2 adj
			outMesh->adjacencyIndices.push_back(outMesh->halfEdges[he2].vert);
			outMesh->adjacencyIndices.push_back(getAdjVertexIndex(outMesh, he0));
			outMesh->adjacencyIndices.push_back(outMesh->halfEdges[he0].vert);
			outMesh->adjacencyIndices.push_back(getAdjVertexIndex(outMesh, he1));
			outMesh->adjacencyIndices.push_back(outMesh->halfEdges[he1].vert);
			outMesh->adjacencyIndices.push_back(getAdjVertexIndex(outMesh, he2));
		}

		return true;
	}

	void MeshOperations::ProcessEdge(Mesh* outMesh, std::unordered_map<EdgeKey, EdgeInfo, EdgeKeyHash>& edgeMap, uint32_t fromVert, uint32_t toVert, uint32_t halfEdgeIdx)
	{
		EdgeKey key(fromVert, toVert);
		auto it = edgeMap.find(key);

		if (it == edgeMap.end())
		{
			// First half-edge for this edge
			EdgeInfo edgeInfo;
			edgeInfo.halfEdgeIdx = halfEdgeIdx;
			edgeInfo.fromVert = fromVert;
			edgeMap[key] = edgeInfo;
		}
		else
		{
			// Found the twin half-edge
			EdgeInfo& existing = it->second;
			uint32_t twinHalfEdgeIdx = existing.halfEdgeIdx;

			//Verify opposite direction
			if (existing.fromVert != toVert)
			{
				logger::warning("MeshOperations::ProcessEdge: Inconsistent edge direction detected when processing half-edges.");
				return;
			}

			// Set twin indices
			outMesh->halfEdges[halfEdgeIdx].twin = twinHalfEdgeIdx;
			outMesh->halfEdges[twinHalfEdgeIdx].twin = halfEdgeIdx;

			// Remove from map as both half-edges are now processed
			edgeMap.erase(it);
		}
	}


	uint32_t GetOrCreateMidpoint(
		Mesh* mesh,
		std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash>& midpointMap,
		uint32_t v0Idx,
		uint32_t v1Idx)
	{
		EdgeKey key(v0Idx, v1Idx);

		// Check if midpoint already exists
		auto it = midpointMap.find(key);
		if (it != midpointMap.end())
		{
			return it->second; // Return existing midpoint
		}

		// Create new midpoint vertex
		const Vertex& v0 = mesh->vertices[v0Idx];
		const Vertex& v1 = mesh->vertices[v1Idx];

		Vertex midpoint;
		midpoint.position = (v0.position + v1.position) * 0.5f;
		midpoint.uv = (v0.uv + v1.uv) * 0.5f;
		midpoint.normal = glm::normalize((v0.normal + v1.normal) * 0.5f);

		uint32_t newIdx = mesh->vertices.size();
		mesh->vertices.push_back(midpoint);

		// Store in map
		midpointMap[key] = newIdx;

		return newIdx;
	}

	bool MeshOperations::PlanarSubdivide(const Mesh* inMesh, Mesh* outMesh)
	{
		if (!inMesh || !outMesh) return false;
		if (inMesh->vertices.empty() || inMesh->indices.empty()) return false;

		// Clear output mesh
		outMesh->vertices.clear();
		outMesh->indices.clear();
		outMesh->halfEdges.clear();
		outMesh->faces.clear();

		// Copy original vertices
		outMesh->vertices = inMesh->vertices;

		// Map to store midpoint vertices: edge -> new vertex index
		// Key: (min_vert, max_vert), Value: new vertex index
		std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> midpointMap;

		// Reserve space (rough estimate: 4x triangles, 3x vertices)
		uint32_t numTriangles = inMesh->indices.size() / 3;
		outMesh->indices.reserve(numTriangles * 12); // 4 triangles * 3 indices each
		outMesh->vertices.reserve(inMesh->vertices.size() + numTriangles * 3);

		// Process each triangle
		for (uint32_t triIdx = 0; triIdx < numTriangles; ++triIdx)
		{
			uint32_t v0 = inMesh->indices[triIdx * 3 + 0];
			uint32_t v1 = inMesh->indices[triIdx * 3 + 1];
			uint32_t v2 = inMesh->indices[triIdx * 3 + 2];

			// Get or create midpoint vertices
			uint32_t m01 = GetOrCreateMidpoint(outMesh, midpointMap, v0, v1);
			uint32_t m12 = GetOrCreateMidpoint(outMesh, midpointMap, v1, v2);
			uint32_t m20 = GetOrCreateMidpoint(outMesh, midpointMap, v2, v0);

			// Create 4 new triangles (maintain winding order)
			// Corner triangle 0: v0, m01, m20
			outMesh->indices.push_back(v0);
			outMesh->indices.push_back(m01);
			outMesh->indices.push_back(m20);

			// Corner triangle 1: m01, v1, m12
			outMesh->indices.push_back(m01);
			outMesh->indices.push_back(v1);
			outMesh->indices.push_back(m12);

			// Corner triangle 2: m20, m12, v2
			outMesh->indices.push_back(m20);
			outMesh->indices.push_back(m12);
			outMesh->indices.push_back(v2);

			// Center triangle: m01, m12, m20
			outMesh->indices.push_back(m01);
			outMesh->indices.push_back(m12);
			outMesh->indices.push_back(m20);
		}


		GenerateHalfEdgeData(outMesh);
		GenerateAdjacencyIndices(outMesh);

		// Update bounding box
		outMesh->minBounds = inMesh->minBounds;
		outMesh->maxBounds = inMesh->maxBounds;

		return true;
	}
}