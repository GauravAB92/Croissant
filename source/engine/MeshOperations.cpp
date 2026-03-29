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
			uint32_t he0Idx = (uint32_t)outMesh->halfEdges.size();
			uint32_t he1Idx = (uint32_t)he0Idx + 1;
			uint32_t he2Idx = (uint32_t)he0Idx + 2;
			face.halfEdges = { he0Idx, he1Idx, he2Idx };

			uint32_t faceIdx = (uint32_t)outMesh->faces.size();
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

		uint32_t newIdx = (uint32_t)mesh->vertices.size();
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
		uint32_t numTriangles = (uint32_t)inMesh->indices.size() / 3;
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

	Vertex LerpVertex(const Vertex& a, const Vertex& b, float t)
	{
		Vertex v;
		v.position = a.position + t * (b.position - a.position);
		v.normal = glm::normalize(a.normal + t * (b.normal - a.normal));
		v.uv = a.uv + t * (b.uv - a.uv);
		return v;
	}

	bool MeshOperations::LinearSubdivide(const Mesh* inMesh, Mesh* outMesh, uint32_t level)
	{
		if (!inMesh || !outMesh) return false;
		if (inMesh->vertices.empty() || inMesh->indices.empty()) return false;
		if (level == 0) { return true; }

		outMesh->vertices.clear();
		outMesh->indices.clear();
		outMesh->halfEdges.clear();
		outMesh->faces.clear();

		const uint32_t n = level;
		const uint32_t numTriangles = (uint32_t)inMesh->indices.size() / 3;

		std::unordered_map<EdgeVertKey, uint32_t, EdgeVertKeyHash> edgeVertMap;
		// Max edge verts: 3 edges * (n-1) interior steps per edge * numTriangles / 2 (shared)
		edgeVertMap.reserve(3 * (n - 1) * numTriangles);

		// Copy original vertices — corners reuse these indices directly
		outMesh->vertices = inMesh->vertices;
		outMesh->indices.reserve(numTriangles * n * n * 3);

		for (uint32_t triIdx = 0; triIdx < numTriangles; ++triIdx)
		{
			uint32_t i0 = inMesh->indices[triIdx * 3 + 0];
			uint32_t i1 = inMesh->indices[triIdx * 3 + 1];
			uint32_t i2 = inMesh->indices[triIdx * 3 + 2];

			const Vertex& vA = inMesh->vertices[i0];
			const Vertex& vB = inMesh->vertices[i1];
			const Vertex& vC = inMesh->vertices[i2];

			const uint32_t gridSize = (n + 1) * (n + 2) / 2;
			std::vector<uint32_t> grid(gridSize);

			for (uint32_t row = 0; row <= n; ++row)
			{
				float tRow = (float)row / n;
				Vertex rowLeft = LerpVertex(vA, vB, tRow);
				Vertex rowRight = LerpVertex(vA, vC, tRow);

				for (uint32_t col = 0; col <= row; ++col)
				{
					float tCol = (row == 0) ? 0.f : (float)col / row;
					uint32_t& gridSlot = grid[row * (row + 1) / 2 + col];

					const bool isCornerA = (row == 0);
					const bool isCornerB = (row == n && col == 0);
					const bool isCornerC = (row == n && col == n);

					if (isCornerA) { gridSlot = i0; continue; }
					if (isCornerB) { gridSlot = i1; continue; }
					if (isCornerC) { gridSlot = i2; continue; }

					const bool isLeftEdge = (col == 0);             // A->B, step = row
					const bool isRightEdge = (col == row);           // A->C, step = row
					const bool isBottomEdge = (row == n);             // B->C, step = col

					if (isLeftEdge || isRightEdge || isBottomEdge)
					{
						// Canonical edge direction: always low index -> high index
						// so two triangles sharing the edge produce the same key
						uint32_t eV0, eV1, step;
						if (isLeftEdge) {
							// A->B, step goes 1..n-1
							eV0 = i0; eV1 = i1; step = row;
							if (eV0 > eV1) { std::swap(eV0, eV1); step = n - step; }
						}
						else if (isRightEdge) {
							// A->C, step goes 1..n-1
							eV0 = i0; eV1 = i2; step = row;
							if (eV0 > eV1) { std::swap(eV0, eV1); step = n - step; }
						}
						else {
							// B->C, step goes 1..n-1
							eV0 = i1; eV1 = i2; step = col;
							if (eV0 > eV1) { std::swap(eV0, eV1); step = n - step; }
						}

						EdgeVertKey key{ eV0, eV1, step, n };
						auto it = edgeVertMap.find(key);
						if (it != edgeVertMap.end()) {
							gridSlot = it->second;
						}
						else {
							uint32_t newIdx = (uint32_t)outMesh->vertices.size();
							outMesh->vertices.push_back(LerpVertex(rowLeft, rowRight, tCol));
							edgeVertMap[key] = newIdx;
							gridSlot = newIdx;
						}
					}
					else
					{
						// Interior — always unique
						gridSlot = (uint32_t)outMesh->vertices.size();
						outMesh->vertices.push_back(LerpVertex(rowLeft, rowRight, tCol));
					}
				}
			}

			auto idx = [&](uint32_t row, uint32_t col) -> uint32_t {
				return grid[row * (row + 1) / 2 + col];
				};

			for (uint32_t row = 0; row < n; ++row)
			{
				for (uint32_t col = 0; col <= row; ++col)
				{
					outMesh->indices.push_back(idx(row, col));
					outMesh->indices.push_back(idx(row + 1, col));
					outMesh->indices.push_back(idx(row + 1, col + 1));

					if (col > 0)
					{
						outMesh->indices.push_back(idx(row, col - 1));
						outMesh->indices.push_back(idx(row + 1, col));
						outMesh->indices.push_back(idx(row, col));
					}
				}
			}
		}

		GenerateHalfEdgeData(outMesh);
		GenerateAdjacencyIndices(outMesh);
		outMesh->minBounds = inMesh->minBounds;
		outMesh->maxBounds = inMesh->maxBounds;
		return true;
	}

}