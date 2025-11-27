//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR

#include "PopcornFXCustomizationAttributeSamplerImage.h"

#include "PopcornFXAttributeSamplerImage.h"

#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXCustomizationAttributeSamplerImage, Log, All);

//----------------------------------------------------------------------------

/** Makes a new instance of this detail layout class for a specific detail view requesting it */
TSharedRef<IPropertyTypeCustomization>	FPopcornFXCustomizationAttributeSamplerImage::MakeInstance()
{
	return MakeShareable(new FPopcornFXCustomizationAttributeSamplerImage);
}

//----------------------------------------------------------------------------

void FPopcornFXCustomizationAttributeSamplerImage::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder &ChildBuilder, IPropertyTypeCustomizationUtils &CustomizationUtils)
{
	if (!PropertyHandle->IsValidHandle())
	{
		return;
	}

	FPopcornFXCustomizationAttributeSampler::CustomizeChildren(PropertyHandle, ChildBuilder, CustomizationUtils);

	TSharedPtr<IPropertyHandle>	currentProperty;

	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, SamplingMode));
	uint8	samplingMode;
	currentProperty->GetValue(samplingMode);
	// Modifying sampling mode implies hiding/showing DensitySource and DensityPower so refresh the panel
	currentProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSamplerImage::RebuildProperties));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef());

	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, bAllowTextureConversionAtRuntime));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef());

	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, Texture));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef());

	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, TextureAtlas));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef());

	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, DensitySource));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef()).
		EditCondition(samplingMode != EPopcornFXImageSamplingMode::Type::Regular, FOnBooleanValueChanged()).EditConditionHides(true);

	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, DensityPower));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef()).
		EditCondition(samplingMode != EPopcornFXImageSamplingMode::Type::Regular, FOnBooleanValueChanged()).EditConditionHides(true);
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
