//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsAttributeSamplerActor.h"

#include "PopcornFXAttributeSamplerActor.h"
#include "PopcornFXSDK.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXDetailsAttributeSamplerActor"

//----------------------------------------------------------------------------

FPopcornFXDetailsAttributeSamplerActor::FPopcornFXDetailsAttributeSamplerActor()
	: m_CachedDetailLayoutBuilder(null)
{

}

//----------------------------------------------------------------------------

TSharedRef<IDetailCustomization>	FPopcornFXDetailsAttributeSamplerActor::MakeInstance()
{
	return MakeShareable(new FPopcornFXDetailsAttributeSamplerActor);
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerActor::RebuildDetails()
{
	if (m_CachedDetailLayoutBuilder != null)
		m_CachedDetailLayoutBuilder->ForceRefreshDetails();
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerActor::CustomizeDetails(IDetailLayoutBuilder& detailLayout)
{
	TSharedRef<IPropertyHandle>	samplerComponentType = detailLayout.GetProperty("SamplerComponentType");
	TSharedRef<IPropertyHandle>	sampler = detailLayout.GetProperty("Sampler");

	m_CachedDetailLayoutBuilder = &detailLayout;
	if (samplerComponentType->IsValidHandle())
		samplerComponentType->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeSamplerActor::RebuildDetails));
	if (sampler->IsValidHandle())
		sampler->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeSamplerActor::RebuildDetails));
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
