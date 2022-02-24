//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsAttributeSamplerCurve.h"

#include "PopcornFXAttributeSampler.h"
#include "PopcornFXAttributeSamplerCurve.h"
#include "PopcornFXSDK.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXDetailsAttributeSamplerCurve"

//----------------------------------------------------------------------------

FPopcornFXDetailsAttributeSamplerCurve::FPopcornFXDetailsAttributeSamplerCurve()
	: m_CachedDetailLayoutBuilder(null)
{

}

//----------------------------------------------------------------------------

TSharedRef<IDetailCustomization>	FPopcornFXDetailsAttributeSamplerCurve::MakeInstance()
{
	return MakeShareable(new FPopcornFXDetailsAttributeSamplerCurve);
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerCurve::RebuildDetails()
{
	if (m_CachedDetailLayoutBuilder != null)
		m_CachedDetailLayoutBuilder->ForceRefreshDetails();
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsAttributeSamplerCurve::CustomizeDetails(IDetailLayoutBuilder& detailLayout)
{
	TSharedRef<IPropertyHandle>	isDoubleCurve = detailLayout.GetProperty("bIsDoubleCurve");
	TSharedRef<IPropertyHandle>	curveDimension = detailLayout.GetProperty("CurveDimension");
	IDetailCategoryBuilder		&detailCategory = detailLayout.EditCategory("PopcornFX AttributeSampler");

	m_CachedDetailLayoutBuilder = &detailLayout;
	if (curveDimension->IsValidHandle())
	{
		uint8	curveDimensionValue = 0;
		bool	isDoubleCurveValue = false;

		isDoubleCurve->GetValue(isDoubleCurveValue);
		curveDimension->GetValue(curveDimensionValue);
		switch (curveDimensionValue)
		{
		case	EAttributeSamplerCurveDimension::Float1:
			if (!isDoubleCurveValue)
				detailLayout.HideProperty("SecondCurve1D");
			detailLayout.HideProperty("Curve2D");
			detailLayout.HideProperty("SecondCurve2D");
			detailLayout.HideProperty("Curve3D");
			detailLayout.HideProperty("SecondCurve3D");
			detailLayout.HideProperty("Curve4D");
			detailLayout.HideProperty("SecondCurve4D");
			break;
		case	EAttributeSamplerCurveDimension::Float2:
			if (!isDoubleCurveValue)
				detailLayout.HideProperty("SecondCurve2D");
			detailLayout.HideProperty("Curve1D");
			detailLayout.HideProperty("SecondCurve1D");
			detailLayout.HideProperty("Curve3D");
			detailLayout.HideProperty("SecondCurve3D");
			detailLayout.HideProperty("Curve4D");
			detailLayout.HideProperty("SecondCurve4D");
			break;
		case	EAttributeSamplerCurveDimension::Float3:
			if (!isDoubleCurveValue)
				detailLayout.HideProperty("SecondCurve3D");
			detailLayout.HideProperty("Curve1D");
			detailLayout.HideProperty("SecondCurve1D");
			detailLayout.HideProperty("Curve2D");
			detailLayout.HideProperty("SecondCurve2D");
			detailLayout.HideProperty("Curve4D");
			detailLayout.HideProperty("SecondCurve4D");
			break;
		case	EAttributeSamplerCurveDimension::Float4:
			if (!isDoubleCurveValue)
				detailLayout.HideProperty("SecondCurve4D");
			detailLayout.HideProperty("Curve1D");
			detailLayout.HideProperty("SecondCurve1D");
			detailLayout.HideProperty("Curve2D");
			detailLayout.HideProperty("SecondCurve2D");
			detailLayout.HideProperty("Curve3D");
			detailLayout.HideProperty("SecondCurve3D");
			break;
		default:
			break;
		}
		curveDimension->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeSamplerCurve::RebuildDetails));
		isDoubleCurve->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeSamplerCurve::RebuildDetails));
	}
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
