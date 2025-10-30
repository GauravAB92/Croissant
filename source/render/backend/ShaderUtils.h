#pragma once

#include <core/stdafx.h>
#include <nvrhi/nvrhi.h>	
#include <wrl/client.h>
#include <dxcapi.h>
#include <dxc/package/inc/d3d12shader.h>
#include <core/stdafx.h>
#include <core/VFS.h>

using Microsoft::WRL::ComPtr;

static bool CompileShaderFileNVRHIDXC(
	LPCSTR            filename,
	LPCSTR            entry_point,
	nvrhi::ShaderType shadertype,
	D3D_SHADER_MACRO* pDefines,
	nvrhi::IDevice* device,
	std::shared_ptr<vfs::RootFileSystem> fs,
	nvrhi::ShaderHandle& shader
)
{
#if defined(_WIN32) && defined(ENABLE_D3D12)
	auto shaderData = fs->readFile(filename);
	if (!shaderData) {
		logger::error("Failed to read shader file: %s", filename);
		return false;
	}

	ComPtr<IDxcUtils> pUtils;
	ComPtr<IDxcCompiler3> pCompiler;
	ComPtr<IDxcIncludeHandler> pIncludeHandler;
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
	if (FAILED(hr)) return false;
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));
	if (FAILED(hr)) return false;
	hr = pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
	if (FAILED(hr)) return false;

	DxcBuffer sourceBuffer = {};
	sourceBuffer.Ptr = shaderData->data();
	sourceBuffer.Size = shaderData->size();
	sourceBuffer.Encoding = DXC_CP_UTF8;                 // FIX: set UTF-8 explicitly

	std::wstring eraaPath = vfs::getExecutablePath().parent_path().wstring() + L"/shaders/ERAA";
	std::wstring commonPath = vfs::getExecutablePath().parent_path().wstring() + L"/shaders/common";

	LPCWSTR targetProfile = L"";
	switch (shadertype) {
	case nvrhi::ShaderType::Vertex:   targetProfile = L"vs_6_0"; break;
	case nvrhi::ShaderType::Pixel:    targetProfile = L"ps_6_0"; break;
	case nvrhi::ShaderType::Geometry: targetProfile = L"gs_6_0"; break;
	case nvrhi::ShaderType::Hull:     targetProfile = L"hs_6_0"; break;
	case nvrhi::ShaderType::Domain:   targetProfile = L"ds_6_0"; break;
	case nvrhi::ShaderType::Compute:  targetProfile = L"cs_6_0"; break;
	case nvrhi::ShaderType::Mesh:     targetProfile = L"ms_6_0"; break;
	default:
		logger::error("Unknown shader type");
		return false;
	}

	std::vector<LPCWSTR> arguments;
	std::wstring wEntryPoint;
	if (entry_point) {
		wEntryPoint.assign(entry_point, entry_point + strlen(entry_point));
	}
	else {
		wEntryPoint = L"main";
	}
	arguments.push_back(L"-E"); arguments.push_back(wEntryPoint.c_str());
	arguments.push_back(L"-T"); arguments.push_back(targetProfile);
	arguments.push_back(L"-I"); arguments.push_back(eraaPath.c_str());
	arguments.push_back(L"-I"); arguments.push_back(commonPath.c_str());
	arguments.push_back(L"-Zi");
	arguments.push_back(L"-Qembed_debug");
	arguments.push_back(L"-HV"); arguments.push_back(L"2021");      // (optional) HLSL 2021

	// pass defines to DXC ----
	std::vector<std::wstring> defineArgs;  // keep storage alive
	if (pDefines) {
		for (D3D_SHADER_MACRO* def = pDefines; def && def->Name; ++def) {
			std::wstring tok = L"-D";
			tok += std::wstring(def->Name, def->Name + strlen(def->Name));
			if (def->Definition && *def->Definition) {
				tok += L"=";
				tok += std::wstring(def->Definition, def->Definition + strlen(def->Definition));
			}
			defineArgs.push_back(std::move(tok));
			arguments.push_back(defineArgs.back().c_str());
		}
	}
	// -------------------------------------------

	ComPtr<IDxcResult> pResult;
	hr = pCompiler->Compile(
		&sourceBuffer,
		arguments.data(),
		(UINT32)arguments.size(),
		pIncludeHandler.Get(),
		IID_PPV_ARGS(&pResult)
	);
	if (FAILED(hr) || !pResult) {
		logger::error("DXC call failed (HRESULT=0x%08X)", hr);
		return false;
	}

	// ---- FIX: distinguish warnings from errors ----
	HRESULT status = S_OK;
	pResult->GetStatus(&status);

	ComPtr<IDxcBlobUtf8> pErrors;
	pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
	if (pErrors && pErrors->GetStringLength() > 0) {
		if (FAILED(status)) {
			logger::error("%s:\nShader compilation errors:\n%s", filename, pErrors->GetStringPointer());
			return false;
		}
		else {
			logger::warning("%s:\nShader compilation warnings:\n%s", filename, pErrors->GetStringPointer());
		}
	}
	if (FAILED(status)) {
		logger::error("DXC reported failure with no message");
		return false;
	}
	// -----------------------------------------------

	ComPtr<IDxcBlob> pShader;
	hr = pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), nullptr);
	if (FAILED(hr) || !pShader) {
		logger::error("Failed to get compiled shader blob from DXC");
		return false;
	}

	auto shaderDesc = nvrhi::ShaderDesc().setShaderType(shadertype);
	shader = device->createShader(shaderDesc, pShader->GetBufferPointer(), pShader->GetBufferSize());
	if (!shader) {
		logger::error("Failed to create NVRHI shader from DXC blob");
		return false;
	}
	return true;
#else
	return false;
#endif
}



struct DirIncludeHandler : public ID3DInclude
{
	std::vector<std::wstring> m_searchDirs;

	// ctor takes a list of absolute or relative paths
	DirIncludeHandler(std::initializer_list<std::wstring> paths)
		: m_searchDirs(paths)
	{
	}

	STDMETHOD(Open)(
		D3D_INCLUDE_TYPE IncludeType,
		LPCSTR           pFileName,
		LPCVOID          pParentData,
		LPCVOID* ppData,
		UINT* pBytes) override
	{
		for (auto& dir : m_searchDirs)
		{
			std::filesystem::path full = std::filesystem::path(dir) / pFileName;
			std::wstringstream ss;
			ss << L"[Include] trying: " << full.wstring() << L"\n";
			OutputDebugStringW(ss.str().c_str());

			if (!std::filesystem::exists(full))
				continue;

			std::ifstream f(full, std::ios::binary | std::ios::ate);
			if (!f)
				continue;

			auto size = (size_t)f.tellg();
			f.seekg(0);
			char* data = new char[size];
			f.read(data, size);
			*ppData = data;
			*pBytes = (UINT)size;

			ss.str(L"");
			ss << L"[Include] loaded: " << full.wstring() << L"\n";
			OutputDebugStringW(ss.str().c_str());
			return S_OK;
		}

		OutputDebugStringA("[Include] FAILED to find include\n");
		return E_FAIL;
	}

	STDMETHOD(Close)(LPCVOID pData) override
	{
		delete[](char*)pData;
		return S_OK;
	}
};


// Compiles a shader from a file and returns the compiled blob.
static bool CompileShaderFileNVRHI(
	LPCSTR            filename,
	LPCSTR            entry_point,
	nvrhi::ShaderType shadertype,
	D3D_SHADER_MACRO* pDefines,
	nvrhi::IDevice* device,
	std::shared_ptr<vfs::RootFileSystem> fs,
	nvrhi::ShaderHandle& shader)
{
	//ID3DBlob* blob = nullptr;
	//ID3DBlob* errorBlob = nullptr;
	//HRESULT   hr = S_OK;

	//auto shaderData = fs->readFile(filename);

	//std::wstring eraaPath = vfs::getExecutablePath().parent_path().wstring() + L"/shaders/ERAA";
	//std::wstring commonPath = vfs::getExecutablePath().parent_path().wstring() + L"/shaders/common";


	//DirIncludeHandler includeHandler({ eraaPath,commonPath });

	//if (!shaderData)
	//{
	//	logger::error("Failed to read shader file: %s", filename);
	//	return false;
	//}
	//else
	//{
	//	//include handler to includ /shaders directory
	//	

	//	auto compile = [&](LPCSTR target)
	//		{
	//			hr = D3DCompile(
	//				shaderData->data(),
	//				shaderData->size(),
	//				filename,
	//				pDefines,
	//				&includeHandler,
	//				entry_point,
	//				target,
	//				0,            // compile flags1
	//				0,            // compile flags2
	//				&blob,
	//				&errorBlob
	//			);
	//		};


	//	switch (shadertype)
	//	{
	//	case nvrhi::ShaderType::Vertex:   compile("vs_5_1"); break;
	//	case nvrhi::ShaderType::Pixel:    compile("ps_5_1"); break;
	//	case nvrhi::ShaderType::Geometry: compile("gs_5_0"); break;
	//	case nvrhi::ShaderType::Hull:     compile("hs_5_0"); break;
	//	case nvrhi::ShaderType::Domain:   compile("ds_5_0"); break;
	//	case nvrhi::ShaderType::Compute:  compile("cs_5_0"); break;
	//	case nvrhi::ShaderType::Mesh:     compile("ms_6_0"); break;
	//	default:
	//		logger::error("Unknown shader type");
	//	}

	//	if (FAILED(hr))
	//	{
	//		// If the compiler gave us an error blob, show it:
	//		if (errorBlob)
	//		{
	//			const char* msg = static_cast<const char*>(errorBlob->GetBufferPointer());
	//			MessageBoxA(
	//				nullptr,
	//				msg,
	//				"D3DCompileFromFile: Shader Compilation Error\n",
	//				MB_OK | MB_ICONERROR
	//			);
	//			errorBlob->Release();
	//		}
	//		// Clean up any partial blob and propagate error:
	//		if (blob) blob->Release();
	//		return false;
	//	}

	//	// Clean up the error blob if it was produced but compilation succeeded:
	//	if (errorBlob)
	//		errorBlob->Release();

	//	// Create and return the NVRHI shader
	//	auto shaderDesc = nvrhi::ShaderDesc()
	//		.setShaderType(shadertype);

	//	if (shader)
	//		shader = nullptr;

	//	shader = device->createShader(
	//		shaderDesc,
	//		blob->GetBufferPointer(),
	//		blob->GetBufferSize()
	//	);

	//	if (!shader)
	//	{
	//		logger::error("Failed to create NVRHI shader from compiled blob");
	//		return false;
	//	}

	//	blob->Release();
	//	return true;
	//}

	return CompileShaderFileNVRHIDXC(filename, entry_point, shadertype, pDefines, device, fs, shader);
}

