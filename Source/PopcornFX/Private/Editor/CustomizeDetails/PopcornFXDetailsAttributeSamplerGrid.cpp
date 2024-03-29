//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsAttributeSamplerGrid.h"
#include "PopcornFXAttributeSamplerGrid.h"
#include "PopcornFXSDK.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"

//----------------------------------------------------------------------------

FPopcornFXDetailsAttributeSamplerGrid::FPopcornFXDetailsAttributeSamplerGrid()
:	m_CachedDetailLayoutBuilder(null)
{

}

//----------------------------------------------------------------------------

TSharedRef<IDetailCustomization>	FPopcornFXDetailsAttributeSamplerGrid::MakeInstance()
{
	return MakeShareable(new FPopcornFXDetailsAttributeSamplerGrid);
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerGrid::RebuildDetails()
{
	if (!PK_VERIFY(m_CachedDetailLayoutBuilder != null))
		return;
	m_CachedDetailLayoutBuilder->ForceRefreshDetails();
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerGrid::CustomizeDetails(IDetailLayoutBuilder &detailLayout)
{
	TSharedRef<IPropertyHandle>	isAssetGrid = detailLayout.GetProperty("bAssetGrid");
	IDetailCategoryBuilder		&detailCategory = detailLayout.EditCategory("PopcornFX AttributeSampler");

	m_CachedDetailLayoutBuilder = &detailLayout;
	if (isAssetGrid->IsValidHandle())
	{
		bool	_isAssetGrid = false;

		isAssetGrid->GetValue(_isAssetGrid);
		if (_isAssetGrid)
		{
			detailLayout.HideProperty("SizeX");
			detailLayout.HideProperty("SizeY");
			detailLayout.HideProperty("SizeZ");
			detailLayout.HideProperty("DataType");
		}
		else
		{
			detailLayout.HideProperty("RenderTarget");
		}
		isAssetGrid->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeSamplerGrid::RebuildDetails));
	}

}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
