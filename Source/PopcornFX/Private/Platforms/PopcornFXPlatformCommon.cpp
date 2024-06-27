//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXPlatform.h"

#include "Internal/ResourceHandlerImage_UE.h"

#include "Engine/Texture2D.h"

#if (PK_COMPILE_GPU != 0)
#	include "PopcornFXSettingsEditor.h"
#	include "Modules/ModuleManager.h"
#	include "Interfaces/IShaderFormat.h"
#	include "Interfaces/IShaderFormatModule.h"
#	include "RHIShaderFormatDefinitions.inl"
#	include "ShaderCompilerCore.h"
#	include "Misc/FileHelper.h"

#	if (ENGINE_MINOR_VERSION >= 3)
#		include "ShaderPreprocessTypes.h"
#	endif // (ENGINE_MINOR_VERSION >= 3)

#	include <pk_engine_utils/include/eu_random.h>
#	include <pk_maths/include/pk_maths_random.h>
#endif // (PK_COMPILE_GPU != 0)

#if (PKFX_COMMON_NewImageFromTexture != 0)
#	include "TextureResource.h"
#endif

#include <pk_maths/include/pk_numeric_tools_int.h>

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXPlatformCommon, Log, All);

//----------------------------------------------------------------------------

PopcornFX::CImage		*_CreateFallbackImage()
{
	const PopcornFX::CUint3				size(16, 16, 1);
	const PopcornFX::CImage::EFormat	format = PopcornFX::CImage::Format_BGRA8;

	const u32							bufferSize = PopcornFX::CImage::GetFormatPixelBufferSizeInBytes(format, size);
	PopcornFX::PRefCountedMemoryBuffer	dstBuffer = PopcornFX::CRefCountedMemoryBuffer::AllocAligned(bufferSize, 0x80);
	if (!PK_VERIFY(dstBuffer != null))
		return null;
	PopcornFX::Mem::Clear(dstBuffer->Data<u8>(), dstBuffer->DataSizeInBytes());

	PopcornFX::CImage					*image = PK_NEW(PopcornFX::CImage);
	if (!PK_VERIFY(image != null))
		return null;
	image->m_Format = format;

	if (!PK_VERIFY(image->m_Frames.Resize(1)) ||
		!PK_VERIFY(image->m_Frames[0].m_Mipmaps.Resize(1)))
	{
		PK_SAFE_DELETE(image);
		return null;
	}

	image->m_Frames[0].m_Mipmaps[0] = PopcornFX::CImageMap(size, dstBuffer);
	return image;
}

//----------------------------------------------------------------------------
//
//	CPU PopcornFX image creation from UTexture
//
//----------------------------------------------------------------------------

#if (PKFX_COMMON_NewImageFromTexture != 0)

PopcornFX::CImage	*PopcornFXPlatform_NewImageFromTexture(UTexture *texture)
{
#define	TEXTURE_ERROR_COMMONF(__msg, ...)	do {														\
		/*UE_LOG(LogPopcornFXPlatformCommon, Warning, __msg TEXT(": Texture 'LOD Group' must be 'ColorLookupTable': \"%s\""), ## __VA_ARGS__, *texture->GetPathName()); */\
		UE_LOG(LogPopcornFXPlatformCommon, Warning, TEXT("Could not load texture for sampling: ") __msg TEXT(": '%s'"), ## __VA_ARGS__, *texture->GetPathName()); \
	} while (0)

#define	TEXTURE_ERROR_COMMON(__msg)			TEXTURE_ERROR_COMMONF(TEXT("%s"), __msg)

#if PLATFORM_SWITCH
	TEXTURE_ERROR_COMMON(TEXT("Texture sampling not supported yet on Switch"));
	return _CreateFallbackImage();
#endif

	texture->ConditionalPostLoad();

	FTexturePlatformData	**platformDataPP = texture->GetRunningPlatformData();
	if (!/*PK_VERIFY*/(platformDataPP != null))
	{
		TEXTURE_ERROR_COMMON(TEXT("No PlatformData available"));
		return null;
	}

	FTexturePlatformData	*platformData = *platformDataPP;
	if (!/*PK_VERIFY*/(platformData != null))
	{
		TEXTURE_ERROR_COMMON(TEXT("Invalid PlatformData"));
		return null;
	}

#if WITH_EDITOR
	// Ugly workarround for cooking: create a fallback image.
	if (IsRunningCommandlet() && (platformData->Mips.Num() == 0 || !platformData->Mips[0].BulkData.IsBulkDataLoaded()))
	{
		//
		// During cooking, we cannot get a valid platformData because:
		// - either UTexture functions check for FApp::CanEverRender() (false while cooking)
		// - or we cannot call FTexturePlatformData functions because not exported
		// - or hacking PRIVATE_GIsRunningCommandlet makes UTexture::FinishCachePlatformData() check() fail
		//
		return _CreateFallbackImage();
	}
#endif

	if (!/*PK_VERIFY*/(platformData->Mips.Num() > 0))
	{
		TEXTURE_ERROR_COMMON(TEXT("No Mips in PlatformData"));
		return null;
	}

	/*const*/ FByteBulkData		&mipData = platformData->Mips[0].BulkData;

	const EPixelFormat			srcFormat = platformData->PixelFormat;
	const CUint3				imageSize(platformData->SizeX, platformData->SizeY, 1);
	const u32					bulkDataSize = mipData.GetBulkDataSize();

	if (!/*PK_VERIFY*/(All(imageSize.xy() > 0)) ||
		!/*PK_VERIFY*/(bulkDataSize > 0))
	{
		TEXTURE_ERROR_COMMON(TEXT("Empty PlatformData Mips BulkData"));
		return null;
	}

	const PopcornFX::CImage::EFormat	dstFormat = _UE2PKImageFormat(srcFormat, false);
	if (dstFormat == PopcornFX::CImage::Format_Invalid)
	{
		TEXTURE_ERROR_COMMONF(TEXT("Format '%s' not supported"), _My_GetPixelFormatString(srcFormat));
		return null;
	}

	const u32				expectedSizeInBytes = PopcornFX::CImage::GetFormatPixelBufferSizeInBytes(dstFormat, imageSize);
	if (!/*PK_VERIFY*/(bulkDataSize >= expectedSizeInBytes)) // don't mind if there is padding
	{
		TEXTURE_ERROR_COMMONF(TEXT("Mismatching texture size for format '%s' %dx%dx%d (got 0x%x bytes, expected 0x%x)"),
			_My_GetPixelFormatString(srcFormat),
			imageSize.x(), imageSize.y(), imageSize.z(),
			bulkDataSize, expectedSizeInBytes);
		return null;
	}

	enum { kAlignment = 0x80 };
	PopcornFX::PRefCountedMemoryBuffer	dstBuffer = PopcornFX::CRefCountedMemoryBuffer::AllocAligned(bulkDataSize + kAlignment, kAlignment);
	if (!PK_VERIFY(dstBuffer != null))
		return null;

	{
		// GetCopy crashes on 4.15 !
		void		*bulkDataCopyPtr = dstBuffer->Data<void>();
		mipData.GetCopy(&bulkDataCopyPtr, false);
		// no way to check if successful !?
		if (!PK_VERIFY(bulkDataCopyPtr == dstBuffer->Data<void>()))
		{
			TEXTURE_ERROR_COMMON(TEXT("Failed to get a copy of the texture"));
			return null;
		}
	}

	PopcornFX::CImage	*image = PK_NEW(PopcornFX::CImage);
	if (!PK_VERIFY(image != null) ||
		!PK_VERIFY(image->m_Frames.Resize(1)) ||
		!PK_VERIFY(image->m_Frames[0].m_Mipmaps.Resize(1)))
	{
		PK_DELETE(image);
		return null;
	}

	image->m_Format = dstFormat;

	PopcornFX::CImageMap	&dstMap = image->m_Frames[0].m_Mipmaps[0];
	dstMap.m_RawBuffer = dstBuffer;
	dstMap.m_Dimensions = imageSize;

#undef	TEXTURE_ERROR_COMMON
#undef	TEXTURE_ERROR_COMMONF

	return image;
}

#endif // (PKFX_COMMON_NewImageFromTexture != 0)

//----------------------------------------------------------------------------
//
//	GPU sim bytecodes compilation
//
//----------------------------------------------------------------------------

#if (PK_COMPILE_GPU != 0)

const char	*kKernelStageNames[] = {
	"KernelStage_Simulation",
	"KernelStage_Kill",
	"KernelStage_Bounds",
	"KernelStage_Merge"
};
PK_STATIC_ASSERT(PK_ARRAY_COUNT(kKernelStageNames) == PopcornFX::SBackendBuildInfos::__MaxKernelStages);

const char	*kExecFreqNames[] = {
	"Spawn",
	"Evolve_Low",
	"Evolve_Medium",
	"Evolve_High",
	"Evolve_Full",
	"Evolve_TimeVarying",
};
PK_STATIC_ASSERT(PK_ARRAY_COUNT(kExecFreqNames) == PopcornFX::Compiler::__MaxExecFreqs + 1); // +1, __MaxExecFreqs actually maps to a valid name

bool	CompileComputeShaderForAPI(	const PopcornFX::CString				&source,
									const PopcornFX::SBackendBuildInfos		&buildInfos,
									const PopcornFX::CStringView			&apiName,
									EShaderPlatform							shaderPlatform,
									const PopcornFX::CStringView			&bytecodeMagic,
									TArray<uint32>							&compilerFlags,
									PopcornFX::TArray<u8>					&outBytecode,
									PopcornFX::CMessageStream				&outMessages)
{
	// Force sync on the compilation, otherwise DXC crashes (cannot disable the async compilation in the baking right now)
	static FCriticalSection	stallWThreads;
	FScopeLock				scopedLock(&stallWThreads);

	TArray<FName>	modules;
	FModuleManager::Get().FindModules(SHADERFORMAT_MODULE_WILDCARD, modules);

	if (!modules.Num())
	{
		UE_LOG(LogPopcornFXPlatformCommon, Verbose, TEXT("Failed compiling bytecodes for %s: No target shader formats found"), *ToUE(apiName));
		return false;
	}

	TArray<FName>	supportedFormats;
	for (int32 iModule = 0; iModule < modules.Num(); ++iModule)
	{
		IShaderFormat	*shaderFormat = FModuleManager::LoadModuleChecked<IShaderFormatModule>(modules[iModule]).GetShaderFormat();

		if (shaderFormat == null)
			continue;
		supportedFormats.Reset();
		shaderFormat->GetSupportedFormats(supportedFormats);

		for (int32 iFormat = 0; iFormat < supportedFormats.Num(); ++iFormat)
		{
			const EShaderPlatform	currentShaderPlatform = ShaderFormatNameToShaderPlatform(supportedFormats[iFormat]);
			if (currentShaderPlatform != shaderPlatform)
				continue;
			const FName		&shaderFormatName = supportedFormats[iFormat];
			FString			srcFilePath;
			do
			{
				const u32	bakeKey = PopcornFX::Random::DefaultGenerator()->RandomU32();

				srcFilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / FString::Printf(TEXT("Saved/PopcornFX/GPUSimKernels/%s_%08X.usf"), *shaderFormatName.ToString(), bakeKey));
			} while (IFileManager::Get().FileExists(*srcFilePath));

			// Build shader header
			PopcornFX::CString		finalSource = source;
			{
				PopcornFX::CString	allBuildTags;
				for (const PopcornFX::CString &buildTag : buildInfos.m_BuildTags)
				{
					allBuildTags += buildTag;
					allBuildTags.Append(", ");
				}
				PK_ASSERT(buildInfos.m_KernelStage < PopcornFX::SBackendBuildInfos::__MaxKernelStages);

#if 0
				if (buildInfos.m_KernelStage == PopcornFX::SBackendBuildInfos::KernelStage_Simulation)
				{
					PK_ASSERT(buildInfos.m_KernelKey <= PopcornFX::Compiler::EExecFreq::__MaxExecFreqs); // <=, __MaxExecFreqs actually maps to a valid name
					finalSource.Prepend(PopcornFX::CString::Format("// Shader kernel key: %s\n", kExecFreqNames[buildInfos.m_KernelKey]));
				}
				else
					finalSource.Prepend(PopcornFX::CString::Format("// Shader kernel key: %d\n", buildInfos.m_KernelKey));
#endif

				finalSource.Prepend(PopcornFX::CString::Format(	"// Effect: %s\n"
																"// Layer: %s\n"
																"// Shader built for API: %s\n"
																"// Build tags: %s\n"
																"// Shader kernel stage: %s\n",
																buildInfos.m_SourceEffect.Empty() ? "<not-available>" : buildInfos.m_SourceEffect.ToString().Data(),
																buildInfos.m_SourceLayerName.Empty() ? "<not-available>" : buildInfos.m_SourceLayerName.ToString().Data(),
																apiName.Data(),
																allBuildTags.Data(),
																kKernelStageNames[buildInfos.m_KernelStage]));
			}

			if (!FFileHelper::SaveStringToFile(ToUE(finalSource), *srcFilePath))
			{
				UE_LOG(LogPopcornFXPlatformCommon, Verbose, TEXT("Failed compiling bytecodes for %s: couldn't create temp source file"), *ToUE(apiName));
				return false;
			}

			FShaderCompilerInput	input;
			{
				input.Target = FShaderTarget(SF_Compute, shaderPlatform);
				input.ShaderFormat = shaderFormatName;

				// Tell UE where the content is located, and to skip the mcpp pass
				input.VirtualSourceFilePath = srcFilePath;
				input.EntryPointName = "main";
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 4)
				input.DebugInfoFlags |= EShaderDebugInfoFlags::CompileFromDebugUSF;
#else
				input.bSkipPreprocessedCache = true;
#endif

				for (u32 compilerFlag : compilerFlags)
					input.Environment.CompilerFlags.Append(compilerFlag);
			}

			FShaderCompilerOutput		output;
			const FString				workingDirectory;
#if (ENGINE_MINOR_VERSION >= 3)
			FShaderPreprocessOutput		preprocessorOutput;
			FShaderCompilerEnvironment	mergedEnvironment;
			if (!shaderFormat->PreprocessShader(input, mergedEnvironment, preprocessorOutput))
			{
				UE_LOG(LogPopcornFXPlatformCommon, Verbose, TEXT("Failed preprocessing shader for %s"), *ToUE(apiName));
				return false;
			}
			shaderFormat->CompilePreprocessedShader(input, preprocessorOutput, output, workingDirectory);
#else
			shaderFormat->CompileShader(shaderFormatName, input, output, workingDirectory);
#endif // (ENGINE_MINOR_VERSION >= 3)

			if (output.bSucceeded)
			{
				const TArray<uint8>	&shaderCodeWithOptionalData = output.ShaderCode.GetReadAccess();
				const u8			*fullShaderCode = shaderCodeWithOptionalData.GetData();
				const u8			*fullShaderCodeEnd = fullShaderCode + shaderCodeWithOptionalData.Num();
				const u8			*shaderCodeStart = fullShaderCode;
				{
					// Look for the beginning of DXBC bytecode, we don't need UE's metadata
					while (shaderCodeStart < fullShaderCodeEnd && *shaderCodeStart != bytecodeMagic[0])
						++shaderCodeStart;
					if (shaderCodeStart >= fullShaderCodeEnd ||
						(fullShaderCodeEnd - shaderCodeStart) < bytecodeMagic.Length())
					{
						UE_LOG(LogPopcornFXPlatformCommon, Verbose, TEXT("Failed compiling bytecodes for %s: couldn't find %s header"), *ToUE(apiName), *ToUE(bytecodeMagic));
						return false;
					}
					for (u32 i = 0; i < bytecodeMagic.Length(); ++i)
					{
						if (shaderCodeStart[i] != bytecodeMagic[i])
						{
							UE_LOG(LogPopcornFXPlatformCommon, Verbose, TEXT("Failed compiling bytecodes for %s: couldn't find %s header"), *ToUE(apiName), *ToUE(bytecodeMagic));
							return false;
						}
					}
				}

				const u32	headerSizeInBytes = shaderCodeStart - fullShaderCode;
				const u32	bytecodeSizeInBytes = output.ShaderCode.GetShaderCodeSize() - headerSizeInBytes; // do not grab additional data
				if (!PK_VERIFY(outBytecode.Resize(bytecodeSizeInBytes)))
					return false;
				PopcornFX::Mem::Copy(outBytecode.RawDataPointer(), shaderCodeStart, bytecodeSizeInBytes);

				// We can delete the temp file now
				IFileManager::Get().Delete(*srcFilePath);

				return true;
			}
			else
			{
				for (const FShaderCompilerError &error : output.Errors)
				{
					const FString	errorString = error.GetErrorString();
					if (!errorString.IsEmpty())
						UE_LOG(LogPopcornFXPlatformCommon, Verbose, TEXT("Failed compiling bytecodes for %s: '%s'"), *ToUE(apiName), *errorString);
				}
				return false;
			}
		}
	}

	UE_LOG(LogPopcornFXPlatformCommon, Verbose, TEXT("Failed compiling bytecodes for %s: couldn't find shader compiler module"), *ToUE(apiName));
	return false;
}
#endif // (PK_COMPILE_GPU != 0)

//----------------------------------------------------------------------------

#if (PK_COMPILE_GPU_PC != 0)
namespace
{
	// For D3D11, we will use SM5.0, compiled with FXC
	bool	_FXCCompileBytecode(const PopcornFX::SCompileCacheBuildArgs::SBackendConfig	&backendCfg,
								const PopcornFX::SBackendBuildInfos						&buildInfos,
								const PopcornFX::CString								&source,
								PopcornFX::TArray<u8>									&outBytecode,
								PopcornFX::CString										&outRawCompilerOutput,
								PopcornFX::CMessageStream								&outMessages)
	{
		TArray<uint32>	compilerFlags;
		return CompileComputeShaderForAPI(source, buildInfos, "D3D11", SP_PCD3D_SM5, "DXBC", compilerFlags, outBytecode, outMessages);
	}

	//----------------------------------------------------------------------------

	// For D3D12, we will use SM6.0, compiled with DXC
	bool	_DXCCompileBytecode(const PopcornFX::SCompileCacheBuildArgs::SBackendConfig	&backendCfg,
								const PopcornFX::SBackendBuildInfos						&buildInfos,
								const PopcornFX::CString								&source,
								PopcornFX::TArray<u8>									&outBytecode,
								PopcornFX::CString										&outRawCompilerOutput,
								PopcornFX::CMessageStream								&outMessages)
	{
		TArray<uint32>	compilerFlags;
		return CompileComputeShaderForAPI(source, buildInfos, "D3D12", SP_PCD3D_SM6, "DXBC", compilerFlags, outBytecode, outMessages);
	}
}

//----------------------------------------------------------------------------

void	PopcornFXPlatform_PC_SetupGPUBackendCompilers(	const PopcornFX::CString		&fileSourceVirtualPath,
														u32								backendTargets,
														PopcornFXCbBuildBytecodeArray	&outCbCompiles)
{
	UPopcornFXSettingsEditor	*settings = GetMutableDefault<UPopcornFXSettingsEditor>();
	if (!PK_VERIFY(settings != null))
		return;

	if (backendTargets & (1 << PopcornFX::BackendTarget_D3D11))
		PK_VERIFY(outCbCompiles.PushBack(SPopcornFXCbBuildBytecode(PopcornFX::COvenBakeConfig_ParticleCompiler::CbBuildBytecode(&_FXCCompileBytecode), PopcornFX::BackendTarget_D3D11)).Valid());
	if (backendTargets & (1 << PopcornFX::BackendTarget_D3D12))
		PK_VERIFY(outCbCompiles.PushBack(SPopcornFXCbBuildBytecode(PopcornFX::COvenBakeConfig_ParticleCompiler::CbBuildBytecode(&_DXCCompileBytecode), PopcornFX::BackendTarget_D3D12)).Valid());
}
#endif // (PK_COMPILE_GPU_PC != 0)
