#include "GeometryCache.h"
#include "MeshOperations.h"

namespace croissant
{

	void GeometryCache::addMesh(uint32_t meshIdx, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
	{
		MeshRange range;
		range.vertexOffset = (uint32_t)m_Vertices.size(); // Current total vertex count as offset
		range.indexOffset = (uint32_t)m_Indices.size();   // Current total index count as offset
		range.indexCount = (uint32_t)indices.size();
		m_MeshRanges.push_back(range);
		// Append vertices and indices to the buffers
		m_Vertices.insert(m_Vertices.end(), vertices.begin(), vertices.end());
		m_Indices.insert(m_Indices.end(), indices.begin(), indices.end());
	}

}