#pragma once

#include <core/stdafx.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#include <glm/glm.hpp>
#include <core/log.h>
#include <nvrhi/nvrhi.h>
#include <unordered_map>
#include <algorithm>
#include <engine/MeshOperations.h>

#include "Scene.h"


namespace croissant
{

	constexpr int MAX_SUBDIVISION_LEVELS = 20;

	class ModelLoader
	{
	public:
		ModelLoader(const char* filename, Scene& scene, glm::mat4 modelTransform);
		~ModelLoader();

		glm::mat4 m_MatModel = glm::mat4(1.0f); // Model matrix for transformations

	private:
		Assimp::Importer m_Importer;
		const aiScene* m_Scene = nullptr;

		void LoadScene(const aiScene* sourceScene, Scene& scene);
		void traverse(const aiScene* sourceScene, Scene& scene, aiNode* node,
			int parent, int atLevel);

		void LoadModel(const char* filename);
		void LoadTextures(const aiScene* scene);
		void LoadMeshes(const aiScene* scene);
		void LoadMaterials(const aiScene* scene);
		bool GenerateSubdividedMeshes(int levels);

	public:

		Mesh* Mesh0 = nullptr; // Current mesh
		std::unique_ptr<Mesh> defaultMesh;
		std::vector<std::unique_ptr<Mesh>> subdividedMeshes;
		bool isLoaded = false;
	};
};


