//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"

#include "Misc/Build.h"
#include "HAL/Platform.h"

#include "Runtime/Launch/Resources/Version.h"
#if !defined(ENGINE_MAJOR_VERSION)
#	error no ENGINE_MAJOR_VERSION
#endif
#if (ENGINE_MAJOR_VERSION != 4)
#	error invalid ENGINE_MAJOR_VERSION
#endif
#if (ENGINE_MINOR_VERSION < 24)
#	error PopcornFX Plugin only supported in UE >= 4.24
#endif

// Can't use preprocessor defines for serialized classes. Define a dummy class for prior versions where this interface does not exist
// To remove once UE4.25 support is dropped
#if (ENGINE_MINOR_VERSION < 26)
class	IMovieSceneTrackTemplateProducer { };
#endif // (ENGINE_MINOR_VERSION < 26)

#if PLATFORM_WINDOWS
#	include "Windows/MinimalWindowsApi.h"
#	include "Windows/PreWindowsApi.h"
#	include "Windows/MinWindows.h"
#	include "Windows/PostWindowsApi.h"
#elif PLATFORM_XBOXONE
#	if (ENGINE_MINOR_VERSION >= 25)
#		include "XboxCommonPreApi.h"
#		include "XboxCommonMinApi.h"
#		include "XboxCommonPostApi.h"
#	else
#		include "XboxOnePreApi.h"
#		include "XboxOneMinApi.h"
#		include "XboxOnePostApi.h"
#	endif // (ENGINE_MINOR_VERSION >= 25)
#endif

#if PLATFORM_WINDOWS
#	ifdef WINDOWS_PLATFORM_TYPES_GUARD
#		include "Windows/HideWindowsPlatformTypes.h"
#	endif
#	include "Windows/AllowWindowsPlatformTypes.h"
#elif PLATFORM_XBOXONE
#	if (ENGINE_MINOR_VERSION >= 25)
#		ifdef XBOX_PLATFORM_TYPES_GUARD
#			include "XboxCommonHidePlatformTypes.h"
#		endif // XBOX_PLATFORM_TYPES_GUARD
#	else
#		ifdef XBOXONE_PLATFORM_TYPES_GUARD
#			include "XboxOneHidePlatformTypes.h"
#		endif // XBOXONE_PLATFORM_TYPES_GUARD
#	endif // (ENGINE_MINOR_VERSION >= 25)
#	if (ENGINE_MINOR_VERSION >= 25)
#		include "XboxCommonAllowPlatformTypes.h"
#	else
#		include "XboxOneAllowPlatformTypes.h"
#	endif // (ENGINE_MINOR_VERSION >= 25)
#endif

// TODO: Fix d3d11 header leak
#if ((PK_GPU_D3D11 == 1) || (PK_GPU_D3D12 == 1))
#	undef D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS
#	undef D3D11_ERROR_FILE_NOT_FOUND
#	undef D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS
#	undef D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD
#	undef D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS
#	undef D3D10_ERROR_FILE_NOT_FOUND
#endif

#ifdef GetObject
#	undef GetObject
#endif

#undef	PV_MODULE_INIT_NAME
#undef	PV_MODULE_NAME
#undef	PV_MODULE_SYM
#define	PV_MODULE_INIT_NAME	"UE4 PopcornFX Plugin"
#define	PV_MODULE_NAME		"UE4"
#define	PV_MODULE_SYM		UE4

#include <pkapi/include/pk_precompiled_default.h>

// disable non-default warning enabled by PopcornFX.
#if defined(_MSC_VER)
#	pragma warning(disable : 4061 4062)
#endif

PK_LOG_MODULE_DEFINE();

#undef PK_TODO
#define PK_TODO(...)
#undef PK_FIXME
#define PK_FIXME(...)

#ifdef PK_NULL_AS_VARIABLE
using PopcornFX:: null;
#endif

#if	!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#	define	POPCORNFX_RENDER_DEBUG			1
#else
#	define	POPCORNFX_RENDER_DEBUG			0
#endif

#define PK_V1	0

// using useful types unlikely to collide with UE types

using PopcornFX:: ureg;
using PopcornFX:: sreg;
using PopcornFX:: u64;
using PopcornFX:: s64;
using PopcornFX:: u32;
using PopcornFX:: u16;
using PopcornFX:: u8;
using PopcornFX:: s32;
using PopcornFX:: s16;
using PopcornFX:: s8;

using PopcornFX:: CQuaternion;
using PopcornFX:: CFloat4x4;
using PopcornFX:: CFloat4;
using PopcornFX:: CFloat3;
using PopcornFX:: CFloat2;
using PopcornFX:: CFloat1;
using PopcornFX:: CInt4;
using PopcornFX:: CInt3;
using PopcornFX:: CInt2;
using PopcornFX:: CInt1;
using PopcornFX:: CUint4;
using PopcornFX:: CUint3;
using PopcornFX:: CUint2;
using PopcornFX:: CUint1;
using PopcornFX:: CBool1;
using PopcornFX:: CBool2;
using PopcornFX:: CBool3;
using PopcornFX:: CBool4;

using PopcornFX:: TMemoryView;
using PopcornFX:: TStridedMemoryView;
using PopcornFX:: TStridedMemoryViewWithFootprint;

// /!\ at least those types collides:
// TArray TRefPtr TWeakPtr

// FastDelegate warning about reinterpret_cast
#if PLATFORM_WINDOWS || PLATFORM_XBOXONE
#	pragma warning( disable : 4191 )
#endif

#include "PopcornFXMinimal.h"
#include "PopcornFXHelper.h"
