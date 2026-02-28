#include "CommonRenderPasses.h"



CommonRenderPasses::CommonRenderPasses(nvrhi::IDevice* device, std::shared_ptr<vfs::RootFileSystem>& fs, uint32_t width, uint32_t height) : m_Device(device)
{

	CompileShaderFileNVRHI("shaders/common/rect_vs.hlsl", "main", nvrhi::ShaderType::Vertex, nullptr, device, fs, m_RectVS);
	CompileShaderFileNVRHI("shaders/common/blit_ps.hlsl", "main", nvrhi::ShaderType::Pixel, nullptr, device, fs, m_BlitPS);

	auto samplerDesc = nvrhi::SamplerDesc()
		.setAllFilters(false)
		.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);

	m_PointClampSampler = m_Device->createSampler(samplerDesc);

	samplerDesc.setAllFilters(true);
	m_LinearClampSampler = m_Device->createSampler(samplerDesc);

	samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
	m_LinearWrapSampler = m_Device->createSampler(samplerDesc);

	samplerDesc.setMaxAnisotropy(16);
	m_AnisotropicWrapSampler = m_Device->createSampler(samplerDesc);

	{
		nvrhi::BindingLayoutDesc layoutDesc;
		layoutDesc.visibility = nvrhi::ShaderType::All;
		layoutDesc.bindings = {
			nvrhi::BindingLayoutItem::PushConstants(0, sizeof(BlitConstants)),
			nvrhi::BindingLayoutItem::Texture_SRV(0),
			nvrhi::BindingLayoutItem::Sampler(0)
		};

		m_BlitBindingLayout = m_Device->createBindingLayout(layoutDesc);
	}
}

void CommonRenderPasses::BlitTexture(nvrhi::ICommandList* commandList, const BlitParameters& params)
{
	assert(commandList);
	assert(params.targetFramebuffer);
	assert(params.sourceTexture);

	const FramebufferDesc& fbDesc = params.targetFramebuffer->getDesc();
	assert(fbDesc.colorAttachments.size() == 1);
	assert(fbDesc.colorAttachments[0].valid());


	const nvrhi::FramebufferInfoEx& fbinfo = params.targetFramebuffer->getFramebufferInfo();
	const nvrhi::TextureDesc& sourceDesc = params.sourceTexture->getDesc();

	assert(sourceDesc.dimension == nvrhi::TextureDimension::Texture2D);

	nvrhi::Viewport targetViewport = params.targetViewport;
	if (targetViewport.width() == 0 && targetViewport.height() == 0)
	{
		// If no viewport is specified, create one based on the framebuffer dimensions.
		// Note that the FB dimensions may not be the same as target texture dimensions, in case a non-zero mip level is used.
		targetViewport = nvrhi::Viewport(float(fbinfo.width), float(fbinfo.height));
	}

	nvrhi::IShader* shader = nullptr;
	switch (params.sampler)
	{
	case BlitSampler::Point:
	case BlitSampler::Linear: shader = m_BlitPS; break;
	default: assert(false);
	}

	nvrhi::GraphicsPipelineHandle pso;
	{
		nvrhi::GraphicsPipelineDesc psoDesc;
		psoDesc.bindingLayouts = { m_BlitBindingLayout };
		psoDesc.VS = m_RectVS;
		psoDesc.PS = shader;
		psoDesc.primType = nvrhi::PrimitiveType::TriangleStrip;
		psoDesc.renderState.rasterState.setCullNone();
		psoDesc.renderState.depthStencilState.depthTestEnable = false;
		psoDesc.renderState.depthStencilState.stencilEnable = false;
		psoDesc.renderState.blendState.targets[0] = params.blendState;
		pso = m_Device->createGraphicsPipeline(psoDesc, params.targetFramebuffer);
	}

	nvrhi::BindingSetDesc bindingSetDesc;
	{
		auto sourceDimension = sourceDesc.dimension;
		if (sourceDimension == nvrhi::TextureDimension::TextureCube || sourceDimension == nvrhi::TextureDimension::TextureCubeArray)
			sourceDimension = nvrhi::TextureDimension::Texture2DArray;

		auto sourceSubresources = nvrhi::TextureSubresourceSet(params.sourceMip, 1, params.sourceArraySlice, 1);

		bindingSetDesc.bindings = {
			nvrhi::BindingSetItem::PushConstants(0, sizeof(BlitConstants)),
			nvrhi::BindingSetItem::Texture_SRV(0, params.sourceTexture, params.sourceFormat, sourceSubresources, sourceDimension),
			nvrhi::BindingSetItem::Sampler(0, params.sampler == BlitSampler::Point ? m_PointClampSampler : m_LinearClampSampler)
		};
	}

	// If a binding cache is provided, get the binding set from the cache.
	// Otherwise, create one and then release it.
	nvrhi::BindingSetHandle sourceBindingSet;
	sourceBindingSet = m_Device->createBindingSet(bindingSetDesc, m_BlitBindingLayout);

	nvrhi::GraphicsState state;
	state.pipeline = pso;
	state.framebuffer = params.targetFramebuffer;
	state.bindings = { sourceBindingSet };
	state.viewport.addViewport(targetViewport);
	state.viewport.addScissorRect(nvrhi::Rect(targetViewport));
	state.blendConstantColor = params.blendConstantColor;

	glm::vec2 sourceDim = glm::vec2(sourceDesc.width, sourceDesc.height);
	glm::vec2 targetDim = glm::vec2(fbinfo.width, fbinfo.height);

	BlitConstants blitConstants = {};
	blitConstants.sourceOrigin = glm::vec2(0,0);
	blitConstants.sourceSize = sourceDim;
	blitConstants.targetOrigin = glm::vec2(0,0);
	blitConstants.targetSize = targetDim;

	commandList->setGraphicsState(state);

	commandList->setPushConstants(&blitConstants, sizeof(blitConstants));

	nvrhi::DrawArguments args;
	args.instanceCount = 1;
	args.vertexCount = 4;
	commandList->draw(args);


}

void CommonRenderPasses::BlitTexturePointSampled(nvrhi::ICommandList* commandList, nvrhi::ITexture* sourceTexture, nvrhi::IFramebuffer* targetFrameBuffer)
{
	assert(commandList);
	assert(targetFrameBuffer);
	assert(sourceTexture);

	BlitParameters params;
	params.targetFramebuffer = targetFrameBuffer;
	params.sourceTexture = sourceTexture;
	params.sampler = BlitSampler::Point;
	BlitTexture(commandList, params);
}

void CommonRenderPasses::BlitTextureLinearSampled(nvrhi::ICommandList* commandList, nvrhi::ITexture* sourceTexture, nvrhi::IFramebuffer* targetFrameBuffer)
{
	assert(commandList);
	assert(targetFrameBuffer);
	assert(sourceTexture);

	BlitParameters params;
	params.targetFramebuffer = targetFrameBuffer;
	params.sourceTexture = sourceTexture;
	params.sampler = BlitSampler::Linear;
	BlitTexture(commandList, params);

}

void CommonRenderPasses::RebuildShaders(DeviceManager* deviceManager, std::shared_ptr<vfs::RootFileSystem>& fs)
{
	CompileShaderFileNVRHI("shaders/common/rect_vs.hlsl", "main", nvrhi::ShaderType::Vertex, nullptr, deviceManager->GetDevice(), fs, m_RectVS);
	CompileShaderFileNVRHI("shaders/common/blit_ps.hlsl", "main", nvrhi::ShaderType::Pixel, nullptr, deviceManager->GetDevice(), fs, m_BlitPS);
}
