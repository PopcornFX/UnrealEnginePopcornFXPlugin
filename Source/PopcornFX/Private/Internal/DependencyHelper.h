//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "PopcornFXSDK.h"
#include <pk_kernel/include/kr_delegates.h>

FWD_PK_API_BEGIN
namespace HBO
{
	class CFieldDefinition;
}
FWD_PK_API_END
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

namespace PopcornFX
{
	typedef FastDelegate<void(const PopcornFX::CString &value, const PopcornFX::CBaseObject &hbo, u32 fieldIndex)> CbDependency;
	void			GatherDependencies(const PopcornFX::CString &path, const CbDependency &cb);
} // namespace PopcornFX

#endif // WITH_EDITOR
