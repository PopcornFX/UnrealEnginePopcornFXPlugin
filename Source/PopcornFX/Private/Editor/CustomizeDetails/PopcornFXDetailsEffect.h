//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "IDetailCustomization.h"

class FPopcornFXEffectEditor;

class FPopcornFXDetailsEffect : public IDetailCustomization
{
public:
	virtual void	CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;
};

#endif // WITH_EDITOR
