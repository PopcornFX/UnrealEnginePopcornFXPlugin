//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
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
	AddErrorableProperty(PropertyHandle, "SamplingMode", ChildBuilder, true, false);

	AddErrorableProperty(PropertyHandle, "bAllowTextureConversionAtRuntime", ChildBuilder, true, false);

	AddErrorableProperty(PropertyHandle, "Texture", ChildBuilder, true, false);

	AddErrorableProperty(PropertyHandle, "TextureAtlas", ChildBuilder, true, false);

	AddErrorableProperty(PropertyHandle, "DensitySource", ChildBuilder, samplingMode != EPopcornFXImageSamplingMode::Type::Regular, true);

	AddErrorableProperty(PropertyHandle, "DensityPower", ChildBuilder, samplingMode != EPopcornFXImageSamplingMode::Type::Regular, true);
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
