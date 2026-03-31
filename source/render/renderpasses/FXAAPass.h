#pragma once

#include <render/backend/dx12/DXHelper.h>
#include <render/backend/DeviceManager.h>
#include <render/renderpasses/CommonRenderPasses.h>

#include <nvrhi/nvrhi.h>
#include <nvrhi/utils.h>

struct FXAAConstantsAligned
{
	BlitConstants base;
	glm::vec2  inverseScreenSize; 
	float      padding[54];
};

class FXAAPass
{
public:
	FXAAPass(DeviceManager* deviceManager, std::shared_ptr<vfs::RootFileSystem>& fs, nvrhi::CommandListHandle& commandList, std::shared_ptr<CommonRenderPasses> m_CommonRenderPasses) : m_CommonRenderPasses(m_CommonRenderPasses)
	{
		Init(deviceManager, fs, commandList);
	}

	void Init(DeviceManager* deviceManager, std::shared_ptr<vfs::RootFileSystem>& fs, nvrhi::CommandListHandle& commandList)
	{

		//Create Constant Buffer
		auto constantBufferDesc = nvrhi::BufferDesc()
			.setByteSize(sizeof(FXAAConstantsAligned))
			.setIsConstantBuffer(true);

		m_ConstantBuffer = deviceManager->GetDevice()->createBuffer(constantBufferDesc);

		//Compile Shaders
		CompileShaders(deviceManager, fs, commandList);

		//Create Layout
		{
			nvrhi::BindingLayoutDesc bindingLayoutDesc;
			bindingLayoutDesc.visibility = nvrhi::ShaderType::AllGraphics;
			bindingLayoutDesc.bindings =
			{
				nvrhi::BindingLayoutItem::ConstantBuffer(0),
				nvrhi::BindingLayoutItem::Texture_SRV(0),
				nvrhi::BindingLayoutItem::Sampler(0)
			};

			m_bindingLayout = deviceManager->GetDevice()->createBindingLayout(bindingLayoutDesc);
		}

		//Create sampler
		auto samplerDesc = nvrhi::SamplerDesc()
			.setAllFilters(true)
			.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);

		m_sampler = deviceManager->GetDevice()->createSampler(samplerDesc);

	}

	//Recreates framebuffer and textures. Called on init and on resize
	void CreateResources(nvrhi::IDevice* device, nvrhi::IFramebuffer* framebuffer, float viewportScale = 1.0f)
	{
		const nvrhi::FramebufferInfoEx& fbInfo = framebuffer->getFramebufferInfo();

		uint32_t width = static_cast<uint32_t>(fbInfo.width * viewportScale);
		uint32_t height = static_cast<uint32_t>(fbInfo.height * viewportScale);

		m_ColorBufferTexture = nullptr;
		m_DepthBuffer = nullptr;
		m_framebuffer = nullptr;
		m_Pipeline = nullptr;

		// Create unfiltered render target texture
		if (!m_ColorBufferTexture)
		{
			nvrhi::TextureDesc textureDesc;
			textureDesc.format = nvrhi::Format::SRGBA8_UNORM;
			textureDesc.isRenderTarget = true;
			textureDesc.width = width;
			textureDesc.height = height;
			textureDesc.dimension = nvrhi::TextureDimension::Texture2D;
			textureDesc.initialState = nvrhi::ResourceStates::RenderTarget;
			textureDesc.keepInitialState = true;
			textureDesc.clearValue = nvrhi::Color(0.f);
			textureDesc.useClearValue = true;
			textureDesc.isUAV = true;
			textureDesc.debugName = "FXAA Resolve Buffer";
			textureDesc.sampleCount = 1;

			m_ColorBufferTexture = device->createTexture(textureDesc);
			if (!m_ColorBufferTexture)
			{
				logger::error("Failed to create FXAA Resolved Buffer");
			}

			m_targetDim = glm::vec2(width, height);
		}

	
		//Create depth attachment
		if (!m_DepthBuffer)
		{
			nvrhi::TextureDesc textureDesc;
			textureDesc.format = nvrhi::Format::D32;
			textureDesc.isRenderTarget = true;
			textureDesc.initialState = nvrhi::ResourceStates::DepthWrite;
			textureDesc.keepInitialState = true;
			textureDesc.clearValue = nvrhi::Color(0.f);
			textureDesc.useClearValue = true;
			textureDesc.debugName = "DepthBuffer";
			textureDesc.width = width;
			textureDesc.height = height;
			textureDesc.isShaderResource = true;
			textureDesc.sampleCount = 1;
			textureDesc.dimension = nvrhi::TextureDimension::Texture2D;

			m_DepthBuffer = device->createTexture(textureDesc);
			if (!m_DepthBuffer)
			{
				logger::error("Failed to create DepthTexture");
			}
		}



		//Init Framebuffer
		if (!m_framebuffer)
		{
			nvrhi::FramebufferDesc fbDesc;
			fbDesc.addColorAttachment(m_ColorBufferTexture);
			fbDesc.setDepthAttachment(m_DepthBuffer);
			m_framebuffer = device->createFramebuffer(fbDesc);
			if (!m_framebuffer)
			{
				logger::error("Failed to create FXAA framebuffer");
			}
		}

	}

	void Render(DeviceManager* deviceManager, nvrhi::CommandListHandle& commandList, nvrhi::TextureHandle SimpleForwardColor)
	{
		//Create Pipeline
		if (!m_Pipeline)
		{
			nvrhi::GraphicsPipelineDesc psoDesc;
			psoDesc.VS = m_VertexShader;
			psoDesc.PS = m_PixelShader;
			psoDesc.bindingLayouts = { m_bindingLayout };
			psoDesc.primType = nvrhi::PrimitiveType::TriangleStrip;
			psoDesc.renderState.rasterState.setCullNone();
			psoDesc.renderState.depthStencilState.depthTestEnable = false;
			psoDesc.renderState.depthStencilState.stencilEnable = false;

			m_Pipeline = deviceManager->GetDevice()->createGraphicsPipeline(psoDesc, m_framebuffer);
		}

		BlitConstants blitConstants;

		blitConstants.sourceOrigin = glm::vec2(0, 0);
		blitConstants.sourceSize = m_sourceDim;
		blitConstants.targetOrigin = glm::vec2(0, 0);
		blitConstants.targetSize = m_targetDim;

		FXAAConstantsAligned fxaaConstants;

		fxaaConstants.base = blitConstants;

		const nvrhi::TextureDesc& sfcDesc = SimpleForwardColor->getDesc();

		fxaaConstants.inverseScreenSize = glm::vec2(1.0f / m_targetDim.x, 1.0f / m_targetDim.y);

		commandList->writeBuffer(m_ConstantBuffer, &fxaaConstants, sizeof(fxaaConstants));


		nvrhi::BindingSetDesc bindingSetDesc;
		bindingSetDesc.bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer(0, m_ConstantBuffer, nvrhi::BufferRange(0, sizeof(fxaaConstants))),
			nvrhi::BindingSetItem::Texture_SRV(0, SimpleForwardColor),
			nvrhi::BindingSetItem::Sampler(0, m_sampler)
		};


		if (!nvrhi::utils::CreateBindingSetAndLayout(deviceManager->GetDevice(), nvrhi::ShaderType::All, 0, bindingSetDesc, m_bindingLayout, m_bindingSet))
		{
			MessageBoxA(nullptr, "Couldn't create the binding set or layout", "Error", MB_OK | MB_ICONERROR);
		}

		nvrhi::Viewport targetViewport = m_framebuffer->getFramebufferInfo().getViewport();

		//set state and dispatch
		auto graphicsState = nvrhi::GraphicsState()
			.setPipeline(m_Pipeline)
			.setFramebuffer(m_framebuffer)
			.addBindingSet(m_bindingSet);
		graphicsState.viewport.addScissorRect(nvrhi::Rect(targetViewport));
		graphicsState.viewport.addViewport(targetViewport);

		commandList->setGraphicsState(graphicsState);
		commandList->beginMarker("FXAA Pass");

		nvrhi::DrawArguments args;
		args.instanceCount = 1;
		args.vertexCount = 4;
		commandList->draw(args);

		commandList->endMarker();
	}

	void ClearResources()
	{
		if (m_ColorBufferTexture)
			m_ColorBufferTexture = nullptr;

		if (m_framebuffer)
			m_framebuffer = nullptr;

		if (m_Pipeline)
			m_Pipeline = nullptr;

		if (m_DepthBuffer)
			m_DepthBuffer = nullptr;
	}

	void CompileShaders(DeviceManager* deviceManager, std::shared_ptr<vfs::RootFileSystem>& fs, nvrhi::CommandListHandle& commandList)
	{
		CompileShaderFileNVRHI("shaders/common/rect_vs.hlsl", "main", nvrhi::ShaderType::Vertex, nullptr, deviceManager->GetDevice(), fs, m_VertexShader);
		CompileShaderFileNVRHI("shaders/FXAA/fxaa_ps.hlsl", "FXAA_PS", nvrhi::ShaderType::Pixel, nullptr, deviceManager->GetDevice(), fs, m_PixelShader);
		m_Pipeline = nullptr; //force pipeline rebuild
	}


public:
	nvrhi::FramebufferHandle				m_framebuffer;
	nvrhi::TextureHandle					m_ColorBufferTexture;

private:
	std::shared_ptr<CommonRenderPasses> m_CommonRenderPasses;
	nvrhi::BindingLayoutHandle				m_bindingLayout;
	nvrhi::BindingSetHandle					m_bindingSet;

	nvrhi::ShaderHandle						m_VertexShader;         //Access vertex shader  bytecode
	nvrhi::ShaderHandle						m_PixelShader;          //Access pixel  shader  bytecode

	nvrhi::SamplerHandle					m_sampler;

	glm::vec2 m_sourceDim;
	glm::vec2 m_targetDim;

	nvrhi::TextureHandle					m_DepthBuffer;

	nvrhi::BufferHandle						m_ConstantBuffer;

	nvrhi::GraphicsPipelineHandle			m_Pipeline;
};
