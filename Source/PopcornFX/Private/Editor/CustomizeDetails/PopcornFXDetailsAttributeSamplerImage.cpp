//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsAttributeSamplerImage.h"
#include "PopcornFXAttributeSamplerImage.h"
#include "PopcornFXSDK.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"

//----------------------------------------------------------------------------

FPopcornFXDetailsAttributeSamplerImage::FPopcornFXDetailsAttributeSamplerImage()
	: m_CachedDetailLayoutBuilder(null)
{

}

//----------------------------------------------------------------------------

TSharedRef<IDetailCustomization>	FPopcornFXDetailsAttributeSamplerImage::MakeInstance()
{
	return MakeShareable(new FPopcornFXDetailsAttributeSamplerImage);
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerImage::RebuildDetails()
{
	if (!PK_VERIFY(m_CachedDetailLayoutBuilder != null))
		return;
	m_CachedDetailLayoutBuilder->ForceRefreshDetails();
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerImage::CustomizeDetails(IDetailLayoutBuilder &detailLayout)
{
	TSharedRef<IPropertyHandle>	samplingMode = detailLayout.GetProperty("SamplingMode");
	IDetailCategoryBuilder		&detailCategory = detailLayout.EditCategory("PopcornFX AttributeSampler");

	m_CachedDetailLayoutBuilder = &detailLayout;
	if (samplingMode->IsValidHandle())
	{
		uint8	value;

		samplingMode->GetValue(value);
		switch (value)
		{
		case	EPopcornFXImageSamplingMode::Regular:
			detailLayout.HideProperty("DensitySource");
			detailLayout.HideProperty("DensityPower");
			break;
		case	EPopcornFXImageSamplingMode::Both:
		case	EPopcornFXImageSamplingMode::Density:
			break;
			break;
		default:
			break;
		}
		samplingMode->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeSamplerImage::RebuildDetails));
	}
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
