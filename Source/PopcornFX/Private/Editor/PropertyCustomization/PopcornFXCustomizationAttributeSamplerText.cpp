//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR

#include "PopcornFXCustomizationAttributeSamplerText.h"

#include "PopcornFXAttributeSamplerText.h"

#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXCustomizationAttributeSamplerText, Log, All);

//----------------------------------------------------------------------------

/** Makes a new instance of this detail layout class for a specific detail view requesting it */
TSharedRef<IPropertyTypeCustomization>	FPopcornFXCustomizationAttributeSamplerText::MakeInstance()
{
	return MakeShareable(new FPopcornFXCustomizationAttributeSamplerText);
}

//----------------------------------------------------------------------------

void FPopcornFXCustomizationAttributeSamplerText::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder &ChildBuilder, IPropertyTypeCustomizationUtils &CustomizationUtils)
{
	if (!PropertyHandle->IsValidHandle())
	{
		return;
	}

	FPopcornFXCustomizationAttributeSampler::CustomizeChildren(PropertyHandle, ChildBuilder, CustomizationUtils);

	TSharedPtr<IPropertyHandle>	textPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesText, Text));
	ChildBuilder.AddProperty(textPty.ToSharedRef());

}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
