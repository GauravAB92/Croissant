#pragma once

#include <render/backend/dx12/DXHelper.h>
#include <render/backend/DeviceManager.h>
#include <render/renderpasses/CommonRenderPasses.h>
#include <nvrhi/nvrhi.h>
#include <nvrhi/utils.h>

#include "AreaTex.h"
#include "SearchTex.h"


struct SMAAConstantsAligned
{
	glm::vec4 subsampleIndices;
	glm::vec4 rtMetrics;
	float      padding[56];
};

class SMAAPass
{
public:
	SMAAPass(DeviceManager* deviceManager, std::shared_ptr<vfs::RootFileSystem>& fs, nvrhi::CommandListHandle& commandList, std::shared_ptr<CommonRenderPasses> m_CommonRenderPasses) : m_CommonRenderPasses(m_CommonRenderPasses)
	{
		Init(deviceManager, fs, commandList);
	}

	void Init(DeviceManager* deviceManager, std::shared_ptr<vfs::RootFileSystem>& fs, nvrhi::CommandListHandle& commandList)
	{

		//Create Constant Buffer
		auto constantBufferDesc = nvrhi::BufferDesc()
			.setByteSize(sizeof(SMAAConstantsAligned))
			.setIsConstantBuffer(true);

		m_ConstantBuffer = deviceManager->GetDevice()->createBuffer(constantBufferDesc);

		//Compile Shaders
		CompileShaders(deviceManager, fs, commandList);

		//Create Layouts
		//Shared Layout
		{
			nvrhi::BindingLayoutDesc bindingLayoutDesc;
			bindingLayoutDesc.visibility = nvrhi::ShaderType::AllGraphics;
			bindingLayoutDesc.bindings =
			{
				nvrhi::BindingLayoutItem::ConstantBuffer(0),
				nvrhi::BindingLayoutItem::Sampler(0),
				nvrhi::BindingLayoutItem::Sampler(1),
			};

			m_BindingLayoutShared = deviceManager->GetDevice()->createBindingLayout(bindingLayoutDesc);
		}

		//Edge Detection Layout
		{
			nvrhi::BindingLayoutDesc bindingLayoutDesc;
			bindingLayoutDesc.visibility = nvrhi::ShaderType::AllGraphics;
			bindingLayoutDesc.bindings =
			{
				nvrhi::BindingLayoutItem::Texture_SRV(0), // colorTexGamma
			};

			m_BindingLayoutEdgeDetection = deviceManager->GetDevice()->createBindingLayout(bindingLayoutDesc);
		}

		//Weights Calculation Layout
		{
			nvrhi::BindingLayoutDesc bindingLayoutDesc;
			bindingLayoutDesc.visibility = nvrhi::ShaderType::AllGraphics;
			bindingLayoutDesc.bindings =
			{
				nvrhi::BindingLayoutItem::Texture_SRV(0), // edgesTex
				nvrhi::BindingLayoutItem::Texture_SRV(1), // areaTex
				nvrhi::BindingLayoutItem::Texture_SRV(2), // searchTex
			};

			m_BindingLayoutWeightsCalc = deviceManager->GetDevice()->createBindingLayout(bindingLayoutDesc);
		}

		//Neighbor Blend Layout
		{
			nvrhi::BindingLayoutDesc bindingLayoutDesc;
			bindingLayoutDesc.visibility = nvrhi::ShaderType::AllGraphics;
			bindingLayoutDesc.bindings =
			{
				nvrhi::BindingLayoutItem::Texture_SRV(0), // colorTex
				nvrhi::BindingLayoutItem::Texture_SRV(1), // blendTex
				nvrhi::BindingLayoutItem::Texture_SRV(2), // motionVectorsTex
			};

			m_BindingLayoutNeighborBlend = deviceManager->GetDevice()->createBindingLayout(bindingLayoutDesc);
		}

		//Temporal Resolve Layout
		{
			nvrhi::BindingLayoutDesc bindingLayoutDesc;
			bindingLayoutDesc.visibility = nvrhi::ShaderType::AllGraphics;
			bindingLayoutDesc.bindings =
			{
				nvrhi::BindingLayoutItem::Texture_SRV(0), // currentFrameTex
				nvrhi::BindingLayoutItem::Texture_SRV(1), // previousFrameTex
				nvrhi::BindingLayoutItem::Texture_SRV(2), // motionVectorsTex
			};

			m_BindingLayoutTemporalResolve = deviceManager->GetDevice()->createBindingLayout(bindingLayoutDesc);
		}

		//Load precomputed textures
		//Area Texture
		{
			nvrhi::TextureDesc textureDesc;
			textureDesc.format = nvrhi::Format::RG8_UNORM;
			textureDesc.isRenderTarget = false;
			textureDesc.width = AREATEX_WIDTH;
			textureDesc.height = AREATEX_HEIGHT;
			textureDesc.dimension = nvrhi::TextureDimension::Texture2D;
			textureDesc.initialState = nvrhi::ResourceStates::ShaderResource;
			textureDesc.keepInitialState = true;
			textureDesc.debugName = "Area Texture";
			textureDesc.sampleCount = 1;

			m_AreaTexture = deviceManager->GetDevice()->createTexture(textureDesc);
			if (!m_AreaTexture)
			{
				logger::error("Failed to create AreaTexture");
			}

			commandList->open();
			commandList->writeTexture(m_AreaTexture, 0, 0, areaTexBytes, AREATEX_PITCH);
			commandList->close();
			deviceManager->GetDevice()->executeCommandList(commandList);
		}

		//Search Texture
		{
			nvrhi::TextureDesc textureDesc;
			textureDesc.format = nvrhi::Format::R8_UNORM;
			textureDesc.isRenderTarget = false;
			textureDesc.width = SEARCHTEX_WIDTH;
			textureDesc.height = SEARCHTEX_HEIGHT;
			textureDesc.dimension = nvrhi::TextureDimension::Texture2D;
			textureDesc.initialState = nvrhi::ResourceStates::ShaderResource;
			textureDesc.keepInitialState = true;
			textureDesc.debugName = "Search Texture";
			textureDesc.sampleCount = 1;

			m_SearchTexture = deviceManager->GetDevice()->createTexture(textureDesc);
			if (!m_SearchTexture)
			{
				logger::error("Failed to create SearchTexture");
			}

			commandList->open();
			commandList->writeTexture(m_SearchTexture, 0, 0, searchTexBytes, SEARCHTEX_PITCH);
			commandList->close();
			deviceManager->GetDevice()->executeCommandList(commandList);
		}
	}

	//Recreates framebuffer and textures. Called on init and on resize
	void CreateResources(nvrhi::IDevice* device, nvrhi::IFramebuffer* framebuffer, float viewportScale = 1.0f)
	{
		ClearResources();
		const nvrhi::FramebufferInfoEx& fbInfo = framebuffer->getFramebufferInfo();

		uint32_t width = static_cast<uint32_t>(fbInfo.width * viewportScale);
		uint32_t height = static_cast<uint32_t>(fbInfo.height * viewportScale);

		m_ColorBufferTexture = nullptr;
		m_DepthBuffer = nullptr;

		m_FBEdgeDetection = nullptr;
		m_FBWeightsCalc = nullptr;
		m_FBNeighborBlend = nullptr;

		m_WeightsBuffer = nullptr;
		m_EdgesBuffer = nullptr;

		m_PipelineEdgeDetection = nullptr;
		m_PipelineWeightsCalc = nullptr;
		m_PipelineNeighborBlend = nullptr;

		// Create unfiltered render target texture
		if (!m_ColorBufferTexture)
		{
			nvrhi::TextureDesc textureDesc;
			textureDesc.format = nvrhi::Format::RGBA8_UNORM;
			textureDesc.isRenderTarget = true;
			textureDesc.width = width;
			textureDesc.height = height;
			textureDesc.dimension = nvrhi::TextureDimension::Texture2D;
			textureDesc.initialState = nvrhi::ResourceStates::RenderTarget;
			textureDesc.keepInitialState = true;
			textureDesc.clearValue = nvrhi::Color(0.f);
			textureDesc.useClearValue = true;
			textureDesc.isUAV = true;
			textureDesc.debugName = "SMAA Resolve Buffer";
			textureDesc.sampleCount = 1;

			m_ColorBufferTexture = device->createTexture(textureDesc);
			if (!m_ColorBufferTexture)
			{
				logger::error("Failed to create SMAA Resolved Buffer");
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

		//Create SMAA specific buffers
		if (!m_EdgesBuffer)
		{
			nvrhi::TextureDesc textureDesc;
			textureDesc.format = nvrhi::Format::RGBA8_UNORM;
			textureDesc.isRenderTarget = true;
			textureDesc.width = width;
			textureDesc.height = height;
			textureDesc.dimension = nvrhi::TextureDimension::Texture2D;
			textureDesc.initialState = nvrhi::ResourceStates::RenderTarget;
			textureDesc.keepInitialState = true;
			textureDesc.clearValue = nvrhi::Color(1.0f);
			textureDesc.useClearValue = true;
			textureDesc.isShaderResource = true;
			textureDesc.debugName = "SMAA Edges Buffer";
			textureDesc.sampleCount = 1;

			m_EdgesBuffer = device->createTexture(textureDesc);
			if (!m_EdgesBuffer)
			{
				logger::error("Failed to create SMAA Edge Detection Buffer");
			}
		}

		if (!m_WeightsBuffer)
		{
			nvrhi::TextureDesc textureDesc;
			textureDesc.format = nvrhi::Format::RGBA8_UNORM;
			textureDesc.isRenderTarget = true;
			textureDesc.width = width;
			textureDesc.height = height;
			textureDesc.dimension = nvrhi::TextureDimension::Texture2D;
			textureDesc.initialState = nvrhi::ResourceStates::RenderTarget;
			textureDesc.keepInitialState = true;
			textureDesc.clearValue = nvrhi::Color(0.f);
			textureDesc.useClearValue = true;
			textureDesc.isShaderResource = true;
			textureDesc.debugName = "SMAA Blend Weights Buffer";
			textureDesc.sampleCount = 1;

			m_WeightsBuffer = device->createTexture(textureDesc);
			if (!m_WeightsBuffer)
			{
				logger::error("Failed to create SMAA Blend Weights Buffer");
			}
		}

		if (!m_HistoryBuffer[0])
		{
			nvrhi::TextureDesc textureDesc;
			textureDesc.format = nvrhi::Format::RGBA8_UNORM;
			textureDesc.isRenderTarget = true;
			textureDesc.width = width;
			textureDesc.height = height;
			textureDesc.dimension = nvrhi::TextureDimension::Texture2D;
			textureDesc.initialState = nvrhi::ResourceStates::Common;
			textureDesc.keepInitialState = true;
			textureDesc.clearValue = nvrhi::Color(0.f);
			textureDesc.useClearValue = true;
			textureDesc.isShaderResource = true;
			textureDesc.debugName = "SMAA History Buffer0";
			textureDesc.sampleCount = 1;

			m_HistoryBuffer[0] = device->createTexture(textureDesc);
			if (!m_HistoryBuffer[0])
			{
				logger::error("Failed to create SMAA History Buffer0");
			}
		}

		if (!m_HistoryBuffer[1])
		{
			nvrhi::TextureDesc textureDesc;
			textureDesc.format = nvrhi::Format::RGBA8_UNORM;
			textureDesc.isRenderTarget = true;
			textureDesc.width = width;
			textureDesc.height = height;
			textureDesc.keepInitialState = true;
			textureDesc.dimension = nvrhi::TextureDimension::Texture2D;
			textureDesc.initialState = nvrhi::ResourceStates::Common;
			textureDesc.clearValue = nvrhi::Color(0.f);
			textureDesc.useClearValue = true;
			textureDesc.isShaderResource = true;
			textureDesc.debugName = "SMAA History Buffer1";
			textureDesc.sampleCount = 1;

			m_HistoryBuffer[1] = device->createTexture(textureDesc);
			if (!m_HistoryBuffer[1])
			{
				logger::error("Failed to create SMAA History Buffer1");
			}
		}

		//Init Framebuffers
		if (!m_FBEdgeDetection)
		{
			nvrhi::FramebufferDesc fbDesc;
			fbDesc.addColorAttachment(m_EdgesBuffer);
			//fbDesc.setDepthAttachment(m_DepthBuffer);
			m_FBEdgeDetection = device->createFramebuffer(fbDesc);
			if (!m_FBEdgeDetection)
			{
				logger::error("Failed to create Edge Detection framebuffer");
			}
		}

		if (!m_FBWeightsCalc)
		{
			nvrhi::FramebufferDesc fbDesc;
			fbDesc.addColorAttachment(m_WeightsBuffer);
			fbDesc.setDepthAttachment(m_DepthBuffer);
			m_FBWeightsCalc = device->createFramebuffer(fbDesc);
			if (!m_FBWeightsCalc)
			{
				logger::error("Failed to create Weights Calculation framebuffer");
			}
		}

		if (!m_FBNeighborBlend)
		{
			nvrhi::FramebufferDesc fbDesc;
			fbDesc.addColorAttachment(m_ColorBufferTexture);
			fbDesc.setDepthAttachment(m_DepthBuffer);
			m_FBNeighborBlend = device->createFramebuffer(fbDesc);
			if (!m_FBNeighborBlend)
			{
				logger::error("Failed to create Neighbor Blend framebuffer");
			}
		}

		if (!m_FBTemporalResolve)
		{
			nvrhi::FramebufferDesc fbDesc;
			fbDesc.addColorAttachment(m_HistoryBuffer[0]);
			m_FBTemporalResolve = device->createFramebuffer(fbDesc);

			if (!m_FBTemporalResolve)
			{
				logger::error("Failed to create Resolve 0 framebuffer");
			}
		}

	}

	void AdvanceFrame()
	{
		m_FrameIndex = (m_FrameIndex + 1) % 2;

	}

	glm::vec2 GetCurrentPixelOffset()
	{
		return (m_FrameIndex == 0) ? glm::vec2(-0.25f, 0.25f) : glm::vec2(0.25f, -0.25f);

	}

	void Render(DeviceManager* deviceManager, nvrhi::CommandListHandle& commandList,
		nvrhi::TextureHandle SimpleForwardColor,
		nvrhi::TextureHandle MotionVectorsTexture, bool useTemporalSamples)
	{
		//Create Pipelines
		if (!m_PipelineEdgeDetection)
		{
			nvrhi::GraphicsPipelineDesc psoDesc;
			psoDesc.VS = m_VSEdgeDetection;
			psoDesc.PS = m_PSEdgeDetection;
			psoDesc.bindingLayouts = { m_BindingLayoutShared, m_BindingLayoutEdgeDetection };
			psoDesc.primType = nvrhi::PrimitiveType::TriangleList;
			psoDesc.renderState.rasterState.setCullNone();
			psoDesc.renderState.depthStencilState.depthTestEnable = false;
			psoDesc.renderState.depthStencilState.stencilEnable = false;

			m_PipelineEdgeDetection = deviceManager->GetDevice()->createGraphicsPipeline(psoDesc, m_FBEdgeDetection);
		}


		if (!m_PipelineWeightsCalc)
		{
			nvrhi::GraphicsPipelineDesc psoDesc;
			psoDesc.VS = m_VSWeightsCalc;
			psoDesc.PS = m_PSWeightsCalc;
			psoDesc.bindingLayouts = { m_BindingLayoutShared, m_BindingLayoutWeightsCalc };
			psoDesc.primType = nvrhi::PrimitiveType::TriangleStrip;
			psoDesc.renderState.rasterState.setCullNone();
			psoDesc.renderState.depthStencilState.depthTestEnable = false;
			psoDesc.renderState.depthStencilState.stencilEnable = false;

			m_PipelineWeightsCalc = deviceManager->GetDevice()->createGraphicsPipeline(psoDesc, m_FBWeightsCalc);
		}

		if (!m_PipelineNeighborBlend)
		{
			nvrhi::GraphicsPipelineDesc psoDesc;
			psoDesc.VS = m_VSNeighborBlend;
			psoDesc.PS = m_PSNeighborBlend;
			psoDesc.bindingLayouts = { m_BindingLayoutShared, m_BindingLayoutNeighborBlend };
			psoDesc.primType = nvrhi::PrimitiveType::TriangleStrip;
			psoDesc.renderState.rasterState.setCullNone();
			psoDesc.renderState.depthStencilState.depthTestEnable = false;
			psoDesc.renderState.depthStencilState.stencilEnable = false;

			m_PipelineNeighborBlend = deviceManager->GetDevice()->createGraphicsPipeline(psoDesc, m_FBNeighborBlend);
		}

		if (!m_PipelineTemporalResolve)
		{
			nvrhi::GraphicsPipelineDesc psoDesc;
			psoDesc.VS = m_VSTemporalResolve;
			psoDesc.PS = m_PSTemporalResolve;
			psoDesc.bindingLayouts = { m_BindingLayoutShared, m_BindingLayoutTemporalResolve };
			psoDesc.primType = nvrhi::PrimitiveType::TriangleStrip;
			psoDesc.renderState.rasterState.setCullNone();
			psoDesc.renderState.depthStencilState.depthTestEnable = false;
			psoDesc.renderState.depthStencilState.stencilEnable = false;

			m_PipelineTemporalResolve = deviceManager->GetDevice()->createGraphicsPipeline(psoDesc, m_FBTemporalResolve);
		}

		SMAAConstantsAligned smaaConstants;

		smaaConstants.subsampleIndices = glm::vec4(
			0.0f, 0.0f, // subsample 0
			-0.5f, -0.5f // subsample 1 (for 2x2 pattern)
		);

		smaaConstants.rtMetrics = glm::vec4(
			1.0f / m_targetDim.x, 1.0f / m_targetDim.y,
			m_targetDim.x, m_targetDim.y
		);

		commandList->writeBuffer(m_ConstantBuffer, &smaaConstants, sizeof(smaaConstants));

		//Shared Binding Set
		{
			nvrhi::BindingSetDesc bindingSetDesc;
			bindingSetDesc.bindings =
			{
				nvrhi::BindingSetItem::ConstantBuffer(0, m_ConstantBuffer, nvrhi::BufferRange(0, sizeof(smaaConstants))),
				nvrhi::BindingSetItem::Sampler(0, m_CommonRenderPasses->m_LinearClampSampler),
				nvrhi::BindingSetItem::Sampler(1, m_CommonRenderPasses->m_PointClampSampler),
			};

			if (!nvrhi::utils::CreateBindingSetAndLayout(deviceManager->GetDevice(), nvrhi::ShaderType::All, 0, bindingSetDesc, m_BindingLayoutShared, m_BindingSetShared))
			{
				MessageBoxA(nullptr, "Couldn't create the binding set or layout", "Error", MB_OK | MB_ICONERROR);
			}
		}

		//Edge Detection Pass
		commandList->clearTextureFloat(m_EdgesBuffer, nvrhi::AllSubresources, nvrhi::Color(0.0f));
		{
			nvrhi::BindingSetDesc bindingSetDesc;
			bindingSetDesc.bindings =
			{
				nvrhi::BindingSetItem::Texture_SRV(0, SimpleForwardColor),
			};

			if (!nvrhi::utils::CreateBindingSetAndLayout(deviceManager->GetDevice(), nvrhi::ShaderType::All, 0, bindingSetDesc, m_BindingLayoutEdgeDetection, m_BindingSetEdgeDetection))
			{
				MessageBoxA(nullptr, "Couldn't create the binding set or layout", "Error", MB_OK | MB_ICONERROR);
			}
		}

		nvrhi::Viewport targetViewport = m_FBEdgeDetection->getFramebufferInfo().getViewport();

		//set state and dispatch
		auto graphicsState = nvrhi::GraphicsState()
			.setPipeline(m_PipelineEdgeDetection)
			.setFramebuffer(m_FBEdgeDetection)
			.addBindingSet(m_BindingSetShared)
			.addBindingSet(m_BindingSetEdgeDetection);
		graphicsState.viewport.addScissorRect(nvrhi::Rect(targetViewport));
		graphicsState.viewport.addViewport(targetViewport);

		commandList->setGraphicsState(graphicsState);
		commandList->beginMarker("SMAA Pass");
		nvrhi::DrawArguments args;
		args.instanceCount = 1;
		args.vertexCount = 3;
		commandList->draw(args);
		commandList->endMarker();

		//Weights Pass
		commandList->clearTextureFloat(m_WeightsBuffer, nvrhi::AllSubresources, nvrhi::Color(0.0f));
		{
			nvrhi::BindingSetDesc bindingSetDesc;
			bindingSetDesc.bindings =
			{
				nvrhi::BindingSetItem::Texture_SRV(0, m_EdgesBuffer),
				nvrhi::BindingSetItem::Texture_SRV(1, m_AreaTexture),
				nvrhi::BindingSetItem::Texture_SRV(2, m_SearchTexture),
			};
			if (!nvrhi::utils::CreateBindingSetAndLayout(deviceManager->GetDevice(), nvrhi::ShaderType::All, 0, bindingSetDesc, m_BindingLayoutWeightsCalc, m_BindingSetWeightsCalc))
			{
				MessageBoxA(nullptr, "Couldn't create the binding set or layout", "Error", MB_OK | MB_ICONERROR);
			}
		}

		graphicsState = nvrhi::GraphicsState()
			.setPipeline(m_PipelineWeightsCalc)
			.setFramebuffer(m_FBWeightsCalc)
			.addBindingSet(m_BindingSetShared)
			.addBindingSet(m_BindingSetWeightsCalc);
		graphicsState.viewport.addScissorRect(nvrhi::Rect(targetViewport));
		graphicsState.viewport.addViewport(targetViewport);

		commandList->setGraphicsState(graphicsState);
		commandList->beginMarker("SMAA Weights Calculation Pass");
		args.instanceCount = 1;
		args.vertexCount = 3;
		commandList->draw(args);
		commandList->endMarker();

		//Blending Pass
		{
			nvrhi::BindingSetDesc bindingSetDesc;
			bindingSetDesc.bindings =
			{
				nvrhi::BindingSetItem::Texture_SRV(0, SimpleForwardColor),
				nvrhi::BindingSetItem::Texture_SRV(1, m_WeightsBuffer),
				nvrhi::BindingSetItem::Texture_SRV(2, MotionVectorsTexture),
			};
			if (!nvrhi::utils::CreateBindingSetAndLayout(deviceManager->GetDevice(), nvrhi::ShaderType::All, 0, bindingSetDesc, m_BindingLayoutNeighborBlend, m_BindingSetNeighborBlend))
			{
				MessageBoxA(nullptr, "Couldn't create the binding set or layout", "Error", MB_OK | MB_ICONERROR);
			}
		}

		graphicsState = nvrhi::GraphicsState()
			.setPipeline(m_PipelineNeighborBlend)
			.setFramebuffer(m_FBNeighborBlend)
			.addBindingSet(m_BindingSetShared)
			.addBindingSet(m_BindingSetNeighborBlend);
		graphicsState.viewport.addScissorRect(nvrhi::Rect(targetViewport));
		graphicsState.viewport.addViewport(targetViewport);

		commandList->setGraphicsState(graphicsState);
		commandList->beginMarker("SMAA Neighbor Blending Pass");
		args.instanceCount = 1;
		args.vertexCount = 3;
		commandList->draw(args);
		commandList->endMarker();

		//Temporal Resolve Pass
		if (useTemporalSamples)
		{
			nvrhi::TextureHandle current = m_HistoryBuffer[0];
			nvrhi::TextureHandle previous = m_HistoryBuffer[1];

			//resolve to current history buffer
			{
				nvrhi::BindingSetDesc bindingSetDesc;
				bindingSetDesc.bindings =
				{
					nvrhi::BindingSetItem::Texture_SRV(0, m_ColorBufferTexture),
					nvrhi::BindingSetItem::Texture_SRV(1, previous),
					nvrhi::BindingSetItem::Texture_SRV(2, MotionVectorsTexture),
				};
				if (!nvrhi::utils::CreateBindingSetAndLayout(
					deviceManager->GetDevice(),
					nvrhi::ShaderType::All, 0,
					bindingSetDesc,
					m_BindingLayoutTemporalResolve,
					m_BindingSetTemporalResolve))  // index by frame
				{
					MessageBoxA(nullptr, "Couldn't create temporal resolve binding set", "Error", MB_OK | MB_ICONERROR);
				}
			}

			graphicsState = nvrhi::GraphicsState()
				.setPipeline(m_PipelineTemporalResolve)
				.setFramebuffer(m_FBTemporalResolve)
				.addBindingSet(m_BindingSetShared)
				.addBindingSet(m_BindingSetTemporalResolve);
			graphicsState.viewport.addScissorRect(nvrhi::Rect(targetViewport));
			graphicsState.viewport.addViewport(targetViewport);

			commandList->setGraphicsState(graphicsState);
			commandList->beginMarker("SMAA Neighbor Temporal Resolve Pass");
			args.instanceCount = 1;
			args.vertexCount = 3;
			commandList->draw(args);
			commandList->endMarker();

			commandList->copyTexture(
				m_HistoryBuffer[1],           // dest — stable, always same texture
				nvrhi::TextureSlice(),
				m_HistoryBuffer[0],			// src — just written by temporal resolve
				nvrhi::TextureSlice()
			);
		}

	}

	void ClearResources()
	{
		if (m_ColorBufferTexture)
			m_ColorBufferTexture = nullptr;

		if (m_EdgesBuffer)
			m_EdgesBuffer = nullptr;

		if (m_WeightsBuffer)
			m_WeightsBuffer = nullptr;

		if (m_HistoryBuffer[0])
			m_HistoryBuffer[0] = nullptr;

		if (m_HistoryBuffer[1])
			m_HistoryBuffer[1] = nullptr;

		if (m_FBEdgeDetection)
			m_FBEdgeDetection = nullptr;

		if (m_PipelineEdgeDetection)
			m_PipelineEdgeDetection = nullptr;

		if (m_PipelineWeightsCalc)
			m_PipelineWeightsCalc = nullptr;

		if (m_PipelineNeighborBlend)
			m_PipelineNeighborBlend = nullptr;

		if (m_DepthBuffer)
			m_DepthBuffer = nullptr;

		if (m_FBEdgeDetection)
			m_FBEdgeDetection = nullptr;

		if (m_FBWeightsCalc)
			m_FBWeightsCalc = nullptr;

		if (m_FBNeighborBlend)
			m_FBNeighborBlend = nullptr;

		if (m_FBTemporalResolve)
			m_FBTemporalResolve = nullptr;


	}

	void CompileShaders(DeviceManager* deviceManager, std::shared_ptr<vfs::RootFileSystem>& fs, nvrhi::CommandListHandle& commandList)
	{
		CompileShaderFileNVRHI("shaders/SMAA/smaa_shaders.hlsl", "EdgeDetectionVS", nvrhi::ShaderType::Vertex, nullptr, deviceManager->GetDevice(), fs, m_VSEdgeDetection);
		CompileShaderFileNVRHI("shaders/SMAA/smaa_shaders.hlsl", "EdgeDetectionPS", nvrhi::ShaderType::Pixel, nullptr, deviceManager->GetDevice(), fs, m_PSEdgeDetection);

		CompileShaderFileNVRHI("shaders/SMAA/smaa_shaders.hlsl", "WeightsCalcVS", nvrhi::ShaderType::Vertex, nullptr, deviceManager->GetDevice(), fs, m_VSWeightsCalc);
		CompileShaderFileNVRHI("shaders/SMAA/smaa_shaders.hlsl", "WeightsCalcPS", nvrhi::ShaderType::Pixel, nullptr, deviceManager->GetDevice(), fs, m_PSWeightsCalc);

		CompileShaderFileNVRHI("shaders/SMAA/smaa_shaders.hlsl", "NeighborBlendVS", nvrhi::ShaderType::Vertex, nullptr, deviceManager->GetDevice(), fs, m_VSNeighborBlend);
		CompileShaderFileNVRHI("shaders/SMAA/smaa_shaders.hlsl", "NeighborBlendPS", nvrhi::ShaderType::Pixel, nullptr, deviceManager->GetDevice(), fs, m_PSNeighborBlend);

		CompileShaderFileNVRHI("shaders/SMAA/smaa_shaders.hlsl", "TemporalResolveVS", nvrhi::ShaderType::Vertex, nullptr, deviceManager->GetDevice(), fs, m_VSTemporalResolve);
		CompileShaderFileNVRHI("shaders/SMAA/smaa_shaders.hlsl", "TemporalResolvePS", nvrhi::ShaderType::Pixel, nullptr, deviceManager->GetDevice(), fs, m_PSTemporalResolve);

		//force pipeline rebuild
		m_PipelineEdgeDetection = nullptr;
		m_PipelineWeightsCalc = nullptr;
		m_PipelineNeighborBlend = nullptr;
		m_PipelineTemporalResolve = nullptr;
	}


public:
	nvrhi::FramebufferHandle				m_FBEdgeDetection;
	nvrhi::FramebufferHandle				m_FBWeightsCalc;
	nvrhi::FramebufferHandle				m_FBNeighborBlend;
	nvrhi::FramebufferHandle				m_FBTemporalResolve;

	nvrhi::TextureHandle					m_ColorBufferTexture;
	nvrhi::TextureHandle					m_DepthBuffer;
	nvrhi::TextureHandle 					m_EdgesBuffer;			//Result of Edge Detection pass. RGBA
	nvrhi::TextureHandle					m_WeightsBuffer;		//Calculated blending weights. RGBA

	nvrhi::TextureHandle					m_HistoryBuffer[2];
	int m_FrameIndex = 0;
	bool firstFrame = true;


private:
	std::shared_ptr<CommonRenderPasses> m_CommonRenderPasses;

	nvrhi::BindingLayoutHandle				m_BindingLayoutShared;
	nvrhi::BindingSetHandle					m_BindingSetShared;

	nvrhi::BindingLayoutHandle				m_BindingLayoutEdgeDetection;
	nvrhi::BindingSetHandle					m_BindingSetEdgeDetection;

	nvrhi::BindingLayoutHandle				m_BindingLayoutWeightsCalc;
	nvrhi::BindingSetHandle					m_BindingSetWeightsCalc;

	nvrhi::BindingLayoutHandle				m_BindingLayoutNeighborBlend;
	nvrhi::BindingSetHandle					m_BindingSetNeighborBlend;

	nvrhi::BindingLayoutHandle				m_BindingLayoutTemporalResolve;
	nvrhi::BindingSetHandle					m_BindingSetTemporalResolve;

	nvrhi::ShaderHandle						m_VSEdgeDetection;
	nvrhi::ShaderHandle						m_PSEdgeDetection;

	nvrhi::ShaderHandle						m_VSWeightsCalc;
	nvrhi::ShaderHandle						m_PSWeightsCalc;

	nvrhi::ShaderHandle						m_VSNeighborBlend;
	nvrhi::ShaderHandle						m_PSNeighborBlend;

	nvrhi::ShaderHandle						m_VSTemporalResolve;
	nvrhi::ShaderHandle						m_PSTemporalResolve;

	nvrhi::SamplerHandle					m_LinearClampSampler;
	nvrhi::SamplerHandle					m_PointClampSampler;

	glm::vec2 m_sourceDim;
	glm::vec2 m_targetDim;

	nvrhi::TextureHandle					m_AreaTexture;			//Precomputed area texture. RGBA8
	nvrhi::TextureHandle					m_SearchTexture;		//Precomputed search texture. RG8

	nvrhi::BufferHandle						m_ConstantBuffer;

	nvrhi::GraphicsPipelineHandle 			m_PipelineEdgeDetection;
	nvrhi::GraphicsPipelineHandle			m_PipelineWeightsCalc;
	nvrhi::GraphicsPipelineHandle			m_PipelineNeighborBlend;
	nvrhi::GraphicsPipelineHandle			m_PipelineTemporalResolve;

};

