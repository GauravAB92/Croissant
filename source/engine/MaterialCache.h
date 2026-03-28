#pragma once

#include <glm/glm.hpp>
namespace croissant
{

	struct GpuMaterialData
	{
		glm::vec4				baseColorFactor = { 1, 1, 1, 1 }; // RGBA
		float					metallicFactor = 1.0f;
		float 					roughnessFactor = 1.0f;

		float					normalScale = 1.0f;
		float					occlusionStrength = 1.0f;
		glm::vec3				emissiveFactor = { 0, 0, 0 }; // RGB
		float					alphaCutoff = 0.5f;

		int32_t					baseColorTextureIndex = -1;
		int32_t                 baseColorSamplerIndex = 0;
		int32_t					metallicRoughnessTextureIndex = -1;
		int32_t					metallicRoughnessSamplerIndex = 0;
		int32_t					normalTextureIndex = -1;
		int32_t					normalSamplerIndex = 0;
		int32_t					occlusionTextureIndex = -1;
		int32_t					occlusionSamplerIndex = 0;
		int32_t					emissiveTextureIndex = -1;
		int32_t					emissiveSamplerIndex = 0;

		int32_t					alphaMode = 0; // 0 = OPAQUE, 1 = MASK, 2 = BLEND
		int32_t					doubleSided = 0; // 0 = false, 1 = true
	};
	static_assert(sizeof(GpuMaterialData) % 16 == 0, "GpuMaterialData must be 16-byte aligned for GPU constant buffer usage.");


	class MaterialCache
	{
		public:
			MaterialCache() {};
			~MaterialCache() {}

			void addMaterial(uint32_t matIdx, const GpuMaterialData& mat)
	};
}
