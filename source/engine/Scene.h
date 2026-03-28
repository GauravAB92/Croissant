#pragma once

#include <core/stdafx.h>
#include <glm/glm.hpp>

#include "Geometry.h"

namespace croissant
{

	const int MAX_NODE_LEVEL = 10;

	struct Hierarchy
	{
		int parent = -1;
		int firstChild = -1;
		int nextSibling = -1;
		int lastSibling = -1;
		int level = 0;
	};

	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;

		int addNode(int parent, int level);

		void markAsChanged(int nodeID);

		void recalculateGlobalTransforms();

		std::vector<glm::mat4> m_localTransforms;
		std::vector<glm::mat4> m_globalTransforms;
		std::vector<Hierarchy> m_hierarchy;

		std::unordered_map<uint32_t, uint32_t> m_meshForNode; // node index -> mesh index (should probably be renamed to geometries)
		std::unordered_map<uint32_t, uint32_t> m_materialForNode; // mesh index -> material index

		std::unordered_map<uint32_t, uint32_t> m_nameForNode; // node index -> name index
		std::vector<std::string> m_nodeNames; // name index -> name string
		std::vector<std::string> m_materialNames; // material index -> name string

		std::vector<int> m_changedAtThisFrame[MAX_NODE_LEVEL];
	};
}

