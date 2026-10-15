//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#if WITH_EDITOR

#include "PopcornFXCustomizationAttributeSamplerGrid.h"

#include "PopcornFXAttributeSamplerGrid.h"

#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXCustomizationAttributeSamplerGrid, Log, All);

//----------------------------------------------------------------------------

/** Makes a new instance of this detail layout class for a specific detail view requesting it */
TSharedRef<IPropertyTypeCustomization>	FPopcornFXCustomizationAttributeSamplerGrid::MakeInstance()
{
	return MakeShareable(new FPopcornFXCustomizationAttributeSamplerGrid);
}

//----------------------------------------------------------------------------

void FPopcornFXCustomizationAttributeSamplerGrid::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder &ChildBuilder, IPropertyTypeCustomizationUtils &CustomizationUtils)
{
	if (!PropertyHandle->IsValidHandle())
	{
		return;
	}

	FPopcornFXCustomizationAttributeSampler::CustomizeChildren(PropertyHandle, ChildBuilder, CustomizationUtils);

	TSharedPtr<IPropertyHandle>	isAssetGridPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesGrid, bAssetGrid));
	bool isAssetGridValue;
	isAssetGridPty->GetValue(isAssetGridValue);
	isAssetGridPty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSamplerGrid::RebuildProperties));

	AddErrorableProperty(PropertyHandle, "bAssetGrid", ChildBuilder, true, false);
	
	AddErrorableProperty(PropertyHandle, "bSRGB", ChildBuilder, isAssetGridValue, true);

	AddErrorableProperty(PropertyHandle, "RenderTarget", ChildBuilder, isAssetGridValue, true);

	// Always show the property but only editable when using an external sampler
	if (!isAssetGridValue)
	{
		AddErrorableProperty(PropertyHandle, "Order", ChildBuilder, m_Sampler && !m_Sampler->bIsInline, false);
	}

	// Refresh UI when we change the order to add/remove rows
	TSharedPtr<IPropertyHandle>	orderPty;
	uint8 orderValue;
	orderPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesGrid, Order));
	orderPty->GetValue(orderValue);
	orderPty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSamplerGrid::RebuildProperties));

	AddErrorableProperty(PropertyHandle, "SizeX", ChildBuilder, !isAssetGridValue, true);

	AddErrorableProperty(PropertyHandle, "SizeY", ChildBuilder, !isAssetGridValue && orderValue >= EPopcornFXGridOrder::TwoD, true);

	AddErrorableProperty(PropertyHandle, "SizeZ", ChildBuilder, !isAssetGridValue && orderValue >= EPopcornFXGridOrder::ThreeD, true);

	// Always show the property but only editable when using an external sampler
	if (!isAssetGridValue)
	{
		AddErrorableProperty(PropertyHandle, "DataType", ChildBuilder, m_Sampler && !m_Sampler->bIsInline, false);
	}
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
