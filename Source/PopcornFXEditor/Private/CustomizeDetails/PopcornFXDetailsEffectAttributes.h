//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "PopcornFXDetailsAttributeList.h"
#include "Runtime/Launch/Resources/Version.h"
#include "PropertyEditorModule.h"

#include "IDetailCustomization.h"

class UPopcornFXAttributeList;

class FPopcornFXDetailsEffectAttributes : public FPopcornFXDetailsAttributeList
{
public:

	virtual void	CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;

private:
	void			BuildSampler(const FPopcornFXSamplerDesc *desc, const TSharedPtr<IPropertyHandle> samplerPty, const TSharedPtr<IPropertyHandle> samplerDescPty, const UPopcornFXAttributeList *attrList, uint32 sampleri, uint32 iCategory) override;
};

#endif // WITH_EDITOR
