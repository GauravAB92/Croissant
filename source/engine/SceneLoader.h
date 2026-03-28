#pragma once

#include "Scene.h"
#include "GeometryCache.h"
#include "MaterialCache.h"

namespace croissant
{

	class SceneLoader
	{
	public:
		SceneLoader() {};
		~SceneLoader() {};

		bool LoadScene(const char* filename,
			Scene& scene,
			GeometryCache& geo,
			MaterialCache& mat);

	private:
		void LoadTextures(const aiScene* scene);
		void LoadMeshes(const aiScene* scene, GeometryCache& geo);
		void LoadMaterials(const aiScene* scene, MaterialCache& mat);
		bool GenerateSubdividedMeshes(int levels);


		void traverse(const aiScene* sourceScene, Scene& scene, aiNode* node,
			int parent, int atLevel);
	};
}

 