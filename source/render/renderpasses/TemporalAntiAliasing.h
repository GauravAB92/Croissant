/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

//Modified by Gaurav Bhokare

#include <nvrhi/nvrhi.h>
#include <nvrhi/utils.h>
#include <glm.hpp>
#include <memory>
#include <core/VFS.h>
#include <render/backend/DeviceManager.h>
#include <render/backend/ShaderUtils.h>
#include <render/renderpasses/CommonRenderPasses.h>
#include <core/log.h>
#include <core/VFS.h>

#include "../shaders/common/taa_cb.h"



using namespace glm;
using namespace vfs;

enum class TemporalAntiAliasingJitter
{
	MSAA,
	Halton,
	R2,
	Whitenoise
};

struct TemporalAntiAliasingParameters
{
	float newFrameWeight = 0.1f; // Weight for the new frame in the blend
	float clampingFactor = 1.0f; // Clamping factor to limit the maximum change per frame
	float maxRadiance = 10000.f;
	bool  enableHistoryClamping = false;

	// Requires CreateParameters::historyClampRelax single channel [0, 1] mask to be provided. 
	// For texels with mask value of 0 the behavior is unchanged; for texels with mask value > 0, 
	// 'newFrameWeight' will be reduced and 'clampingFactor' will be increased proportionally. 
	bool  useHistoryClampRelax = false; // Use a relaxed history clamp
};

class TemporalAntiAliasingPass
{
	std::shared_ptr<CommonRenderPasses> m_CommonRenderPasses;
	std::shared_ptr<RootFileSystem> m_RootFileSystem;

	nvrhi::ShaderHandle  m_MotionVectorPS;
	nvrhi::ShaderHandle  m_TemporalAntiAliasingCS;
	nvrhi::SamplerHandle m_BilinearSampler;
	nvrhi::BufferHandle  m_TemporalAntiAliasingCB;

	nvrhi::BindingLayoutHandle		m_MotionVectorBindingLayout;
	nvrhi::BindingSetHandle			m_MotionVectorBindingSet;	
	nvrhi::GraphicsPipelineHandle	m_MotionVectorPSO;

	nvrhi::BindingLayoutHandle	 m_ResolveBindingLayout;
	nvrhi::BindingSetHandle		 m_ResolveBindingSet;
	nvrhi::BindingSetHandle		 m_PrevResolveBindingSet;
	nvrhi::ComputePipelineHandle m_ResolvePSO;

	uint32_t m_FrameIndex;		//num frames accumulated so far
	uint32_t m_StencilMask;     
	vec2     m_ResolvedColorSize;

	vec2						m_R2Jitter;
	TemporalAntiAliasingJitter	m_Jitter;

	bool     m_HasHistoryClampRelaxTexture;

public:

	struct CreateParameters
	{
		nvrhi::ITexture* sourceDepth = nullptr;
		nvrhi::ITexture* motionVectors = nullptr;
		nvrhi::ITexture* unresolvedColor = nullptr;
		nvrhi::ITexture* resolvedColor = nullptr;
		nvrhi::ITexture* feedback1 = nullptr;
		nvrhi::ITexture* feedback2 = nullptr;
		nvrhi::ITexture* historyClampRelax = nullptr;
		bool useCatmullRomFilter = true;
		uint32_t motionVectorStencilMask = 0;
		uint32_t numConstantBufferVersions = 16;
	};


	TemporalAntiAliasingPass(
		nvrhi::IDevice* device,
		std::shared_ptr<CommonRenderPasses> commonPasses,
		const CreateParameters& params,
		std::shared_ptr<RootFileSystem> fs);

	void RenderMotionVectors(
		nvrhi::ICommandList* commandList,
		vec3 preViewTranslationDifference = vec3(0.0f,0.0f,0.0f));

	void Resolve(
		nvrhi::ICommandList* commandList,
		const TemporalAntiAliasingParameters& params
		, nvrhi::Viewport intputViewport, nvrhi::Viewport outputViewport, glm::vec2 pixelOffset,
		bool feedbackIsValid);


	void RebuildShaders();
	void AdvanceFrame();
	void SetJitter(TemporalAntiAliasingJitter jitter);
	TemporalAntiAliasingJitter GetJitter() const { return m_Jitter; }
	vec2 GetCurrentPixelOffset();
};