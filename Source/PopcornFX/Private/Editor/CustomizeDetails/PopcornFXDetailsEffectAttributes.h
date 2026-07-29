//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "IDetailCustomization.h"
#include "Runtime/Launch/Resources/Version.h"
#include "PropertyEditorModule.h"

#include "IDetailCustomization.h"

class IDetailCategoryBuilder;

class FPopcornFXDetailsEffectAttributes : public IDetailCustomization
{
public:

	virtual void	CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;

protected:

	IDetailCategoryBuilder	*m_AttributeListCategory;
};

#endif // WITH_EDITOR
