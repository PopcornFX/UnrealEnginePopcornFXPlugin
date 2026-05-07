//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PopcornFXSDK.h"
#include <pk_maths/include/pk_maths.h>
#include <pk_kernel/include/kr_base_types.h>

template <typename _Type, uint32 _Dim>
struct TraitToVectorOrScalarIFN
{
	typedef	PopcornFX::TVector<_Type, _Dim>	Type;
};

template <typename _Type>
struct TraitToVectorOrScalarIFN<_Type, 1>
{
	typedef	_Type	Type;
};

template <typename _Type, uint32 _Dim>
struct TraitVectorBase : public PopcornFX::CBaseType<typename TraitToVectorOrScalarIFN<_Type, _Dim>::Type>
{
};

template <typename _Type, uint32 _Dim>
struct TraitVector;

#define GENERATE_VECTOR_TRAIT(__type, __dim, __ue)		\
template <> struct TraitVector<__type, __dim> : public TraitVectorBase<__type, __dim> { typedef __ue UEType; };

GENERATE_VECTOR_TRAIT(float, 1, float)
GENERATE_VECTOR_TRAIT(float, 2, FVector2D)
GENERATE_VECTOR_TRAIT(float, 3, FVector)
GENERATE_VECTOR_TRAIT(float, 4, FLinearColor)
GENERATE_VECTOR_TRAIT(int32, 1, int32)
GENERATE_VECTOR_TRAIT(int32, 2, FIntPoint)
GENERATE_VECTOR_TRAIT(int32, 3, FIntVector)
//GENERATE_VECTOR_TRAIT(uint32, 4, uint32)

#undef GENERATE_VECTOR_TRAIT
