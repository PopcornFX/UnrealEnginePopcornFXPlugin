//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

UENUM(BlueprintType)
namespace	EPopcornFXParticleFieldType
{
	enum	Type
	{
		Float,
		Float2,
		Float3,
		Float4,
		Int,
		Int2,
		Int3,
		Int4,
	};
}
namespace	EPopcornFXParticleFieldType
{
	inline uint32		Dim(Type type) { return (uint32(type) % 4) + 1; }
}

UENUM(BlueprintType)
namespace	EPopcornFXPayloadType
{
	enum	Type
	{
		Bool,
		Bool2,
		Bool3,
		Bool4,
		Float,
		Float2,
		Float3,
		Float4,
		Int,
		Int2,
		Int3,
		Int4,
		Quaternion = Int4 + 4, // float4

		Invalid
	};
}

UENUM(BlueprintType)
namespace	EPopcornFXPinDataType
{
	enum	Type
	{
		Float,
		Float2,
		Float3,
		Float4,
		Int,
		Int2,
		Int3,
		Int4,
		Bool,
		Bool2,
		Bool3,
		Bool4,
		Vector2D UMETA(DisplayName="Vector2D (Float2)"),
		Vector UMETA(DisplayName="Vector (Float3)"),
		LinearColor UMETA(DisplayName="LinearColor (Float4)"),
		Rotator,
	};
}
