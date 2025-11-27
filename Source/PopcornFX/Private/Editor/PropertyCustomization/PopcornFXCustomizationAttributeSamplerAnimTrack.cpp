//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR

#include "PopcornFXCustomizationAttributeSamplerAnimTrack.h"

#include "PopcornFXAttributeSamplerAnimTrack.h"

#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXCustomizationAttributeSamplerAnimTrack, Log, All);

//----------------------------------------------------------------------------

/** Makes a new instance of this detail layout class for a specific detail view requesting it */
TSharedRef<IPropertyTypeCustomization>	FPopcornFXCustomizationAttributeSamplerAnimTrack::MakeInstance()
{
	return MakeShareable(new FPopcornFXCustomizationAttributeSamplerAnimTrack);
}

//----------------------------------------------------------------------------

void FPopcornFXCustomizationAttributeSamplerAnimTrack::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder &ChildBuilder, IPropertyTypeCustomizationUtils &CustomizationUtils)
{
	if (!PropertyHandle->IsValidHandle())
	{
		return;
	}

	FPopcornFXCustomizationAttributeSampler::CustomizeChildren(PropertyHandle, ChildBuilder, CustomizationUtils);

	TSharedPtr<IPropertyHandle>	currentProperty;

	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, TargetActor));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef());
	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, SplineComponentName));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef());
	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, bTranslate));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef());
	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, bRotate));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef());
	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, bScale));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef());
	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, bFastSampler));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef());
	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, bEditorRebuildEachFrame));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef());
	currentProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, Transforms));
	ChildBuilder.AddProperty(currentProperty.ToSharedRef());
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
