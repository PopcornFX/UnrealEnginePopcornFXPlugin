//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include <PopcornFXTypes.h>

#include <pk_maths/include/pk_maths.h>
#include <pk_kernel/include/kr_base_types.h>


namespace	EPopcornFXPayloadType
{
	Type	FromPopcornFXBaseTypeID(const PopcornFX::EBaseTypeID typeID)
	{
		switch (typeID)
		{
		case	PopcornFX::BaseType_Bool:
			return Type::Bool;
		case	PopcornFX::BaseType_Bool2:
			return Type::Bool2;
		case	PopcornFX::BaseType_Bool3:
			return Type::Bool3;
		case	PopcornFX::BaseType_Bool4:
			return Type::Bool4;
		case	PopcornFX::BaseType_I32:
			return Type::Int;
		case	PopcornFX::BaseType_Int2:
			return Type::Int2;
		case	PopcornFX::BaseType_Int3:
			return Type::Int3;
		case	PopcornFX::BaseType_Int4:
			return Type::Int4;
		case	PopcornFX::BaseType_Float:
		case	PopcornFX::BaseType_Double:
			return Type::Float;
		case	PopcornFX::BaseType_Float2:
			return Type::Float2;
		case	PopcornFX::BaseType_Float3:
			return Type::Float3;
		case	PopcornFX::BaseType_Float4:
		case	PopcornFX::BaseType_Quaternion:
			return Type::Float4;
		default:
			PK_ASSERT_NOT_REACHED();
			return Type::Invalid;
		}
	}
}
