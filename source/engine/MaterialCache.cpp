#include "MaterialCache.h"

namespace croissant
{
	void MaterialCache::addMaterial(const GpuMaterialData& mat)
	{
		m_Materials.push_back(mat);
	}


}