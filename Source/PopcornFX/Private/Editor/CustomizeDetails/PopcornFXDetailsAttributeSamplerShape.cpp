//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsAttributeSamplerShape.h"

#include "PopcornFXAttributeSamplerShape.h"
#include "PopcornFXSDK.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXDetailsAttributeSamplerShape"

//----------------------------------------------------------------------------

FPopcornFXDetailsAttributeSamplerShape::FPopcornFXDetailsAttributeSamplerShape()
	: m_CachedDetailLayoutBuilder(null)
{

}

//----------------------------------------------------------------------------

TSharedRef<IDetailCustomization>	FPopcornFXDetailsAttributeSamplerShape::MakeInstance()
{
	return MakeShareable(new FPopcornFXDetailsAttributeSamplerShape);
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerShape::RebuildDetails()
{
	if (!PK_VERIFY(m_CachedDetailLayoutBuilder != null))
		return;
	m_CachedDetailLayoutBuilder->ForceRefreshDetails();
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerShape::CustomizeDetails(IDetailLayoutBuilder& detailLayout)
{
	TSharedRef<IPropertyHandle>	shapeType = detailLayout.GetProperty("ShapeType");
	TSharedRef<IPropertyHandle>	shapeSamplingMode = detailLayout.GetProperty("ShapeSamplingMode");
	IDetailCategoryBuilder		&detailCategory = detailLayout.EditCategory("PopcornFX AttributeSampler");

	m_CachedDetailLayoutBuilder = &detailLayout;
	if (shapeType->IsValidHandle() && shapeSamplingMode->IsValidHandle())
	{
		uint8	value;
		uint8	shapeSamplingModeValue;

		shapeType->GetValue(value);
		shapeSamplingMode->GetValue(shapeSamplingModeValue);

		switch (value)
		{
		case	EPopcornFXAttribSamplerShapeType::Box:
			detailLayout.HideProperty("Radius");
			detailLayout.HideProperty("InnerRadius");
			detailLayout.HideProperty("Height");
			detailLayout.HideProperty("StaticMesh");
			detailLayout.HideProperty("StaticMeshSubIndex");
			detailLayout.HideProperty("Shapes");
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
			detailLayout.HideProperty("CollectionSamplingHeuristic");
			detailLayout.HideProperty("CollectionUseShapeWeights");
#endif
			detailLayout.HideProperty("ShapeSamplingMode");
			detailLayout.HideProperty("DensityColorChannel");
			break;
		case	EPopcornFXAttribSamplerShapeType::ComplexEllipsoid:
		case	EPopcornFXAttribSamplerShapeType::Sphere:
			detailLayout.HideProperty("BoxDimension");
			detailLayout.HideProperty("Height");
			detailLayout.HideProperty("StaticMesh");
			detailLayout.HideProperty("StaticMeshSubIndex");
			detailLayout.HideProperty("Shapes");
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
			detailLayout.HideProperty("CollectionSamplingHeuristic");
			detailLayout.HideProperty("CollectionUseShapeWeights");
#endif
			detailLayout.HideProperty("ShapeSamplingMode");
			detailLayout.HideProperty("DensityColorChannel");
			break;
		case	EPopcornFXAttribSamplerShapeType::Mesh:
			detailLayout.HideProperty("BoxDimension");
			detailLayout.HideProperty("Radius");
			detailLayout.HideProperty("InnerRadius");
			detailLayout.HideProperty("Height");
			detailLayout.HideProperty("Shapes");
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
			detailLayout.HideProperty("CollectionSamplingHeuristic");
			detailLayout.HideProperty("CollectionUseShapeWeights");
#endif
			if (shapeSamplingModeValue != EPopcornFXMeshSamplingMode::Weighted)
				detailLayout.HideProperty("DensityColorChannel");
			break;
		case	EPopcornFXAttribSamplerShapeType::Cylinder:
		case	EPopcornFXAttribSamplerShapeType::Capsule:
			detailLayout.HideProperty("BoxDimension");
			detailLayout.HideProperty("StaticMesh");
			detailLayout.HideProperty("StaticMeshSubIndex");
			detailLayout.HideProperty("Shapes");
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
			detailLayout.HideProperty("CollectionSamplingHeuristic");
			detailLayout.HideProperty("CollectionUseShapeWeights");
#endif
			detailLayout.HideProperty("ShapeSamplingMode");
			detailLayout.HideProperty("DensityColorChannel");
			break;
		case	EPopcornFXAttribSamplerShapeType::Cone:
			detailLayout.HideProperty("BoxDimension");
			detailLayout.HideProperty("InnerRadius");
			detailLayout.HideProperty("StaticMesh");
			detailLayout.HideProperty("StaticMeshSubIndex");
			detailLayout.HideProperty("Shapes");
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
			detailLayout.HideProperty("CollectionSamplingHeuristic");
			detailLayout.HideProperty("CollectionUseShapeWeights");
#endif
			detailLayout.HideProperty("ShapeSamplingMode");
			detailLayout.HideProperty("DensityColorChannel");
			break;
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
		case	EPopcornFXAttribSamplerShapeType::Collection:
			detailLayout.HideProperty("Weight");
			detailLayout.HideProperty("BoxDimension");
			detailLayout.HideProperty("Radius");
			detailLayout.HideProperty("InnerRadius");
			detailLayout.HideProperty("Height");
			detailLayout.HideProperty("StaticMesh");
			detailLayout.HideProperty("StaticMeshSubIndex");
			detailLayout.HideProperty("ShapeSamplingMode");
			detailLayout.HideProperty("DensityColorChannel");
			break;
#endif
		default:
			break;
		}
		shapeType->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeSamplerShape::RebuildDetails));
		shapeSamplingMode->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeSamplerShape::RebuildDetails));
	}
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
