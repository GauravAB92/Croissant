#include "SceneTypes.h"

nvrhi::VertexAttributeDesc Croissant::GetVertexAttributeDesc(VertexAttribute attribute, const char* name, uint32_t bufferIndex)
{
	return nvrhi::VertexAttributeDesc();
}

const char* Croissant::MaterialDomainToString(MaterialDomain domain)
{
	return nullptr;
}

void Croissant::Material::FillConstantBuffer(MaterialConstants& constants) const
{
}

bool Croissant::Material::SetProperty(const std::string& name, const glm::vec4& value)
{
	return false;
}
