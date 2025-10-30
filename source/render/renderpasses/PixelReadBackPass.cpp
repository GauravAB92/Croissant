#include "PixelReadBackPass.h"

PixelReadBackPass::PixelReadBackPass(nvrhi::IDevice* device, std::shared_ptr<vfs::RootFileSystem>& fs, nvrhi::ITexture* sourceTexture, nvrhi::Format sourceFormat, uint32_t arraySlice, uint32_t mipLevel)
{

}

void PixelReadBackPass::Capture(nvrhi::ICommandList* commandList, glm::uvec2 pixelCoords)
{
}

glm::vec4 PixelReadBackPass::ReadFloats()
{
	return glm::vec4();
}

glm::uvec4 PixelReadBackPass::ReadUInts()
{
	return glm::uvec4();
}

glm::ivec4 PixelReadBackPass::ReadInts()
{
	return glm::ivec4();
}
