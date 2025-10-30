#include "Application.h"

ApplicationBase::ApplicationBase(DeviceManager* deviceManager) : Super(deviceManager),
m_RecompileShaders(false)
{

}

void ApplicationBase::Render(nvrhi::IFramebuffer* framebuffer)
{
	RenderScene(framebuffer);
}

void ApplicationBase::RenderScene(nvrhi::IFramebuffer* framebuffer)
{
	//Call Render() from various renderpass derived classes here
}

