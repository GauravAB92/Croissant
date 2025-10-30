#include <engine/ModelLoader.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <render/Application.h>


namespace croissant
{
	ModelLoader::ModelLoader(const char* filename, glm::mat4 modelTransform) : m_MatModel(modelTransform)
	{
		LoadModel(filename);
	}
	ModelLoader::~ModelLoader()
	{
		// Cleanup if necessary
	}

	void ModelLoader::LoadModel(const char* filename)
	{
		m_Scene = m_Importer.ReadFile(filename,
			aiProcess_ConvertToLeftHanded	|
			aiProcess_CalcTangentSpace		|
			aiProcess_JoinIdenticalVertices |
			aiProcess_OptimizeMeshes		| 
			aiProcess_OptimizeGraph			|
			aiProcess_Triangulate			|
			aiProcess_GenBoundingBoxes );
	
		if (!m_Scene || m_Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_Scene->mRootNode)
		{
			logger::error("Assimp error: %s", m_Importer.GetErrorString());
			isLoaded = false;
		}

		isLoaded = true;

		LoadTextures(m_Scene);
		LoadMeshes(m_Scene);
		LoadMaterials(m_Scene);
		
		GenerateSubdividedMeshes(MAX_SUBDIVISION_LEVELS);

	//	Mesh0 = subdividedMeshes.empty() ? defaultMesh.get() : subdividedMeshes[4].get();
	}
	void ModelLoader::LoadTextures(const aiScene* scene)
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
	void ModelLoader::LoadMeshes(const aiScene* scene)
	{
		std::unique_ptr<Mesh> theMesh = std::make_unique<Mesh>();

		// Iterate through all meshes in the scene
		for (unsigned int i = 0; i < 1; i++)
		{
			aiMesh* mesh = scene->mMeshes[i];
		
			// Process vertices, indices, and other mesh data
			for (unsigned int j = 0; j < mesh->mNumVertices; j++)
			{
				glm::vec3  positions	= glm::vec3(0.0f);
				glm::vec2  uvs			= glm::vec2(0.0f);
				glm::vec3  normals		= glm::vec3(0.0f);

				//Check if vertex positions are available then extract
				if (mesh->HasPositions())
				{
					positions.x = (float)mesh->mVertices[j].x;
					positions.y = (float)mesh->mVertices[j].y;
					positions.z = (float)mesh->mVertices[j].z;

				}

				//Check if texture coordinates are available then extract
				if (mesh->HasTextureCoords(0))
				{
					uvs = glm::vec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
				}

				//Check if normals are available then extract
				if (mesh->HasNormals())
				{
					normals.x = (float)mesh->mNormals[j].x;
					normals.y = (float)mesh->mNormals[j].y;
					normals.z = (float)mesh->mNormals[j].z;
				}

				theMesh->vertices.emplace_back(Vertex{ positions, uvs, normals });
			}

			for (unsigned int j = 0; j < mesh->mNumFaces; j++)
			{
				const aiFace& face = mesh->mFaces[j];
				assert(face.mNumIndices == 3); // Ensure the face is a triangle

				for (int k = 0; k < face.mNumIndices; k++)
				{
					theMesh->indices.push_back(face.mIndices[k]);
				}
			}

			theMesh->maxBounds = glm::vec3(mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z);
			theMesh->minBounds = glm::vec3(mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z);
		}

		if (MeshOperations::GenerateHalfEdgeData(theMesh.get()))
		{
			logger::info("half-edge data generated successfully.");
			logger::info("expected half-edges: %d", theMesh->indices.size());
			logger::info("generated half-edges: %d", theMesh->halfEdges.size());
			logger::info("generated faces: %d", theMesh->faces.size());

			// Verify first face
			if(!theMesh->faces.empty())
			{
				const Face& firstFace = theMesh->faces[0];
				logger::info("First face half-edges:");
				for (size_t h = 0; h < firstFace.halfEdges.size(); ++h)
				{
					uint32_t heIdx = firstFace.halfEdges[h];
					const HalfEdge& he = theMesh->halfEdges[heIdx];
					logger::info("  Half-edge %d: vert=%d, twin=%d, next=%d, face=%d", heIdx, he.vert, he.twin, he.next, he.face);
				}
			}
		}
		else
		{
			logger::warning("Failed to generate half-edge data.");
		}

		if (MeshOperations::GenerateAdjacencyIndices(theMesh.get()))
		{
			logger::info("Adjacency indices generated successfully.");
		}
		else
		{
			logger::warning("Failed to generate adjacency indices.");
		}

		defaultMesh = std::move(theMesh);
		Mesh0 = defaultMesh.get();
	}
	void ModelLoader::LoadMaterials(const aiScene* scene)
	{
		// Iterate through all materials in the scene
		for (unsigned int i = 0; i < scene->mNumMaterials; i++)
		{
			const aiMaterial* material = scene->mMaterials[i];
			// Process material properties, textures, etc.
		}
	}
	bool ModelLoader::GenerateSubdividedMeshes(int levels)
	{
		if(!Mesh0)
		{
			logger::warning("No base mesh available for subdivision.");
			return false;
		}

		Mesh* firstLevel = Mesh0;
		for (int i = 0; i < levels; i++)
		{
			std::unique_ptr<Mesh> subdividedMesh = std::make_unique<Mesh>();
			if (MeshOperations::PlanarSubdivide(firstLevel, subdividedMesh.get()))
			{
				logger::info("Subdivision level %d generated successfully.", i + 1);
				subdividedMeshes.push_back(std::move(subdividedMesh));
			}
			else
			{
				logger::warning("Failed to generate subdivision level %d.", i + 1);
				return false;
			}

			firstLevel = subdividedMeshes.back().get();
		}
		return true;
	}
};