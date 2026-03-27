#include "Scene.h"

int Scene::addNode(int parent, int level)
{
	const int node = (int)m_hierarchy.size();
	m_localTransforms.push_back(glm::mat4(1.0f));
	m_globalTransforms.push_back(glm::mat4(1.0f));
	m_hierarchy.push_back({ parent, -1, -1, -1, level });

	if (parent > -1)
	{
		const int s = m_hierarchy[parent].firstChild;
		if (s == -1)
		{
			m_hierarchy[parent].firstChild = node;
			m_hierarchy[node].lastSibling = node;
		}
		else
		{
			int dest = m_hierarchy[s].lastSibling;
			if (dest <= -1)
			{
				for (dest = s; m_hierarchy[dest].nextSibling != -1; dest = m_hierarchy[dest].nextSibling);
			}
			m_hierarchy[dest].nextSibling = node;
			m_hierarchy[s].lastSibling = node;
		}
	}

	m_hierarchy[node].level = level;
	m_hierarchy[node].nextSibling = -1;
	m_hierarchy[node].firstChild = -1;
	return node;
}

void Scene::markAsChanged(int nodeID)
{
	const int level = m_hierarchy[nodeID].level;
	m_changedAtThisFrame[level].push_back(nodeID);

	// Traverse down the hierarchy to mark all children as changed
	for (int s = m_hierarchy[nodeID].firstChild; s != -1; s = m_hierarchy[s].nextSibling)
	{
		markAsChanged(s);
	}
}

void Scene::recalculateGlobalTransforms()
{
	if (!m_changedAtThisFrame[0].empty())
	{
		//Handle root nodes first (those at level 0)
		const int c = m_changedAtThisFrame[0][0];
		m_globalTransforms[c] = m_localTransforms[c];
		m_changedAtThisFrame[0].clear();
	}

	//Then handle all other levels
	for (int i = 1; i < MAX_NODE_LEVEL && (!m_changedAtThisFrame[i].empty()); i++)
	{
		for (int c : m_changedAtThisFrame[i])
		{
			int p = m_hierarchy[c].parent;
			m_globalTransforms[c] = m_globalTransforms[p] * m_localTransforms[c];
		}
		m_changedAtThisFrame[i].clear();
	}
}
