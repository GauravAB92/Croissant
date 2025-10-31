#include <glm/glm.hpp>
#include <memory>
#include <nvrhi/nvrhi.h>
#include <core/VFS.h>



class PixelReadBackPass
{

public:
	PixelReadBackPass(
		nvrhi::IDevice* device,
		std::shared_ptr<vfs::RootFileSystem>& fs,
		nvrhi::ITexture* sourceTexture,
		nvrhi::Format sourceFormat,
		uint32_t arraySlice = 0,
		uint32_t mipLevel = 0);

	void Capture(nvrhi::ICommandList* commandList, glm::uvec2 pixelCoords);
	glm::vec4  ReadFloats();
	glm::uvec4 ReadUInts();
	glm::ivec4 ReadInts();

private:
	nvrhi::DeviceHandle m_Device;
	nvrhi::ShaderHandle m_Shader;
	nvrhi::ComputePipelineHandle m_Pipeline;
	nvrhi::BindingLayoutHandle m_BindingLayout;
	nvrhi::BindingSetHandle m_BindingSet;
	nvrhi::BufferHandle m_ConstantBuffer;
	nvrhi::BufferHandle m_IntermediateBuffer;
	nvrhi::BufferHandle m_ReadbackBuffer;

};