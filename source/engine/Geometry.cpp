#include "Geometry.h"

namespace croissant
{
	void Geometry::Init(DeviceManager* deviceManager, nvrhi::CommandListHandle commandList)
	{
		nvrhi::VertexAttributeDesc attributes[] =
		{
			nvrhi::VertexAttributeDesc()
				.setName("POSITION")
				.setFormat(nvrhi::Format::RGB32_FLOAT)
				.setOffset(0)
				.setBufferIndex(0)
				.setElementStride(sizeof(Vertex)),

			 nvrhi::VertexAttributeDesc()
				.setName("UV")
				.setFormat(nvrhi::Format::RG32_FLOAT)
				.setOffset(0)
				.setBufferIndex(1)
				.setElementStride(sizeof(Vertex)),

			nvrhi::VertexAttributeDesc()
				.setName("NORMAL")
				.setFormat(nvrhi::Format::RGB32_FLOAT)
				.setOffset(0)
				.setBufferIndex(2)
				.setElementStride(sizeof(Vertex)),
		};

		m_InputLayout = deviceManager->GetDevice()->createInputLayout(attributes, uint32_t(std::size(attributes)), nullptr);

		commandList->open();

		//Create vertex buffer
		nvrhi::BufferDesc vertexBufferDesc;
		vertexBufferDesc.isVertexBuffer = true;
		vertexBufferDesc.byteSize = m_Mesh->vertices.size() * sizeof(Vertex);
		vertexBufferDesc.debugName = "VertexBuffer";
		vertexBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
		m_VertexBuffer = deviceManager->GetDevice()->createBuffer(vertexBufferDesc);

		commandList->beginTrackingBufferState(m_VertexBuffer, nvrhi::ResourceStates::CopyDest);
		commandList->writeBuffer(m_VertexBuffer, m_Mesh->vertices.data(), m_Mesh->vertices.size() * sizeof(Vertex));
		commandList->setPermanentBufferState(m_VertexBuffer, nvrhi::ResourceStates::VertexBuffer);

		//Create index buffer
		nvrhi::BufferDesc indexBufferDesc;
		indexBufferDesc.isIndexBuffer = true;
		indexBufferDesc.byteSize = m_Mesh->indices.size() * sizeof(uint32_t);
		indexBufferDesc.debugName = "IndexBuffer";
		indexBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
		m_IndexBuffer = deviceManager->GetDevice()->createBuffer(indexBufferDesc);


		//Write to buffer, with state tracking
		commandList->beginTrackingBufferState(m_IndexBuffer, nvrhi::ResourceStates::CopyDest);
		commandList->writeBuffer(m_IndexBuffer, m_Mesh->indices.data(), m_Mesh->indices.size() * sizeof(uint32_t));
		commandList->setPermanentBufferState(m_IndexBuffer, nvrhi::ResourceStates::IndexBuffer);

		//Create adjacency index buffer
		nvrhi::BufferDesc indexBufferDescAdj;
		indexBufferDescAdj.isIndexBuffer = true;
		indexBufferDescAdj.byteSize = m_Mesh->adjacencyIndices.size() * sizeof(uint32_t);
		indexBufferDescAdj.debugName = "IndexBufferAdjacency";
		indexBufferDescAdj.initialState = nvrhi::ResourceStates::CopyDest;
		m_AdjacencyIB = deviceManager->GetDevice()->createBuffer(indexBufferDescAdj);

		//Write to buffer, with state tracking
		commandList->beginTrackingBufferState(m_AdjacencyIB, nvrhi::ResourceStates::CopyDest);
		commandList->writeBuffer(m_AdjacencyIB, m_Mesh->adjacencyIndices.data(), m_Mesh->adjacencyIndices.size() * sizeof(uint32_t));
		commandList->setPermanentBufferState(m_AdjacencyIB, nvrhi::ResourceStates::IndexBuffer);
		commandList->close();
		deviceManager->GetDevice()->executeCommandList(commandList);
	}
};