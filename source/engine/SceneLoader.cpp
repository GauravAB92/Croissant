#include "SceneLoader.h"

namespace croissant
{
	std::vector<Vertex> extractVertices(const aiMesh* mesh)
	{
		std::vector<Vertex> vertices;
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			Vertex vertex;
			glm::vec3 vector; // temporary vector to transfer Assimp's aiVector3D to GLM's vec3

			// Extract vertex positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.position = vector;

			// Extract normals (if they exist)
			if (mesh->mNormals) {
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.normal = vector;
			}

			// Extract texture coordinates (if they exist)
			if (mesh->mTextureCoords[0]) { // Assimp allows up to 8 different texture coordinates
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.uv = vec;
			}
			else {
				vertex.uv = glm::vec2(0.0f, 0.0f);
			}

			vertices.push_back(vertex);
		}

		return vertices;
	}

	std::vector<uint32_t> extractIndices(const aiMesh* mesh)
	{
		std::vector<uint32_t> indices;

		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			const aiFace& face = mesh->mFaces[i];
			assert(face.mNumIndices == 3); // Ensure the face is a triangle
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
			}
		}
	}

	GpuMaterialData extractMaterial(const aiMaterial* mat, const aiScene* scene)
	{
		GpuMaterialData gm;

		// ?? pbrMetallicRoughness ??????????????????????????????????????????

		aiColor4D baseColor;
		if (mat->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS)
			gm.baseColorFactor = { baseColor.r, baseColor.g, baseColor.b, baseColor.a };

		mat->Get(AI_MATKEY_METALLIC_FACTOR, gm.metallicFactor);
		mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, gm.roughnessFactor);

		gm.baseColorTextureIndex = 

		// Assimp maps glTF metallicRoughness ? aiTextureType_DIFFUSE_ROUGHNESS
		gm.metallicRoughnessTexIndex =
			loadTex(mat, aiTextureType_DIFFUSE_ROUGHNESS, scene, basePath, texCache, catalog,
				/*srgb=*/false);

		// ?? top-level material fields ?????????????????????????????????????

		mat->Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0),
			gm.normalScale);

		gm.normalTexIndex =
			loadTex(mat, aiTextureType_NORMALS, scene, basePath, texCache, catalog,
				/*srgb=*/false);

		mat->Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_LIGHTMAP, 0),
			gm.occlusionStrength);

		// Assimp maps glTF occlusion ? aiTextureType_LIGHTMAP
		gm.occlusionTexIndex =
			loadTex(mat, aiTextureType_LIGHTMAP, scene, basePath, texCache, catalog,
				/*srgb=*/false);

		aiColor3D emissive;
		if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
			gm.emissiveFactor = { emissive.r, emissive.g, emissive.b };

		gm.emissiveTexIndex =
			loadTex(mat, aiTextureType_EMISSIVE, scene, basePath, texCache, catalog,
				/*srgb=*/true);

		// ?? alpha ?????????????????????????????????????????????????????????

		aiString alphaMode;
		if (mat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
			if (strcmp(alphaMode.C_Str(), "MASK") == 0) gm.alphaMode = 1;
			else if (strcmp(alphaMode.C_Str(), "BLEND") == 0) gm.alphaMode = 2;
		}
		mat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, gm.alphaCutoff);

		int doubleSided = 0;
		mat->Get(AI_MATKEY_TWOSIDED, doubleSided);
		gm.doubleSided = doubleSided;

		return gm;
	}

    bool SceneLoader::LoadScene(const char* filename,
                                Scene& scene,
                                GeometryCache& geo,
                                MaterialCache& mat)
    {
        Assimp::Importer importer;
        const aiScene* ai = importer.ReadFile(filename,
            aiProcess_Triangulate | aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);

		if (!ai || ai->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !ai->mRootNode)
		{
			logger::error("Assimp error: %s", importer.GetErrorString());
			return false;
		}

		// Load scene hieararchy and associate meshes/materials with nodes
		traverse(ai, scene, ai->mRootNode, -1, 0);

		LoadTextures(ai);
		LoadMeshes(ai, geo);
		LoadMaterials(ai, mat);
		return true;

    }

	void SceneLoader::LoadTextures(const aiScene* scene)
	{

		//Check for texture availability
		if (scene->mNumTextures == 0)
		{
			logger::warning("No textures found in the model.");
			return;
		}

		// Iterate through all textures in the scene
		for (unsigned int i = 0; i < scene->mNumTextures; i++)
		{
			const aiTexture* texture = scene->mTextures[i];
			if (texture->mHeight == 0) // Check if it's a regular texture
			{
				// Load texture data into nvrhi::TextureHandle
				// This is a placeholder, actual implementation will depend on nvrhi API
			}
		}
	}
	void SceneLoader::LoadMeshes(const aiScene* scene, GeometryCache& geo)
	{
		for (uint32_t i = 0; i < scene->mNumMeshes; i++)
		{
			const aiMesh* mesh = scene->mMeshes[i];
			geo.addMesh(i, extractVertices(mesh), extractIndices(mesh));
		}


	}
	void SceneLoader::LoadMaterials(const aiScene* scene, MaterialCache& mat)
	{
		// Iterate through all materials in the scene
		for (unsigned int i = 0; i < scene->mNumMaterials; i++)
		{
			const aiMaterial* material = scene->mMaterials[i];
			// Process material properties, textures, etc.
		}
	}

	bool SceneLoader::GenerateSubdividedMeshes(int levels)
	{
		/*if (!Mesh0)
		{
			logger::warning("No base mesh available for subdivision.");
			return false;
		}

#if SUBDIV_LINEAR
		// Always subdivides from Mesh0 directly
		// Pass i+1 so level 0 = 1 subdivision, level 1 = 4 triangles, etc.

		for (int i = 0; i < levels; i++)
		{
			std::unique_ptr<Mesh> subdividedMesh = std::make_unique<Mesh>();

			if (MeshOperations::LinearSubdivide(Mesh0, subdividedMesh.get(), i + 2))
			{
				logger::info("Linear subdivision level %d generated successfully.", i + 1);
				subdividedMeshes.push_back(std::move(subdividedMesh));
			}
			else
			{
				logger::warning("Failed to generate linear subdivision level %d.", i + 1);
				return false;
			}
		}
#else
		Mesh* firstLevel = Mesh0;
		for (int i = 0; i < levels; i++)
		{
			std::unique_ptr<Mesh> subdividedMesh = std::make_unique<Mesh>();
			if (MeshOperations::PlanarSubdivide(firstLevel, subdividedMesh.get()))
			{
				logger::info("Planar subdivision level %d generated successfully.", i + 1);
				subdividedMeshes.push_back(std::move(subdividedMesh));
				firstLevel = subdividedMeshes.back().get();
			}
			else
			{
				logger::warning("Failed to generate planar subdivision level %d.", i + 1);
				return false;
			}
		}
#endif

		return true;*/
			return true;
	}
}
