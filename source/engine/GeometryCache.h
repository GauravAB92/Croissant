#pragma once

#include <engine/ModelLoader.h>
#include <render/backend/DeviceManager.h>
#include <core/VFS.h>

namespace croissant
{
	class GeometryCache
	{
	public:

		struct MeshRange
		{
			uint32_t vertexOffset;
			uint32_t indexOffset;
			uint32_t indexCount;
		};

		void addMesh(uint32_t meshIdx, const std::vector<Vertex>& vertices,
			const std::vector<uint32_t>& indices);

		void upload(nvrhi::IDevice* device, nvrhi::ICommandList* cmd);

		GeometryCache() {};
		~GeometryCache()
		{

		}

		std::vector<Vertex>					m_Vertices; // All vertices of all meshes
		std::vector<uint32_t>				m_Indices; // All indices of all meshes

		nvrhi::BufferHandle					m_VertexBuffer; //ALL vertices of all meshes are stored in this buffer, with offsets defined in m_MeshRanges
		nvrhi::BufferHandle					m_IndexBuffer; //ALL indices of all meshes are stored in this buffer, with offsets defined in m_MeshRanges
		std::vector<MeshRange>				m_MeshRanges; // mesh index -> vertex/index offset and count
	};
};