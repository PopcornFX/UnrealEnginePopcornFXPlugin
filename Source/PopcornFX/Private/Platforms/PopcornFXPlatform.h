//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "HAL/Platform.h"

#include "PopcornFXSDK.h"
#if WITH_EDITOR
#	if (PK_HAS_GPU != 0)
#		include "RHIDefinitions.h"

#		include <PK-AssetBakerLib/AssetBaker_Oven_HBO.h>
#	endif // (PK_HAS_GPU != 0)
#endif // WITH_EDITOR

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 2)
#	if (PK_COMPILE_GPU != 0)
#		include "RHIShaderPlatform.h"
#	endif
#endif

FWD_PK_API_BEGIN
class CImage;
FWD_PK_API_END

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5)
#	define PKFX_COMMON_NewImageFromTexture		1
#else
#	if PLATFORM_PS4 || PLATFORM_XBOXONE
#	define PKFX_COMMON_NewImageFromTexture		0
#	else
#	define PKFX_COMMON_NewImageFromTexture		1
#	endif // PLATFORM_PS4 || PLATFORM_XBOXONE
#endif // (ENGINE_MAJOR_VERSION == 5)

//----------------------------------------------------------------------------

PopcornFX::CImage			*PopcornFXPlatform_NewImageFromTexture(class UTexture *texture);
extern PopcornFX::CImage	*_CreateFallbackImage();

//----------------------------------------------------------------------------

#if (PK_COMPILE_GPU != 0)

struct	SPopcornFXCbBuildBytecode
{
	PopcornFX::COvenBakeConfig_ParticleCompiler::CbBuildBytecode	m_CbBuildBytecode;
	PopcornFX::EBackendTarget										m_Target;

	SPopcornFXCbBuildBytecode(const PopcornFX::COvenBakeConfig_ParticleCompiler::CbBuildBytecode &cb, PopcornFX::EBackendTarget target)
	:	m_CbBuildBytecode(cb)
	,	m_Target(target)
	{
	}
};
typedef PopcornFX::TArray<SPopcornFXCbBuildBytecode>	PopcornFXCbBuildBytecodeArray;

bool	CompileComputeShaderForAPI(	const PopcornFX::CString			&source,
									const PopcornFX::SBackendBuildInfos	&buildInfos,
									const PopcornFX::CStringView		&apiName,
									EShaderPlatform						shaderPlatform,
									const PopcornFX::CStringView		&bytecodeMagic,
									TArray<uint32>						&compilerFlags,
									PopcornFX::TArray<u8>				&outBytecode,
									PopcornFX::CMessageStream			&outMessages);
#endif // (PK_COMPILE_GPU != 0)

//----------------------------------------------------------------------------

#if (PK_COMPILE_GPU_PC != 0)
void	PopcornFXPlatform_PC_SetupGPUBackendCompilers(	const PopcornFX::CString		&fileSourceVirtualPath,
														u32								backendTargets, // PopcornFX::EBackendTarget
														PopcornFXCbBuildBytecodeArray	&outCbCompiles);
#endif // (PK_COMPILE_GPU_PC != 0)

//----------------------------------------------------------------------------

#if (PK_COMPILE_GPU_XBOX_ONE != 0)
void	PopcornFXPlatform_XboxOne_UNKNOWN_SetupGPUBackendCompilers(	const PopcornFX::CString		&fileSourceVirtualPath,
																u32								backendTargets, // PopcornFX::EBackendTarget
																PopcornFXCbBuildBytecodeArray	&outCbCompiles);
#endif // (PK_COMPILE_GPU_XBOX_ONE != 0)

//----------------------------------------------------------------------------

#if (PK_COMPILE_GPU_UNKNOWN != 0)
void	PopcornFXPlatform_UNKNOWN1eries_SetupGPUBackendCompilers(	const PopcornFX::CString		&fileSourceVirtualPath,
																u32								backendTargets, // PopcornFX::EBackendTarget
																PopcornFXCbBuildBytecodeArray	&outCbCompiles);
#endif // (PK_COMPILE_GPU_UNKNOWN != 0)

//----------------------------------------------------------------------------

#if (PK_COMPILE_GPU_UNKNOWN2 != 0)
void	PopcornFXPlatform_UNKNOWN2_SetupGPUBackendCompilers(	const PopcornFX::CString		&fileSourceVirtualPath,
														u32								backendTargets, // PopcornFX::EBackendTarget
														PopcornFXCbBuildBytecodeArray	&outCbCompiles);
#endif // (PK_COMPILE_GPU_UNKNOWN2 != 0)

//----------------------------------------------------------------------------
