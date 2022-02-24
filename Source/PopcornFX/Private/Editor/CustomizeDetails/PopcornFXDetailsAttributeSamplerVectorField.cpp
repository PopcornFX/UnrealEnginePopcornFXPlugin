//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsAttributeSamplerVectorField.h"
#include "PopcornFXAttributeSamplerVectorField.h"
#include "PopcornFXSDK.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"

//----------------------------------------------------------------------------

FPopcornFXDetailsAttributeSamplerVectorField::FPopcornFXDetailsAttributeSamplerVectorField()
:	m_CachedDetailLayoutBuilder(null)
{

}

//----------------------------------------------------------------------------

TSharedRef<IDetailCustomization>	FPopcornFXDetailsAttributeSamplerVectorField::MakeInstance()
{
	return MakeShareable(new FPopcornFXDetailsAttributeSamplerVectorField);
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerVectorField::RebuildDetails()
{
	if (!PK_VERIFY(m_CachedDetailLayoutBuilder != null))
		return;
	m_CachedDetailLayoutBuilder->ForceRefreshDetails();
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerVectorField::CustomizeDetails(IDetailLayoutBuilder &detailLayout)
{
	TSharedRef<IPropertyHandle>	boundsSource = detailLayout.GetProperty("BoundsSource");
	IDetailCategoryBuilder		&detailCategory = detailLayout.EditCategory("PopcornFX AttributeSampler");

	m_CachedDetailLayoutBuilder = &detailLayout;
	if (boundsSource->IsValidHandle())
	{
		uint8	value;

		boundsSource->GetValue(value);
		switch (value)
		{
		case	EPopcornFXVectorFieldBounds::Source:
			detailLayout.HideProperty("VolumeDimensions");
			break;
		default:
			break;
		}
		boundsSource->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeSamplerVectorField::RebuildDetails));
	}
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
