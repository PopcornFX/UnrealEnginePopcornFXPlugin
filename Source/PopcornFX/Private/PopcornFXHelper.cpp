//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "PopcornFXHelper.h"

#include "PopcornFXSDK.h"
#include <pk_maths/include/pk_maths_primitives.h>
#include <pk_kernel/include/kr_string_unicode.h>
#if WITH_EDITOR
#	include "Misc/MessageDialog.h"
#endif

//----------------------------------------------------------------------------

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

//----------------------------------------------------------------------------

PopcornFX::CString	ToPk(const FString &str)
{
	return TCHAR_TO_UTF8(*str);
}

//----------------------------------------------------------------------------

FString	ToUE(const PopcornFX::CString &str)
{
	return UTF8_TO_TCHAR(str.Data());
}

//----------------------------------------------------------------------------

FString	ToUE(const PopcornFX::CStringView &str)
{
	return UTF8_TO_TCHAR(str.Data());
}

//----------------------------------------------------------------------------

FString	ToUE(const PopcornFX::CStringUnicode &str)
{
	return FString((const UTF16CHAR*)str.Data());
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
EAppReturnType::Type	OpenMessageBox(EAppMsgCategory category, EAppMsgType::Type messageType, const FText& message, const FText& title)
{
	return FMessageDialog::Open(category, messageType, message, title);
}
#endif

//----------------------------------------------------------------------------
