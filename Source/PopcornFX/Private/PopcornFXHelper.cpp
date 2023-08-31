//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXHelper.h"

#include "PopcornFXSDK.h"
#include <pk_maths/include/pk_maths_primitives.h>
#if WITH_EDITOR
#	include "Misc/MessageDialog.h"
#endif

static const TCHAR	*kHumanStr[] = {
	TEXT(" "), TEXT("k"), TEXT("m"), TEXT("g"), TEXT("t")
};

float		HumanReadF(float v, u32 base)
{
	u32		i = 0;
	while (v > base && i < PK_ARRAY_COUNT(kHumanStr) - 1)
	{
		++i;
		v /= base;
	}
	return v;
}

const TCHAR	*HumanReadS(float v, u32 base)
{
	u32		i = 0;
	while (v > base && i < PK_ARRAY_COUNT(kHumanStr) - 1)
	{
		++i;
		v /= base;
	}
	return kHumanStr[i];
}

#if WITH_EDITOR
EAppReturnType::Type	OpenMessageBox(EAppMsgType::Type messageType, const FText& message, const FText& title)
{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	return FMessageDialog::Open(messageType, message, title);
#else
	return FMessageDialog::Open(messageType, message, &title);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
}
#endif
