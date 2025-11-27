//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "PropertyEditorModule.h"
#include "IPropertyTypeCustomization.h"

#include "Editor/PropertyCustomization/PopcornFXCustomizationAttributeSampler.h"

class FPopcornFXCustomizationAttributeSamplerGrid : public FPopcornFXCustomizationAttributeSampler
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization>	MakeInstance();

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder &ChildBuilder, IPropertyTypeCustomizationUtils &CustomizationUtils) override;
};

#endif // WITH_EDITOR
