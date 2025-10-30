#include "TemporalAntiAliasing.h"
#include <algorithm>
#include <random>

TemporalAntiAliasingPass::TemporalAntiAliasingPass(nvrhi::IDevice* device, std::shared_ptr<CommonRenderPasses> commonPasses, const CreateParameters& params, std::shared_ptr<RootFileSystem> fs) :
	m_CommonRenderPasses(commonPasses),
	m_FrameIndex(0),
	m_StencilMask(params.motionVectorStencilMask),
	m_HasHistoryClampRelaxTexture(params.historyClampRelax != nullptr),
	m_R2Jitter(vec2(0.0f, 0.0f)),
	m_Jitter(TemporalAntiAliasingJitter::MSAA),
	m_RootFileSystem(fs)
{
	// Initialize shaders, pipelines, and other resources here
	const nvrhi::TextureDesc& unresolvedColorDesc = params.unresolvedColor->getDesc();
	const nvrhi::TextureDesc& resolvedColorDesc = params.resolvedColor->getDesc();
	const nvrhi::TextureDesc& feedback1Desc = params.feedback1->getDesc();
	const nvrhi::TextureDesc& feedback2Desc = params.feedback2->getDesc();

	assert(feedback1Desc.width == feedback2Desc.width);
	assert(feedback1Desc.height == feedback2Desc.height);
	assert(feedback1Desc.format == feedback2Desc.format);
	assert(feedback1Desc.isUAV);
	assert(feedback2Desc.isUAV);
	assert(resolvedColorDesc.isUAV);

	bool useStencil = false;
	nvrhi::Format stencilFormat = nvrhi::Format::UNKNOWN;
	if (params.motionVectorStencilMask)
	{
		useStencil = true;

		nvrhi::Format depthFormat = params.sourceDepth->getDesc().format;

		if (depthFormat == nvrhi::Format::D24S8)
			stencilFormat = nvrhi::Format::X24G8_UINT;
		else if (depthFormat == nvrhi::Format::D32S8)
			stencilFormat = nvrhi::Format::X32G8_UINT;
		else
			assert(!"the format of sourceDepth texture doesn't have a stencil plane");
	}

	//Compile motion vector shader
	CompileShaderFileNVRHI("shaders/TAA/motion_vector_generator_cs.hlsl", "main_cs", nvrhi::ShaderType::Compute, nullptr, device, fs, m_MotionVectorPS);

	//Compile taa resolve compute shader
	CompileShaderFileNVRHI("shaders/TAA/taa_resolve_cs.hlsl", "main_cs", nvrhi::ShaderType::Compute, nullptr, device, fs, m_TemporalAntiAliasingCS);

	//Bilinear sampler for inverse reprojection
	nvrhi::SamplerDesc samplerDesc;
	samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW = nvrhi::SamplerAddressMode::Border;
	samplerDesc.borderColor = nvrhi::Color(0.0f);
	m_BilinearSampler = device->createSampler(samplerDesc);

	m_ResolvedColorSize = vec2(float(resolvedColorDesc.width), float(resolvedColorDesc.height));			//Dimensions of the resolved color texture


	nvrhi::BufferDesc constantBufferDesc;
	constantBufferDesc.byteSize = sizeof(TemporalAntiAliasingConstants);
	constantBufferDesc.debugName = "TemporalAntiAliasingConstants";
	constantBufferDesc.isConstantBuffer = true;
	constantBufferDesc.isVolatile = true;
	constantBufferDesc.maxVersions = params.numConstantBufferVersions;
	m_TemporalAntiAliasingCB = device->createBuffer(constantBufferDesc);

	{
		nvrhi::BindingSetDesc bindingSetDesc;
		bindingSetDesc.bindings = 
		{
			nvrhi::BindingSetItem::ConstantBuffer(0, m_TemporalAntiAliasingCB),
			nvrhi::BindingSetItem::Sampler(0, m_BilinearSampler),
			nvrhi::BindingSetItem::Texture_SRV(0, params.unresolvedColor),
			nvrhi::BindingSetItem::Texture_SRV(1, params.motionVectors),
			nvrhi::BindingSetItem::Texture_SRV(2, params.feedback1),
			nvrhi::BindingSetItem::Texture_UAV(0, params.resolvedColor),
			nvrhi::BindingSetItem::Texture_UAV(1, params.feedback2)
		};

		m_HasHistoryClampRelaxTexture = params.historyClampRelax != nullptr;
		if (params.historyClampRelax != nullptr)
		{
			bindingSetDesc.bindings.push_back(nvrhi::BindingSetItem::Texture_SRV(3, params.historyClampRelax));
		}
		else
		{
			// No relax mask, but we need to bind something to match the shader binding slots
			bindingSetDesc.bindings.push_back(nvrhi::BindingSetItem::Texture_SRV(3, params.unresolvedColor));
		}

		nvrhi::utils::CreateBindingSetAndLayout(device, nvrhi::ShaderType::Compute, 0, bindingSetDesc, m_ResolveBindingLayout, m_ResolveBindingSet);

		// Swap resolvedColor and resolvedColorPrevious (t2 and u0)
		bindingSetDesc.bindings[4].resourceHandle = params.feedback2;
		bindingSetDesc.bindings[6].resourceHandle = params.feedback1;
		m_PrevResolveBindingSet = device->createBindingSet(bindingSetDesc, m_ResolveBindingLayout);

		nvrhi::ComputePipelineDesc pipelineDesc;
		pipelineDesc.CS = m_TemporalAntiAliasingCS;
		pipelineDesc.bindingLayouts = { m_ResolveBindingLayout };
		m_ResolvePSO = device->createComputePipeline(pipelineDesc);
	}
}

void TemporalAntiAliasingPass::RenderMotionVectors(nvrhi::ICommandList* commandList, vec3 preViewTranslationDifference)
{
}

/// <summary>
/// Execute compute shader to resolve the color texture using temporal anti-aliasing.
/// </summary>
/// <param name="commandList"></param>
/// <param name="params"></param>
/// <param name="feedbackIsValid"></param>
void TemporalAntiAliasingPass::Resolve(nvrhi::ICommandList* commandList, const TemporalAntiAliasingParameters& params, nvrhi::Viewport intputViewport, nvrhi::Viewport outputViewport, glm::vec2 pixelOffset, bool feedbackIsValid)
{
	commandList->beginMarker("TemporalAA");

	const nvrhi::Viewport viewportInput  = intputViewport;
	const nvrhi::Viewport viewportOutput = outputViewport;

	TemporalAntiAliasingConstants taaConstants	= {};
	taaConstants.inputViewOrigin				= float2(viewportInput.minX, viewportInput.minY);
	taaConstants.inputViewSize					= float2(viewportInput.width(), viewportInput.height());
	taaConstants.outputViewOrigin				= float2(viewportOutput.minX, viewportOutput.minY);
	taaConstants.outputViewSize					= float2(viewportOutput.width(), viewportOutput.height());
	taaConstants.inputPixelOffset				= pixelOffset;
	taaConstants.outputTextureSizeInv			= glm::vec2(1.0f,1.0f) / m_ResolvedColorSize;
	taaConstants.inputOverOutputViewSize		= taaConstants.inputViewSize / taaConstants.outputViewSize;
	taaConstants.outputOverInputViewSize		= taaConstants.outputViewSize / taaConstants.inputViewSize;
	taaConstants.clampingFactor					= params.enableHistoryClamping ? params.clampingFactor : -1.f;
	taaConstants.newFrameWeight					= feedbackIsValid ? 1.0f / m_FrameIndex : 1.f;
	taaConstants.pqC							= clamp(params.maxRadiance, 1e-4f, 1e8f);
	taaConstants.invPqC							= 1.f / taaConstants.pqC;
	taaConstants.useHistoryClampRelax			= (params.useHistoryClampRelax && m_HasHistoryClampRelaxTexture) ? 1 : 0;
	commandList->writeBuffer(m_TemporalAntiAliasingCB, &taaConstants, sizeof(taaConstants));

	int2 viewportSize	= int2(taaConstants.outputViewSize);
	int2 gridSize		= (viewportSize + 15) / 16;

	nvrhi::ComputeState state;
	state.pipeline = m_ResolvePSO;
	state.bindings = { m_ResolveBindingSet };
	commandList->setComputeState(state);
	commandList->dispatch(gridSize.x, gridSize.y, 1);
	commandList->endMarker();
}

void TemporalAntiAliasingPass::AdvanceFrame()
{
	m_FrameIndex++;

	std::swap(m_ResolveBindingSet, m_PrevResolveBindingSet);
	if (m_Jitter == TemporalAntiAliasingJitter::R2)
	{
		// Advance R2 jitter sequence
		// http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/

		static const float g  = 1.32471795724474602596f;
		static const float a1 = 1.0f / g;
		static const float a2 = 1.0f / (g * g);
		m_R2Jitter[0] = fmodf(m_R2Jitter[0] + a1, 1.0f);
		m_R2Jitter[1] = fmodf(m_R2Jitter[1] + a2, 1.0f);
	}
}

void TemporalAntiAliasingPass::SetJitter(TemporalAntiAliasingJitter jitter)
{
	m_Jitter = jitter;
}

static float VanDerCorput(size_t base, size_t index)
{
	float ret = 0.0f;
	float denominator = float(base);
	while (index > 0)
	{
		size_t multiplier = index % base;
		ret += float(multiplier) / denominator;
		index = index / base;
		denominator *= base;
	}
	return ret;
}

vec2 TemporalAntiAliasingPass::GetCurrentPixelOffset()
{
	switch (m_Jitter)
	{
	default:
	case TemporalAntiAliasingJitter::MSAA:
	{
		// This is the standard 8x MSAA sample pattern, source can be found e.g. here:
		// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_standard_multisample_quality_levels
		const float2 offsets[] = {
			float2(0.0625f, -0.1875f), float2(-0.0625f, 0.1875f), float2(0.3125f, 0.0625f), float2(-0.1875f, -0.3125f),
			float2(-0.3125f, 0.3125f), float2(-0.4375f, -0.0625f), float2(0.1875f, 0.4375f), float2(0.4375f, -0.4375f)
		};

		return offsets[m_FrameIndex % 8];
	}
	case TemporalAntiAliasingJitter::Halton:
	{
		uint32_t index = (m_FrameIndex % 16);
		return float2{ VanDerCorput(2, index), VanDerCorput(3, index) } - 0.5f;
	}
	case TemporalAntiAliasingJitter::R2:
	{
		return m_R2Jitter - 0.5f;
	}
	case TemporalAntiAliasingJitter::Whitenoise:
	{
		std::mt19937 rng(m_FrameIndex);
		std::uniform_real_distribution<float> dist(-0.5f, 0.5f);
		return float2{ dist(rng), dist(rng) };
	}
	}
}

void TemporalAntiAliasingPass::RebuildShaders()
{
	
}