//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "Math/Vector.h"
#include "Math/Vector4.h"
#include "Math/Vector2D.h"
#include "Math/Matrix.h"
#include "Math/IntPoint.h"
#include "Math/Color.h"
#include "Math/Box.h"

#include "PopcornFXSDK.h"
#include <pk_maths/include/pk_maths_primitives.h>

#ifndef POPCORNFX_UE_PROFILER_COLOR
	#define POPCORNFX_UE_PROFILER_COLOR			CFloat3(1.0f, 0.5f, 0.0f)
#endif

#ifndef POPCORNFX_UE_PROFILER_FAST_COLOR
	#define POPCORNFX_UE_PROFILER_FAST_COLOR	CFloat3(1.0f, 0.0f, 0.0f)
#endif

template <typename _OutType, typename _InType>
PK_FORCEINLINE const _OutType		&_Reinterpret(const _InType &vec)
{
	PK_STATIC_ASSERT(sizeof(_InType) == sizeof(_OutType));
	return *reinterpret_cast<const _OutType*>(&vec);
}

PK_FORCEINLINE const CFloat4		&ToPk(const FLinearColor &vec) { return _Reinterpret<CFloat4>(vec); }
PK_FORCEINLINE const CInt2			&ToPk(const FIntPoint& point) { return _Reinterpret<CInt2>(point); }

// UE5 LWC: Keep reinterpret casts whenever possible
PK_FORCEINLINE const FVector3f		&ToUE(const CFloat3 &vec) { return _Reinterpret<FVector3f>(vec); }
PK_FORCEINLINE const FVector4f		&ToUE(const CFloat4 &vec) { return _Reinterpret<FVector4f>(vec); }

PK_FORCEINLINE const CFloat3		&ToPk(const FVector3f &vec) { return _Reinterpret<CFloat3>(vec); }
PK_FORCEINLINE const CFloat2		&ToPk(const FVector2f &vec) { return _Reinterpret<CFloat2>(vec); }
PK_FORCEINLINE const CFloat4x4		&ToPk(const FMatrix44f &mat) { return _Reinterpret<CFloat4x4>(mat); }
PK_FORCEINLINE const CFloat4		&ToPk(const FVector4f &vec) { return _Reinterpret<CFloat4>(vec); }

PK_FORCEINLINE const FQuat4f		&ToUE(const CQuaternion& quat) { return *reinterpret_cast<const FQuat4f*>(&quat); }
PK_FORCEINLINE const CQuaternion	&ToPk(const FQuat4f &quat) { return *reinterpret_cast<const CQuaternion*>(&quat); }

#if (ENGINE_MAJOR_VERSION == 5)
// Copies when double types are provided
PK_FORCEINLINE CFloat3			ToPk(const FVector &vec) { return _Reinterpret<CFloat3>(FVector3f(vec)); }
PK_FORCEINLINE CFloat2			ToPk(const FVector2D &vec) { return _Reinterpret<CFloat2>(FVector2f(vec)); }
PK_FORCEINLINE CFloat4x4		ToPk(const FMatrix &mat) { return _Reinterpret<CFloat4x4>(FMatrix44f(mat)); }
PK_FORCEINLINE CFloat4			ToPk(const FVector4 &vec) { return _Reinterpret<CFloat4>(FVector4f(vec)); }

PK_FORCEINLINE CQuaternion		ToPk(const FQuat &quat) { return _Reinterpret<CQuaternion>(FQuat4f(quat)); }
PK_FORCEINLINE FBox				ToUE(const PopcornFX::CAABB &bounds)
{
	return FBox(ToUE(bounds.Min()), ToUE(bounds.Max()));
}
PK_FORCEINLINE PopcornFX::CAABB	ToPk(const FBox &bounds)
{
	return PopcornFX::CAABB(ToPk(bounds.Min), ToPk(bounds.Max));
}
PK_FORCEINLINE PopcornFX::CAABB	ToPk(const FBox3f &bounds)
{
	return PopcornFX::CAABB(ToPk(bounds.Min), ToPk(bounds.Max));
}
#else
PK_FORCEINLINE FBox3f			ToUE(const PopcornFX::CAABB &bounds)
{
	return FBox3f(ToUE(bounds.Min()), ToUE(bounds.Max()));
}
PK_FORCEINLINE PopcornFX::CAABB	ToPk(const FBox3f &bounds)
{
	return PopcornFX::CAABB(ToPk(bounds.Min), ToPk(bounds.Max));
}
#endif // (ENGINE_MAJOR_VERSION == 5)

PK_FORCEINLINE const FMatrix44f		&ToUE(const CFloat4x4 &mat) { return _Reinterpret<FMatrix44f>(mat); }

PK_FORCEINLINE CFloat4x4	&ToPkRef(FMatrix44f &matrix)
{
	PK_STATIC_ASSERT(sizeof(FMatrix44f) == sizeof(CFloat4x4));
	return *reinterpret_cast<CFloat4x4*>(&matrix);
}

PK_FORCEINLINE CFloat3		&ToPkRef(FVector3f &vec)
{
	PK_STATIC_ASSERT(sizeof(FVector3f) == sizeof(CFloat3));
	return *reinterpret_cast<CFloat3*>(&vec);
}

float					HumanReadF(float v, u32 base = 1024);
const TCHAR				*HumanReadS(float v, u32 base = 1024);
static inline float		SafeRcp(float v) { return v == 0.f ? 0.f : 1.f / v; }
