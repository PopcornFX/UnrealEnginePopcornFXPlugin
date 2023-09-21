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

#if (ENGINE_MAJOR_VERSION != 4 && ENGINE_MAJOR_VERSION != 5)
#	error invalid ENGINE_MAJOR_VERSION
#endif
#if (ENGINE_MAJOR_VERSION == 4)
#	if (ENGINE_MINOR_VERSION < 27)
#		error PopcornFX Plugin only supported in UE4 >= 4.27
#	endif
#endif // (ENGINE_MAJOR_VERSION == 4)

#if (ENGINE_MAJOR_VERSION == 5)
#	ifndef PLATFORM_XBOXONE
#		define PLATFORM_XBOXONE	0
#	endif
#	if (ENGINE_MINOR_VERSION < 1)
#		error PopcornFX Plugin only supported in UE5 >= 5.1
#	endif
#endif // (ENGINE_MAJOR_VERSION == 5)

#if PLATFORM_WINDOWS
#	include "Windows/MinimalWindowsApi.h"
#	include "Windows/PreWindowsApi.h"
#	include "Windows/MinWindows.h"
#	include "Windows/PostWindowsApi.h"
#elif PLATFORM_XBOXONE
#	include "XboxCommonPreApi.h"
#	include "XboxCommonMinApi.h"
#	include "XboxCommonPostApi.h"
#endif

#if PLATFORM_WINDOWS
#	ifdef WINDOWS_PLATFORM_TYPES_GUARD
#		include "Windows/HideWindowsPlatformTypes.h"
#	endif
#	include "Windows/AllowWindowsPlatformTypes.h"
#elif PLATFORM_XBOXONE
#	ifdef XBOX_PLATFORM_TYPES_GUARD
#		include "XboxCommonHidePlatformTypes.h"
#	endif // XBOX_PLATFORM_TYPES_GUARD
#	include "XboxCommonAllowPlatformTypes.h"
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
#define	PV_MODULE_INIT_NAME	"UnrealEngine PopcornFX Plugin"
#define	PV_MODULE_NAME		"UE"
#define	PV_MODULE_SYM		UE

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
