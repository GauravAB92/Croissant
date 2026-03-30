#pragma once
#include <glm/glm.hpp>
#include <nvrhi/nvrhi.h>
#include <memory>

struct MaterialConstants;

namespace Croissant
{
    //Vertex layout
    struct Vertex
    {
        glm::vec3 position;
        glm::vec2 uv;
        glm::vec3 normal;
    };

    enum class TextureAlphaMode
    {
        UNKNOWN = 0,
        STRAIGHT = 1,
        PREMULTIPLIED = 2,
        OPAQUE_ = 3,
        CUSTOM = 4,
    };

    struct LoadedTexture
    {
        nvrhi::TextureHandle texture;
        TextureAlphaMode alphaMode = TextureAlphaMode::UNKNOWN;
        uint32_t originalBitsPerPixel = 0;
       // DescriptorHandle bindlessDescriptor;
        std::string path;
        std::string mimeType;
    };

	enum class VertexAttribute
	{
		Position = 0,
		UV = 1,
		Normal = 2,
		Count
	};

	nvrhi::VertexAttributeDesc GetVertexAttributeDesc(VertexAttribute attribute, const char* name, uint32_t bufferIndex);


    // NOTE regarding MaterialDomain and transparency. It may seem that the Transparent attribute
    // is orthogonal to the blending mode (opaque, alpha-tested, alpha-blended). In glTF, it is
    // indeed an independent extension, KHR_materials_transmission, that can interact with the
    // blending mode. But enabling physical transmission on an object is an important change
    // for renderers: for example, rasterizers need to render "opaque" transmissive objects in a
    // separate render pass, together with alpha blended materials; ray tracers also need to
    // process transmissive objects in a different way from regular opaque or alpha-tested objects.
    // Specifying the transmission option in the material domain makes these requirements explicit.

    enum class MaterialDomain : uint8_t
    {
        Opaque,
        AlphaTested,
        AlphaBlended,
        Transmissive,
        TransmissiveAlphaTested,
        TransmissiveAlphaBlended,

        Count
    };

    const char* MaterialDomainToString(MaterialDomain domain);

    struct Material
    {
        std::string name;
        std::string modelFileName;      // where this material originated from, e.g. GLTF file name
        int materialIndexInModel = -1;  // index of the material in the model file
        MaterialDomain domain = MaterialDomain::Opaque;
        std::shared_ptr<LoadedTexture> baseOrDiffuseTexture; // metal-rough: base color; spec-gloss: diffuse color; .a = opacity (both modes)
        std::shared_ptr<LoadedTexture> metalRoughOrSpecularTexture; // metal-rough: ORM map; spec-gloss: specular color, .a = glossiness
        std::shared_ptr<LoadedTexture> normalTexture;
        std::shared_ptr<LoadedTexture> emissiveTexture;
        std::shared_ptr<LoadedTexture> occlusionTexture;
        std::shared_ptr<LoadedTexture> transmissionTexture; // see KHR_materials_transmission; undefined on specular-gloss materials
        std::shared_ptr<LoadedTexture> opacityTexture; // for renderers that store opacity or alpha mask separately, overrides baseOrDiffuse.a
        nvrhi::BufferHandle materialConstants;
        glm::vec3 baseOrDiffuseColor = glm::vec3(1.0f); // metal-rough: base color, spec-gloss: diffuse color (if no texture present)
        glm::vec3 specularColor = glm::vec3(0.f); // spec-gloss: specular color
        glm::vec3 emissiveColor = glm::vec3(0.f);
        float emissiveIntensity = 1.f; // additional multiplier for emissiveColor
        float metalness = 0.f; // metal-rough only
        float roughness = 0.f; // both metal-rough and spec-gloss
        float opacity = 1.f; // for transparent materials; multiplied by diffuse.a if present
        float alphaCutoff = 0.5f; // for alpha tested materials
        float transmissionFactor = 0.f; // see KHR_materials_transmission; undefined on specular-gloss materials
        float normalTextureScale = 1.f;
        float occlusionStrength = 1.f;
        glm::vec2 normalTextureTransformScale = glm::vec2(1.f);

        // Toggle between two PBR models: metal-rough and specular-gloss.
        // See the comments on the other fields here.
        bool useSpecularGlossModel = false;

        // Subsurface Scattering
        bool enableSubsurfaceScattering = false;
        struct SubsurfaceParams
        {
            glm::vec3 transmissionColor = glm::vec3(0.5f);
            glm::vec3 scatteringColor   = glm::vec3(0.5f);
            float scale = 1.0f;
            float anisotropy = 0.0f;
        } subsurface;

        // Hair
        bool enableHair = false;
        struct HairParams
        {
            glm::vec3 baseColor = glm::vec3(1.0f);
            float melanin = 0.5f;
            float melaninRedness = 0.5f;
            float longitudinalRoughness = 0.25f;
            float azimuthalRoughness = 0.6f;
            float diffuseReflectionWeight = 0.0f;
            glm::vec3 diffuseReflectionTint = glm::vec3(0.0f);
            float ior = 1.55f;
            float cuticleAngle = 3.0f;
        } hair;

        // Toggles for the textures. Only effective if the corresponding texture is non-null.
        bool enableBaseOrDiffuseTexture = true;
        bool enableMetalRoughOrSpecularTexture = true;
        bool enableNormalTexture = true;
        bool enableEmissiveTexture = true;
        bool enableOcclusionTexture = true;
        bool enableTransmissionTexture = true;
        bool enableOpacityTexture = true;

        bool doubleSided = false;

        // Useful when metalness and roughness are packed into a 2-channel texture for BC5 encoding.
        bool metalnessInRedChannel = false;

        int materialID = 0;
        bool dirty = true; // set this to true to make Scene update the material data

        virtual ~Material() = default;
        void FillConstantBuffer(struct MaterialConstants& constants) const;
        bool SetProperty(const std::string& name, const glm::vec4& value);
    };

    struct BufferGroup
    {
        nvrhi::BufferHandle indexBuffer;
        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle instanceBuffer;
        //std::shared_ptr<DescriptorHandle> indexBufferDescriptor;
        //std::shared_ptr<DescriptorHandle> vertexBufferDescriptor;
        //std::shared_ptr<DescriptorHandle> instnaceBufferDescriptor;
        std::array<nvrhi::BufferRange, size_t(VertexAttribute::Count)> vertexBufferRanges;
        std::vector<nvrhi::BufferRange> morphTargetBufferRange;
        std::vector<uint32_t> indexData;
        std::vector<glm::vec3> positionData;
        std::vector<glm::vec2> texcoord1Data;
        std::vector<glm::vec2> texcoord2Data;
        std::vector<uint32_t> normalData;
        std::vector<uint32_t> tangentData;
        //std::vector<dm::vector<uint16_t, 4>> jointData;
        //std::vector<dm::float4> weightData;
        //std::vector<float> radiusData;
        //std::vector<dm::float4> morphTargetData;

        [[nodiscard]] bool hasAttribute(VertexAttribute attr) const { return vertexBufferRanges[int(attr)].byteSize != 0; }
        nvrhi::BufferRange& getVertexBufferRange(VertexAttribute attr) { return vertexBufferRanges[int(attr)]; }
        [[nodiscard]] const nvrhi::BufferRange& getVertexBufferRange(VertexAttribute attr) const { return vertexBufferRanges[int(attr)]; }
    };

    enum class MeshGeometryPrimitiveType : uint8_t
    {
        Triangles,
        Lines,
        LineStrip,

        Count
    };

    struct MeshGeometry
    {
        std::shared_ptr<Material> material;
       // dm::box3 objectSpaceBounds;
        uint32_t indexOffsetInMesh = 0;
        uint32_t vertexOffsetInMesh = 0;
        uint32_t numIndices = 0;
        uint32_t numVertices = 0;
        int globalGeometryIndex = 0;

        MeshGeometryPrimitiveType type = MeshGeometryPrimitiveType::Triangles;

        virtual ~MeshGeometry() = default;
    };

    enum class MeshType : uint8_t
    {
        Triangles,
        CurvePolytubes,
        CurveDisjointOrthogonalTriangleStrips,
        CurveLinearSweptSpheres,

        Count
    };

    struct MeshInfo
    {
        std::string name;
        MeshType type = MeshType::Triangles;
        std::shared_ptr<BufferGroup> buffers;
        std::shared_ptr<MeshInfo> skinPrototype;
        std::vector<std::shared_ptr<MeshGeometry>> geometries;
        //dm::box3 objectSpaceBounds;
        uint32_t indexOffset = 0;
        uint32_t vertexOffset = 0;
        uint32_t totalIndices = 0;
        uint32_t totalVertices = 0;
        int globalMeshIndex = 0;
        bool isMorphTargetAnimationMesh = false;
        nvrhi::rt::AccelStructHandle accelStruct; // for use by applications
        bool isSkinPrototype = false;

        virtual ~MeshInfo() = default;
        bool IsCurve() const
        {
            return (type == MeshType::CurvePolytubes)
                || (type == MeshType::CurveDisjointOrthogonalTriangleStrips)
                || (type == MeshType::CurveLinearSweptSpheres);
        }
    };



}
