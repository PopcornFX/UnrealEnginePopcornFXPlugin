//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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

	currentPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, VectorField));
	ChildBuilder.AddProperty(currentPty.ToSharedRef());

	TSharedPtr<IPropertyHandle>	boundsSourcePty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, BoundsSource));
	uint8	boundsSourceValue;
	boundsSourcePty->GetValue(boundsSourceValue);
	boundsSourcePty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSamplerVectorField::RebuildProperties));
	
	currentPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, Intensity));
	ChildBuilder.AddProperty(currentPty.ToSharedRef());

	currentPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, RotationAnimation));
	ChildBuilder.AddProperty(currentPty.ToSharedRef());

	currentPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, WrapMode));
	ChildBuilder.AddProperty(currentPty.ToSharedRef());

	currentPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, SamplingMode));
	ChildBuilder.AddProperty(currentPty.ToSharedRef());

	ChildBuilder.AddProperty(boundsSourcePty.ToSharedRef());

	TSharedPtr<IPropertyHandle>	volumeDimensionPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, VolumeDimensions));
	ChildBuilder.AddProperty(volumeDimensionPty.ToSharedRef()).
		EditCondition(boundsSourceValue != EPopcornFXVectorFieldBounds::Source, FOnBooleanValueChanged()).
		EditConditionHides(true);

	currentPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, bUseRelativeTransform));
	ChildBuilder.AddProperty(currentPty.ToSharedRef());

#if 0
	/** Enable to draw individual vectorfield cells. */
	currentPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, bDrawCells));
	ChildBuilder.AddProperty(currentPty.ToSharedRef());
#endif
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
