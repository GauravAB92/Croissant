#pragma once
#include <glm/glm.hpp>
#include <nvrhi/nvrhi.h>
#include <vector>
#include <memory>


struct ViewConstants;

namespace Croissant
{
    class IView 
    {
    public:
        virtual void FillViewConstants(ViewConstants& constants) const;

        [[nodiscard]] virtual nvrhi::ViewportState GetViewportState() const = 0;
        [[nodiscard]] virtual nvrhi::VariableRateShadingState GetVariableRateShadingState() const = 0;
        [[nodiscard]] virtual nvrhi::TextureSubresourceSet GetSubresources() const = 0;
        [[nodiscard]] virtual bool IsReverseDepth() const = 0;
        [[nodiscard]] virtual bool IsOrthographicProjection() const = 0;
        [[nodiscard]] virtual bool IsStereoView() const = 0;
        [[nodiscard]] virtual bool IsCubemapView() const = 0;
       // [[nodiscard]] virtual bool IsBoxVisible(const dm::box3& bbox) const = 0;
        [[nodiscard]] virtual bool IsMirrored() const = 0;
        [[nodiscard]] virtual glm::vec3 GetViewOrigin() const = 0;
        [[nodiscard]] virtual glm::vec3 GetViewDirection() const = 0;
        //[[nodiscard]] virtual dm::frustum GetViewFrustum() const = 0;
       // [[nodiscard]] virtual dm::frustum GetProjectionFrustum() const = 0;
        [[nodiscard]] virtual glm::mat4 GetViewMatrix() const = 0;
        [[nodiscard]] virtual glm::mat4 GetInverseViewMatrix() const = 0;
        [[nodiscard]] virtual glm::mat4 GetProjectionMatrix(bool includeOffset = true) const = 0;
        [[nodiscard]] virtual glm::mat4 GetInverseProjectionMatrix(bool includeOffset = true) const = 0;
        [[nodiscard]] virtual glm::mat4 GetViewProjectionMatrix(bool includeOffset = true) const = 0;
        [[nodiscard]] virtual glm::mat4 GetInverseViewProjectionMatrix(bool includeOffset = true) const = 0;
        [[nodiscard]] virtual nvrhi::Rect GetViewExtent() const = 0;
        [[nodiscard]] virtual glm::vec2 GetPixelOffset() const = 0;
    };

    
    class View : public IView
    {
    protected:
        // Directly settable parameters
        nvrhi::Viewport m_Viewport;
        nvrhi::Rect m_ScissorRect;
        nvrhi::VariableRateShadingState m_ShadingRateState;
        glm::mat4 m_ViewMatrix  = glm::mat4(1.0f);
        glm::mat4 m_ProjMatrix  = glm::mat4(1.0f);
        glm::vec2 m_PixelOffset = glm::vec2(0.0f);
        int m_ArraySlice = 0;

        // Derived matrices and other information - computed and cached on access
        glm::mat4 m_PixelOffsetMatrix = glm::mat4(1.0f);
        glm::mat4 m_PixelOffsetMatrixInv = glm::mat4(1.0f);
        glm::mat4 m_ViewProjMatrix = glm::mat4(1.0f);
        glm::mat4 m_ViewProjOffsetMatrix = glm::mat4(1.0f);
        glm::mat4 m_ViewMatrixInv = glm::mat4(1.0f);
        glm::mat4 m_ProjMatrixInv = glm::mat4(1.0f);
        glm::mat4 m_ViewProjMatrixInv = glm::mat4(1.0f);
        glm::mat4 m_ViewProjOffsetMatrixInv = glm::mat4(1.0f);

        //dm::frustum m_ViewFrustum = dm::frustum::empty();
        //dm::frustum m_ProjectionFrustum = dm::frustum::empty();
        bool m_ReverseDepth = false;
        bool m_IsMirrored = false;
        bool m_CacheValid = false;

        void EnsureCacheIsValid() const;

    public:
        void SetViewport(const nvrhi::Viewport& viewport);
        void SetVariableRateShadingState(const nvrhi::VariableRateShadingState& shadingRateState);
        void SetMatrices(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void SetPixelOffset(glm::vec2 offset);
        void SetArraySlice(int arraySlice);
        void UpdateCache();

        [[nodiscard]] const nvrhi::Viewport& GetViewport() const { return m_Viewport; }
        [[nodiscard]] const nvrhi::Rect& GetScissorRect() const { return m_ScissorRect; }

        [[nodiscard]] nvrhi::ViewportState GetViewportState() const override;
        [[nodiscard]] nvrhi::VariableRateShadingState GetVariableRateShadingState() const override;
        [[nodiscard]] nvrhi::TextureSubresourceSet GetSubresources() const override;
        [[nodiscard]] bool IsReverseDepth() const override;
        [[nodiscard]] bool IsOrthographicProjection() const override;
        [[nodiscard]] bool IsStereoView() const override;
        [[nodiscard]] bool IsCubemapView() const override;
        //[[nodiscard]] bool IsBoxVisible(const dm::box3& bbox) const override;
        [[nodiscard]] bool IsMirrored() const override;
        [[nodiscard]] glm::vec3 GetViewOrigin() const override;
        [[nodiscard]] glm::vec3 GetViewDirection() const override;
       // [[nodiscard]] dm::frustum GetViewFrustum() const override;
       // [[nodiscard]] dm::frustum GetProjectionFrustum() const override;
        [[nodiscard]] glm::mat4 GetViewMatrix() const override;
        [[nodiscard]] glm::mat4 GetInverseViewMatrix() const override;
        [[nodiscard]] glm::mat4 GetProjectionMatrix(bool includeOffset = true) const override;
        [[nodiscard]] glm::mat4 GetInverseProjectionMatrix(bool includeOffset = true) const override;
        [[nodiscard]] glm::mat4 GetViewProjectionMatrix(bool includeOffset = true) const override;
        [[nodiscard]] glm::mat4 GetInverseViewProjectionMatrix(bool includeOffset = true) const override;
        [[nodiscard]] nvrhi::Rect GetViewExtent() const override;
        [[nodiscard]] glm::vec2 GetPixelOffset() const override;
    };
}