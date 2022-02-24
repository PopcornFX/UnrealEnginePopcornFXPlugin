//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "Styling/SlateStyle.h"

class FPopcornFXStyle
{
public:
	// Register PopcornFX styleset and its icons/brushes
	static void Initialize();

	// Unregister PopcornFX styleset and its icons/brushes
	static void Shutdown();

	static const FName			&GetStyleSetName() { check(m_StyleSet.IsValid()); return m_StyleSet->GetStyleSetName(); }
	static const FSlateBrush	*GetBrush(const FName PropertyName, const ANSICHAR* Specifier = nullptr) { check(m_StyleSet.IsValid()); return m_StyleSet->GetBrush(PropertyName, Specifier); }

private:
	FPopcornFXStyle() {}

	static FString						InContent(const FString& RelativePath, const ANSICHAR* Extension);

	static TSharedPtr<FSlateStyleSet>	m_StyleSet;
};

#endif // WITH_EDITOR
