//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#if WITH_EDITOR

#include "PopcornFXCustomizationAttributeSamplerVectorField.h"

#include "PopcornFXAttributeSamplerVectorField.h"

#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXCustomizationAttributeSamplerVectorField, Log, All);

//----------------------------------------------------------------------------

/** Makes a new instance of this detail layout class for a specific detail view requesting it */
TSharedRef<IPropertyTypeCustomization>	FPopcornFXCustomizationAttributeSamplerVectorField::MakeInstance()
{
	return MakeShareable(new FPopcornFXCustomizationAttributeSamplerVectorField);
}

//----------------------------------------------------------------------------

void FPopcornFXCustomizationAttributeSamplerVectorField::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder &ChildBuilder, IPropertyTypeCustomizationUtils &CustomizationUtils)
{
	if (!PropertyHandle->IsValidHandle())
	{
		return;
	}

	FPopcornFXCustomizationAttributeSampler::CustomizeChildren(PropertyHandle, ChildBuilder, CustomizationUtils);

	TSharedPtr<IPropertyHandle>	currentPty;

	AddErrorableProperty(PropertyHandle, "VectorField", ChildBuilder, true, false);

	TSharedPtr<IPropertyHandle>	boundsSourcePty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, BoundsSource));
	uint8	boundsSourceValue;
	boundsSourcePty->GetValue(boundsSourceValue);
	boundsSourcePty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSamplerVectorField::RebuildProperties));
	
	AddErrorableProperty(PropertyHandle, "Intensity", ChildBuilder, true, false);

	AddErrorableProperty(PropertyHandle, "RotationAnimation", ChildBuilder, true, false);

	AddErrorableProperty(PropertyHandle, "WrapMode", ChildBuilder, true, false);

	AddErrorableProperty(PropertyHandle, "SamplingMode", ChildBuilder, true, false);

	AddErrorableProperty(PropertyHandle, "BoundsSource", ChildBuilder, true, false);

	AddErrorableProperty(PropertyHandle, "VolumeDimensions", ChildBuilder, boundsSourceValue != EPopcornFXVectorFieldBounds::Source, true);

	AddErrorableProperty(PropertyHandle, "bUseRelativeTransform", ChildBuilder, true, false);

#if 0
	/** Enable to draw individual vectorfield cells. */
	AddErrorableProperty(PropertyHandle, "bDrawCells", ChildBuilder, true, false);
#endif
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
