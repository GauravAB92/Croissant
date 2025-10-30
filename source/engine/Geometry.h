#pragma once

#include <engine/ModelLoader.h>
#include <render/backend/DeviceManager.h>
#include <core/VFS.h>

namespace croissant
{
	class Geometry
	{
	public:
		Geometry(const Mesh* mesh): m_Mesh(mesh) {};
		~Geometry()
		{
			if (m_VertexBuffer)
				m_VertexBuffer = nullptr;
			if (m_IndexBuffer)
				m_IndexBuffer = nullptr;
		}

		void Init(DeviceManager* deviceManager, nvrhi::CommandListHandle commandList);

		nvrhi::InputLayoutHandle m_InputLayout;

		nvrhi::BufferHandle m_ConstantBuffer;
		nvrhi::BufferHandle m_VertexBuffer;
		nvrhi::BufferHandle m_IndexBuffer;
		nvrhi::BufferHandle m_AdjacencyIB;

		const Mesh* m_Mesh;
	};
};