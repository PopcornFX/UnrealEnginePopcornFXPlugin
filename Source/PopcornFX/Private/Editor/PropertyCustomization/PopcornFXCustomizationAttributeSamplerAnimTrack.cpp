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

	AddErrorableProperty(PropertyHandle, "TargetActor", ChildBuilder, true, false);
	AddErrorableProperty(PropertyHandle, "SplineComponentName", ChildBuilder, true, false);
	AddErrorableProperty(PropertyHandle, "bTranslate", ChildBuilder, true, false);
	AddErrorableProperty(PropertyHandle, "bRotate", ChildBuilder, true, false);
	AddErrorableProperty(PropertyHandle, "bScale", ChildBuilder, true, false);
	AddErrorableProperty(PropertyHandle, "bFastSampler", ChildBuilder, true, false);
	AddErrorableProperty(PropertyHandle, "bEditorRebuildEachFrame", ChildBuilder, true, false);
	AddErrorableProperty(PropertyHandle, "Transforms", ChildBuilder, true, false);
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
