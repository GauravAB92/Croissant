#pragma once
#include <render/backend/DeviceManager.h>
#include <glm.hpp>
#include <memory>
#include <core/VFS.h>
#include <render/backend/ShaderUtils.h>

using namespace nvrhi;

struct BlitConstants
{
    glm::vec2  sourceOrigin;
    glm::vec2  sourceSize;

    glm::vec2  targetOrigin;
    glm::vec2  targetSize;
};

enum class BlitSampler
{
    Point,
    Linear,
    Sharpen
};

struct BlitParameters
{
    nvrhi::IFramebuffer* targetFramebuffer = nullptr;
    nvrhi::Viewport targetViewport;
//    dm::box2 targetBox = dm::box2(0.f, 1.f);

    nvrhi::ITexture* sourceTexture = nullptr;
    uint32_t sourceArraySlice = 0;
    uint32_t sourceMip = 0;
 //   dm::box2 sourceBox = dm::box2(0.f, 1.f);
    nvrhi::Format sourceFormat = nvrhi::Format::UNKNOWN;

    BlitSampler sampler = BlitSampler::Linear;
    nvrhi::BlendState::RenderTarget blendState;
    nvrhi::Color blendConstantColor = nvrhi::Color(0.f);
};


class CommonRenderPasses
{
public:

    CommonRenderPasses(nvrhi::IDevice* device, std::shared_ptr<vfs::RootFileSystem>& fs, uint32_t width, uint32_t height);
    void BlitTexture(nvrhi::ICommandList* commandList, const BlitParameters& params);
    void BlitTexturePointSampled(nvrhi::ICommandList* commandList,nvrhi::ITexture* sourceTexture, nvrhi::IFramebuffer* targetTexture);
    void BlitTextureLinearSampled(nvrhi::ICommandList* commandList, nvrhi::ITexture* sourceTexture, nvrhi::IFramebuffer* targetTexture);
    void RebuildShaders(DeviceManager* deviceManager, std::shared_ptr<vfs::RootFileSystem>& fs);

    SamplerHandle m_PointClampSampler;
	SamplerHandle m_LinearClampSampler;
	SamplerHandle m_LinearWrapSampler;
	SamplerHandle m_AnisotropicWrapSampler;

	DeviceHandle         m_Device;
	ShaderHandle         m_RectVS;
	ShaderHandle         m_BlitPS;
	BindingLayoutHandle  m_BlitBindingLayout;
};