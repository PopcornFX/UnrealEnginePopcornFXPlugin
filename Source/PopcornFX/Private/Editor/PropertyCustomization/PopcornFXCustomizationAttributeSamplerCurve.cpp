//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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
	ChildBuilder.AddProperty(curveDimensionPty.ToSharedRef());

	TSharedPtr<IPropertyHandle>	curve1DPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesCurve, Curve1D));
	ChildBuilder.AddProperty(curve1DPty.ToSharedRef()).
		EditCondition(curveDimensionValue == EAttributeSamplerCurveDimension::Float1, FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	secondCurve1DPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesCurve, SecondCurve1D));
	ChildBuilder.AddProperty(secondCurve1DPty.ToSharedRef()).
		EditCondition(curveDimensionValue == EAttributeSamplerCurveDimension::Float1 && isDoubleCurveValue, FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	curve3DPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesCurve, Curve3D));
	ChildBuilder.AddProperty(curve3DPty.ToSharedRef()).
		EditCondition(curveDimensionValue == EAttributeSamplerCurveDimension::Float3, FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	secondCurve3DPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesCurve, SecondCurve3D));
	ChildBuilder.AddProperty(secondCurve3DPty.ToSharedRef()).
		EditCondition(curveDimensionValue == EAttributeSamplerCurveDimension::Float3 && isDoubleCurveValue, FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	curve4DPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesCurve, Curve4D));
	ChildBuilder.AddProperty(curve4DPty.ToSharedRef()).
		EditCondition(curveDimensionValue == EAttributeSamplerCurveDimension::Float4, FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	secondCurve4DPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesCurve, SecondCurve4D));
	ChildBuilder.AddProperty(secondCurve4DPty.ToSharedRef()).
		EditCondition(curveDimensionValue == EAttributeSamplerCurveDimension::Float4 && isDoubleCurveValue, FOnBooleanValueChanged()).
		EditConditionHides(true);
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
