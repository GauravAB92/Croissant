#include <engine/View.h>
#include <view_cb.h>


using namespace Croissant;

void Croissant::IView::FillViewConstants(ViewConstants& constants) const
{
    constants.matWorldToView = (GetViewMatrix());
    constants.matViewToClip = GetProjectionMatrix(true);
    constants.matWorldToClip = GetViewProjectionMatrix(true);
    constants.matClipToView = GetInverseProjectionMatrix(true);
    constants.matViewToWorld = (GetInverseViewMatrix());
    constants.matClipToWorld = GetInverseViewProjectionMatrix(true);
    constants.matViewToClipNoOffset = GetProjectionMatrix(false);
    constants.matWorldToClipNoOffset = GetViewProjectionMatrix(false);
    constants.matClipToViewNoOffset = GetInverseProjectionMatrix(false);
    constants.matClipToWorldNoOffset = GetInverseViewProjectionMatrix(false);

    nvrhi::ViewportState viewportState = GetViewportState();
    const nvrhi::Viewport& viewport = viewportState.viewports[0];
    constants.viewportOrigin = float2(viewport.minX, viewport.minY);
    constants.viewportSize = float2(viewport.width(), viewport.height());
    constants.viewportSizeInv = 1.f / constants.viewportSize;

    constants.clipToWindowScale = float2(0.5f * viewport.width(), -0.5f * viewport.height());
    constants.clipToWindowBias = constants.viewportOrigin + constants.viewportSize * 0.5f;

    constants.windowToClipScale = 1.f / constants.clipToWindowScale;
    constants.windowToClipBias = -constants.clipToWindowBias * constants.windowToClipScale;

    constants.cameraDirectionOrPosition = IsOrthographicProjection()
        ? float4(GetViewDirection(), 0.f)
        : float4(GetViewOrigin(), 1.f);

    constants.pixelOffset = GetPixelOffset();
}

void Croissant::View::EnsureCacheIsValid() const
{
    if (!m_CacheValid)
		const_cast<View*>(this)->UpdateCache(); //make this pointer mutable to allow cache update in const method
}

void Croissant::View::SetViewport(const nvrhi::Viewport& viewport)
{
    m_Viewport = viewport;
    m_ScissorRect = nvrhi::Rect(viewport);
    m_CacheValid = false;
}

void Croissant::View::SetVariableRateShadingState(const nvrhi::VariableRateShadingState& shadingRateState)
{
    //TODO:
}


//set primary matrices and invalidate the cache - the rest of the derived matrices will be computed on demand
void Croissant::View::SetMatrices(const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
{
    m_ViewMatrix = viewMatrix;
    m_ProjMatrix = projMatrix;
    m_CacheValid = false;
}

void Croissant::View::SetPixelOffset(glm::vec2 offset)
{
    m_PixelOffset = offset;
	m_CacheValid = false;
}

void Croissant::View::SetArraySlice(int arraySlice)
{
	m_ArraySlice = arraySlice;
}

//update derived matrices and other information based on the primary parameters - this is called on demand when any of the getters is called and the cache is invalid
void Croissant::View::UpdateCache()
{
    if (m_CacheValid)
        return;

    m_PixelOffsetMatrix = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(
            2.0f * m_PixelOffset.x / (m_Viewport.maxX - m_Viewport.minX),
            -2.0f * m_PixelOffset.y / (m_Viewport.maxY - m_Viewport.minY),
            0.0f));

    m_PixelOffsetMatrixInv = inverse(m_PixelOffsetMatrix);

    m_ViewProjMatrix = (m_ViewMatrix) * m_ProjMatrix;
    m_ViewProjOffsetMatrix = m_ViewProjMatrix * m_PixelOffsetMatrix;

    m_ViewMatrixInv = inverse(m_ViewMatrix);
    m_ProjMatrixInv = inverse(m_ProjMatrix);
    m_ViewProjMatrixInv = m_ProjMatrixInv * m_ViewMatrixInv;
    m_ViewProjOffsetMatrixInv = m_PixelOffsetMatrixInv * m_ViewProjMatrixInv;

    m_ReverseDepth = (m_ProjMatrix[2][2] == 0.f);
   // m_ViewFrustum = frustum(m_ViewProjMatrix, m_ReverseDepth);
   // m_ProjectionFrustum = frustum(m_ProjMatrix, m_ReverseDepth);
   // m_IsMirrored = determinant(m_ViewMatrix.m_linear) < 0.f;

    m_CacheValid = true;

}

nvrhi::ViewportState Croissant::View::GetViewportState() const
{
	return nvrhi::ViewportState().addViewport(m_Viewport).addScissorRect(m_ScissorRect);
}

nvrhi::VariableRateShadingState Croissant::View::GetVariableRateShadingState() const
{
    return m_ShadingRateState;
}

nvrhi::TextureSubresourceSet Croissant::View::GetSubresources() const
{
    return nvrhi::TextureSubresourceSet();
}

bool Croissant::View::IsReverseDepth() const
{
	EnsureCacheIsValid();
	return m_ReverseDepth;
}

bool Croissant::View::IsOrthographicProjection() const
{
    return m_ProjMatrix[2][3] == 0.f;
}

bool Croissant::View::IsStereoView() const
{
    return false;
}

bool Croissant::View::IsCubemapView() const
{
    return false;
}

bool Croissant::View::IsMirrored() const
{
    return false;
}

glm::vec3 Croissant::View::GetViewOrigin() const
{
    EnsureCacheIsValid();
    return glm::vec3(m_ViewMatrixInv[3]);
}

glm::vec3 Croissant::View::GetViewDirection() const
{

    EnsureCacheIsValid();
    
	return -glm::vec3(m_ViewMatrix[2]); //TODO: validate
}

glm::mat4 Croissant::View::GetViewMatrix() const
{
	return m_ViewMatrix;
}

glm::mat4 Croissant::View::GetInverseViewMatrix() const
{
    EnsureCacheIsValid();
	return m_ViewMatrixInv;
}

glm::mat4 Croissant::View::GetProjectionMatrix(bool includeOffset) const
{
    EnsureCacheIsValid();
	return includeOffset ? m_ProjMatrix * m_PixelOffsetMatrix : m_ProjMatrix;
}

glm::mat4 Croissant::View::GetInverseProjectionMatrix(bool includeOffset) const
{
    EnsureCacheIsValid();
	return includeOffset ? m_PixelOffsetMatrixInv * m_ProjMatrixInv : m_ProjMatrixInv;
}

glm::mat4 Croissant::View::GetViewProjectionMatrix(bool includeOffset) const
{  
    EnsureCacheIsValid();
	return includeOffset ? m_ViewProjOffsetMatrix : m_ViewProjMatrix;
}

glm::mat4 Croissant::View::GetInverseViewProjectionMatrix(bool includeOffset) const
{   
    EnsureCacheIsValid();
	return includeOffset ? m_ViewProjOffsetMatrixInv : m_ViewProjMatrixInv;
}

nvrhi::Rect Croissant::View::GetViewExtent() const
{
	return m_ScissorRect;
}

glm::vec2 Croissant::View::GetPixelOffset() const
{
	return m_PixelOffset;
}



