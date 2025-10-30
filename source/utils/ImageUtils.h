#pragma once

#include <core/stdafx.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <utils/stb_image.h>
#include <utils/stb_image_write.h>
#include <nvrhi/nvrhi.h>
#include <render/backend/DeviceManager.h>
#include <render/renderpasses/CommonRenderPasses.h>
#include <core/log.h>
#include <core/VFS.h>

#ifdef _MSC_VER 
#define strcasecmp _stricmp
#endif


static bool SaveTextureToFile(
    nvrhi::IDevice* device,
    CommonRenderPasses* pPasses,
    nvrhi::ITexture* texture,
    nvrhi::ResourceStates textureState,
    const char* fileName,
    bool saveAlphaChannel)
{
    if (!fileName)
        return false;

    // Find the file's extension
    char const* ext = strrchr(fileName, '.');

    if (!ext)
        return false; // No extension fond in the file name

    // Determine the image format from the extension
    enum { BMP, PNG, JPG, TGA } destFormat;
    if (strcasecmp(ext, ".bmp") == 0)
        destFormat = BMP;
    else if (strcasecmp(ext, ".png") == 0)
        destFormat = PNG;
    else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)
        destFormat = JPG;
    else if (strcasecmp(ext, ".tga") == 0)
        destFormat = TGA;
    else
        return false; // Unknown file type

    if (destFormat == JPG)
        saveAlphaChannel = false;

    nvrhi::TextureDesc desc = texture->getDesc();
    nvrhi::TextureHandle tempTexture;
    nvrhi::FramebufferHandle tempFramebuffer;

    nvrhi::CommandListHandle commandList = device->createCommandList();
    commandList->open();

    if (textureState != nvrhi::ResourceStates::Unknown)
    {
        commandList->beginTrackingTextureState(texture, nvrhi::TextureSubresourceSet(0, 1, 0, 1), textureState);
    }

    // If the source texture format is not RGBA8, create a temporary texture and blit into it to convert
    switch (desc.format)
    {
    case nvrhi::Format::RGBA8_UNORM:
    case nvrhi::Format::SRGBA8_UNORM:
        tempTexture = texture;
        break;
    default:
        desc.format = nvrhi::Format::SRGBA8_UNORM;
        desc.isRenderTarget = true;
        desc.initialState = nvrhi::ResourceStates::RenderTarget;
        desc.keepInitialState = true;

        tempTexture = device->createTexture(desc);
        tempFramebuffer = device->createFramebuffer(nvrhi::FramebufferDesc().addColorAttachment(tempTexture));

        BlitParameters params;
		params.sourceTexture = texture;
		params.targetFramebuffer = tempFramebuffer;
	
        //pPasses->BlitTexture(commandList, tempFramebuffer, texture);
    }

    // Create a staging texture to access the data from the CPU, copy the data into it
    nvrhi::StagingTextureHandle stagingTexture = device->createStagingTexture(desc, nvrhi::CpuAccessMode::Read);
    commandList->copyTexture(stagingTexture, nvrhi::TextureSlice(), tempTexture, nvrhi::TextureSlice());

    if (textureState != nvrhi::ResourceStates::Unknown)
    {
        commandList->setTextureState(texture, nvrhi::TextureSubresourceSet(0, 1, 0, 1), textureState);
        commandList->commitBarriers();
    }

    commandList->close();
    device->executeCommandList(commandList);

    // Map the staging texture
    size_t rowPitch = 0;
    uint8_t const* pData = static_cast<uint8_t const*>(device->mapStagingTexture(
        stagingTexture, nvrhi::TextureSlice(), nvrhi::CpuAccessMode::Read, &rowPitch));

    if (!pData)
        return false;

    uint8_t* newData = nullptr;
    int channels = saveAlphaChannel ? 4 : 3;

    // If the mapped data is not laid out in a densely packed format with the right number of channels,
    // create a temporary buffer and move the data into the right layout for stb_image.
    if (rowPitch != desc.width * channels)
    {
        newData = new uint8_t[desc.width * desc.height * channels];

        for (uint32_t row = 0; row < desc.height; ++row)
        {
            uint8_t* dstRow = newData + row * desc.width * channels;
            uint8_t const* srcRow = pData + row * rowPitch;

            if (channels == 4)
            {
                // Simple row copy
                memcpy(dstRow, srcRow, desc.width * channels);
            }
            else
            {
                // Convert 4 channels to 3
                for (uint32_t col = 0; col < desc.width; ++col)
                {
                    dstRow[0] = srcRow[0];
                    dstRow[1] = srcRow[1];
                    dstRow[2] = srcRow[2];
                    dstRow += 3;
                    srcRow += 4;
                }
            }
        }

        pData = newData;
    }

    // Write the output image
    bool writeSuccess = false;
    switch (destFormat)
    {
    case BMP:
        writeSuccess = stbi_write_bmp(fileName, int(desc.width), int(desc.height), channels, pData) != 0;
        break;
    case PNG:
        writeSuccess = stbi_write_png(fileName, int(desc.width), int(desc.height), channels, pData, desc.width * channels) != 0;
        break;
    case JPG:
        writeSuccess = stbi_write_jpg(fileName, int(desc.width), int(desc.height), channels, pData, /* quality = */ 99) != 0;
        break;
    case TGA:
        writeSuccess = stbi_write_tga(fileName, int(desc.width), int(desc.height), channels, pData) != 0;
        break;
    }

    if (newData)
    {
        delete[] newData;
        newData = nullptr;
    }

    device->unmapStagingTexture(stagingTexture);

    return writeSuccess;
}



static bool SaveHDRTextureToFile(
    nvrhi::IDevice* device,
    CommonRenderPasses* pPasses,
    nvrhi::ITexture* texture,
    nvrhi::ResourceStates textureState,
    const char* fileName,
    bool saveAlphaChannel,
    bool saveFloatFile)
{
    if (!fileName)
    {
        return false;
    }
        
    // Find the file's extension
    char const* ext = strrchr(fileName, '.');
  
    if (!ext)
    {
        logger::error("No extension found in the file name: %s", fileName);
        return false; // No extension fond in the file name
    }

    // Determine the image format from the extension
    enum { BMP, PNG, JPG, TGA, HDR } destFormat;
    if (strcasecmp(ext, ".bmp") == 0)
    {
        destFormat = BMP;
    }
    else if (strcasecmp(ext, ".png") == 0)
    {
        destFormat = PNG;
    }
    else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)
    {
        destFormat = JPG;
    }
    else if (strcasecmp(ext, ".tga") == 0)
    {
        destFormat = TGA;
    }
    else if (strcasecmp(ext, ".hdr") == 0)
    {
        destFormat = HDR;

    }
    else
    {
        logger::error("Unknown file type for HDR export: %s", fileName);
        return false; // Unknown file type

    }
		
    if (destFormat == JPG)
        saveAlphaChannel = false;

    nvrhi::TextureDesc desc = texture->getDesc();
    nvrhi::TextureHandle tempTexture;
    nvrhi::FramebufferHandle tempFramebuffer;

    nvrhi::CommandListHandle commandList = device->createCommandList();
    commandList->open();

    if (textureState != nvrhi::ResourceStates::Unknown)
    {
        commandList->beginTrackingTextureState(texture, nvrhi::TextureSubresourceSet(0, 1, 0, 1), textureState);
    }

    // If the source texture format is not RGBA8, create a temporary texture and blit into it to convert
    switch (desc.format)
    {
    case nvrhi::Format::RGBA8_UNORM:
    case nvrhi::Format::SRGBA8_UNORM:
    case nvrhi::Format::RGBA32_FLOAT:
    case nvrhi::Format::RGBA16_FLOAT:
        tempTexture = texture;
        break;
    default:
        desc.format = nvrhi::Format::SRGBA8_UNORM;
        desc.isRenderTarget = true;
        desc.initialState = nvrhi::ResourceStates::RenderTarget;
        desc.keepInitialState = true;

        tempTexture = device->createTexture(desc);
        tempFramebuffer = device->createFramebuffer(nvrhi::FramebufferDesc().addColorAttachment(tempTexture));

       // pPasses->BlitTexture(commandList, tempFramebuffer, texture);
    }

    // Create a staging texture to access the data from the CPU, copy the data into it
    nvrhi::StagingTextureHandle stagingTexture = device->createStagingTexture(desc, nvrhi::CpuAccessMode::Read);


    commandList->copyTexture(stagingTexture, nvrhi::TextureSlice(), tempTexture, nvrhi::TextureSlice());

    if (textureState != nvrhi::ResourceStates::Unknown)
    {
        commandList->setTextureState(texture, nvrhi::TextureSubresourceSet(0, 1, 0, 1), textureState);
        commandList->commitBarriers();
    }

    commandList->close();
    device->executeCommandList(commandList);

    // Map the staging texture
    size_t rowPitch = 0;



    const uint16_t* pData = static_cast<const uint16_t*>(device->mapStagingTexture(
        stagingTexture, nvrhi::TextureSlice(), nvrhi::CpuAccessMode::Read, &rowPitch));

    if (!pData)
    {
        logger::error("Failed to map staging texture for HDR export: %s", fileName);
        return false;
    }
		
    // Write the output image
    bool writeSuccess = false;
    int channels = saveAlphaChannel ? 4 : 3;

    if (destFormat == HDR || saveFloatFile)
    {
        // Write HDR file using stbi_write_hdr
        // WriteSuccess = stbi_write_hdr(fileName, 4, 1, 4, pixels) != 0;

        std::ofstream file(fileName, std::ios::binary);
        if (!file.is_open()) 
        {
			logger::error("Failed to open file for writing: %s", fileName);
            return false;
        }

        // Write custom header
        file.write("HDR_CUSTOM", 10); // Identifier
        file.write(reinterpret_cast<const char*>(&(desc.width)), sizeof(int));  // Width
        file.write(reinterpret_cast<const char*>(&(desc.height)), sizeof(int)); // Height
        file.write(reinterpret_cast<const char*>(&channels), sizeof(int)); // Channels

        // Write pixel data directly as floating-point values
        int totalPixels = int(desc.width) * int(desc.height) * channels;
        file.write(reinterpret_cast<const char*>(pData), totalPixels * sizeof(uint16_t));
      
        if (!file.good())
        {
            logger::error("Failed to write pixel data to file: %s", fileName);
        }

		writeSuccess = file.good();

        file.close();

    }
    else
    {
        uint8_t* newData = nullptr;

        // If the mapped data is not laid out in a densely packed format with the right number of channels,
        // create a temporary buffer and move the data into the right layout for stb_image.
        if (rowPitch != desc.width * channels)
        {
            newData = new uint8_t[desc.width * desc.height * channels];

            for (uint32_t row = 0; row < desc.height; ++row)
            {
                uint8_t* dstRow = newData + row * desc.width * channels;
                const uint8_t* srcRow = reinterpret_cast<const uint8_t*>(pData) + row * rowPitch;

                if (channels == 4)
                {
                    // Simple row copy
                    memcpy(dstRow, srcRow, desc.width * channels);
                }
                else
                {
                    // Convert 4 channels to 3
                    for (uint32_t col = 0; col < desc.width; ++col)
                    {
                        dstRow[0] = srcRow[0];
                        dstRow[1] = srcRow[1];
                        dstRow[2] = srcRow[2];
                        dstRow += 3;
                        srcRow += 4;
                    }
                }
            }

            pData = reinterpret_cast<const uint16_t*>(newData);
        }

        switch (destFormat)
        {
        case BMP:
            writeSuccess = stbi_write_bmp(fileName, int(desc.width), int(desc.height), channels, pData) != 0;
            break;
        case PNG:
            writeSuccess = stbi_write_png(fileName, int(desc.width), int(desc.height), channels, pData, desc.width * channels) != 0;
            break;
        case JPG:
            writeSuccess = stbi_write_jpg(fileName, int(desc.width), int(desc.height), channels, pData, /* quality = */ 99) != 0;
            break;
        case TGA:
            writeSuccess = stbi_write_tga(fileName, int(desc.width), int(desc.height), channels, pData) != 0;
            break;
        case HDR:
            break;
        }

        if (newData)
        {
            delete[] newData;
            newData = nullptr;
        }

    }

    device->unmapStagingTexture(stagingTexture);

    return writeSuccess;
}


