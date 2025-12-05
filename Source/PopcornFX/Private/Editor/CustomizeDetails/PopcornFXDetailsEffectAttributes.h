//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "PopcornFXDetailsAttributeList.h"
#include "Runtime/Launch/Resources/Version.h"
#include "PropertyEditorModule.h"

#include "IDetailCustomization.h"

class UPopcornFXEmitterComponent;
class UPopcornFXAttributeList;
class UPopcornFXEffect;
class IDetailGroup;
class IPropertyHandleArray;
class IDetailCategoryBuilder;
struct FPopcornFXAttributeDesc;
struct FPopcornFXSamplerDesc;

class FPopcornFXDetailsEffectAttributes : public FPopcornFXDetailsAttributeList
{
public:

	virtual void	CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;

private:
	void			BuildSampler(const FPopcornFXSamplerDesc *desc, const TSharedPtr<IPropertyHandle> samplerPty, const TSharedPtr<IPropertyHandle> samplerDescPty, const UPopcornFXAttributeList *attrList, uint32 sampleri, uint32 iCategory) override;
};

#endif // WITH_EDITOR
