#pragma once
#include <nvrhi/nvrhi.h>
#include <engine/View.h>
#include <core/VFS.h>
#include <render/backend/DeviceManager.h>


namespace Croissant
{

    class GeometryPassContext
    {

    };

    class IUpdate
    {
    public:
        //Required functions for update   
        virtual void RebuildShaders(DeviceManager* deviceManager, std::shared_ptr<vfs::RootFileSystem>& fs) = 0;
		virtual ~IUpdate() = default;
    };

	class IGeometryPass : public IUpdate
	{

    public:
		//Necessary setup for the pass, such as render targets, viewport, etc. This is called once per view, and can be used to set up view-specific data in the context that will be used in the other methods.
            
        virtual void SetupView(GeometryPassContext& context, nvrhi::ICommandList* commandList, const Croissant::IView* view, const Croissant::IView* viewPrev) = 0;
       // virtual bool SetupMaterial(GeometryPassContext& context, const engine::Material* material, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state) = 0;
      //  virtual void SetupInputBuffers(GeometryPassContext& context, const engine::BufferGroup* buffers, nvrhi::GraphicsState& state) = 0;
        virtual void SetPushConstants(GeometryPassContext& context, nvrhi::ICommandList* commandList, nvrhi::GraphicsState& state, nvrhi::DrawArguments& args) = 0;
        
       
        virtual ~IGeometryPass() = default;
	};
}

