//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#if WITH_EDITOR

#include "PopcornFXCustomizationAttributeSamplerCurve.h"

#include "PopcornFXAttributeSamplerCurve.h"

#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXCustomizationAttributeSamplerCurve, Log, All);

//----------------------------------------------------------------------------

/** Makes a new instance of this detail layout class for a specific detail view requesting it */
TSharedRef<IPropertyTypeCustomization>	FPopcornFXCustomizationAttributeSamplerCurve::MakeInstance()
{
	return MakeShareable(new FPopcornFXCustomizationAttributeSamplerCurve);
}

//----------------------------------------------------------------------------

void FPopcornFXCustomizationAttributeSamplerCurve::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder &ChildBuilder, IPropertyTypeCustomizationUtils &CustomizationUtils)
{
	if (!PropertyHandle->IsValidHandle())
	{
		return;
	}

	FPopcornFXCustomizationAttributeSampler::CustomizeChildren(PropertyHandle, ChildBuilder, CustomizationUtils);

	TSharedPtr<IPropertyHandle>	isDoubleCurvePty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesCurve, bIsDoubleCurve));
	bool	isDoubleCurveValue = false;
	isDoubleCurvePty->GetValue(isDoubleCurveValue);
	isDoubleCurvePty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSamplerCurve::RebuildProperties));
	// Legacy feature from v1 that does not exist anymore
	/*ChildBuilder.AddProperty(isDoubleCurvePty.ToSharedRef());*/

	TSharedPtr<IPropertyHandle>	curveDimensionPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesCurve, CurveDimension));
	uint8	curveDimensionValue = 0;
	curveDimensionPty->GetValue(curveDimensionValue);
	curveDimensionPty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSamplerCurve::RebuildProperties));
	AddErrorableProperty(PropertyHandle, "CurveDimension", ChildBuilder, m_Sampler && !m_Sampler->bIsInline, false);

	AddErrorableProperty(PropertyHandle, "Curve1D", ChildBuilder, curveDimensionValue == EAttributeSamplerCurveDimension::Float1, true);

	AddErrorableProperty(PropertyHandle, "SecondCurve1D", ChildBuilder, curveDimensionValue == EAttributeSamplerCurveDimension::Float1 && isDoubleCurveValue, true);

	AddErrorableProperty(PropertyHandle, "Curve3D", ChildBuilder, curveDimensionValue == EAttributeSamplerCurveDimension::Float3, true);

	AddErrorableProperty(PropertyHandle, "SecondCurve3D", ChildBuilder, curveDimensionValue == EAttributeSamplerCurveDimension::Float3 && isDoubleCurveValue, true);

	AddErrorableProperty(PropertyHandle, "Curve4D", ChildBuilder, curveDimensionValue == EAttributeSamplerCurveDimension::Float4, true);

	AddErrorableProperty(PropertyHandle, "SecondCurve4D", ChildBuilder, curveDimensionValue == EAttributeSamplerCurveDimension::Float4 && isDoubleCurveValue, true);
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
