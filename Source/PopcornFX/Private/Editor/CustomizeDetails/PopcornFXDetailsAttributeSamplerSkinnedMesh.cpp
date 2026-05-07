//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsAttributeSamplerSkinnedMesh.h"

#include "PopcornFXAttributeSamplerSkinnedMesh.h"
#include "PopcornFXSDK.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXDetailsAttributeSamplerSkinnedMesh"

//----------------------------------------------------------------------------

FPopcornFXDetailsAttributeSamplerSkinnedMesh::FPopcornFXDetailsAttributeSamplerSkinnedMesh()
	: m_CachedDetailLayoutBuilder(null)
{

}

//----------------------------------------------------------------------------

TSharedRef<IDetailCustomization>	FPopcornFXDetailsAttributeSamplerSkinnedMesh::MakeInstance()
{
	return MakeShareable(new FPopcornFXDetailsAttributeSamplerSkinnedMesh);
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerSkinnedMesh::RebuildDetails()
{
	if (!PK_VERIFY(m_CachedDetailLayoutBuilder != null))
		return;
	m_CachedDetailLayoutBuilder->ForceRefreshDetails();
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerSkinnedMesh::CustomizeDetails(IDetailLayoutBuilder &detailLayout)
{
	TSharedRef<IPropertyHandle>	meshSamplingMode = detailLayout.GetProperty("MeshSamplingMode");
	IDetailCategoryBuilder		&detailCategory = detailLayout.EditCategory("PopcornFX AttributeSampler");

	m_CachedDetailLayoutBuilder = &detailLayout;
	if (meshSamplingMode->IsValidHandle())
	{
		uint8	meshSamplingModeValue;

		meshSamplingMode->GetValue(meshSamplingModeValue);

		if (meshSamplingModeValue != EPopcornFXMeshSamplingMode::Weighted)
			detailLayout.HideProperty("DensityColorChannel");

		meshSamplingMode->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeSamplerSkinnedMesh::RebuildDetails));
	}
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
