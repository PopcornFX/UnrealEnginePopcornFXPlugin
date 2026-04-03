//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#if WITH_EDITOR

#include "PopcornFXCustomizationAttributeSampler.h"
#include "PopcornFXAttributeSampler.h"
#include "PopcornFXAttributeList.h"
#include "Editor/PopcornFXStyle.h"
#include "PopcornFXSDK.h"

#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"

#include "Styling/StyleColors.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXCustomizationAttributeSampler, Log, All);

//----------------------------------------------------------------------------
namespace
{
	const char *ResolveAttribSamplerNodeName(EPopcornFXAttributeSamplerType::Type samplerType)
	{
		switch (samplerType)
		{
		case	EPopcornFXAttributeSamplerType::Shape:
			return "AttributeSampler_Shape";
		case	EPopcornFXAttributeSamplerType::Curve:
			return "AttributeSampler_Curve";
		case	EPopcornFXAttributeSamplerType::Image:
			return "AttributeSampler_Image";
		case	EPopcornFXAttributeSamplerType::Grid:
			return "AttributeSampler_Grid";
		case	EPopcornFXAttributeSamplerType::AnimTrack:
			return "Attributesampler_AnimTrack";
		case	EPopcornFXAttributeSamplerType::Text:
			return "AttributeSampler_Text";
		case	EPopcornFXAttributeSamplerType::Turbulence:
			return "AttributeSampler_VectorField";
		default:
			PK_ASSERT_NOT_REACHED();
			return null;
		}
	}
}

//----------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------

FPopcornFXCustomizationAttributeSampler::FPopcornFXCustomizationAttributeSampler()
:	m_Emitter(null)
,	m_Effect(null)
,	m_CachedPropertyUtilities()
{

}

//----------------------------------------------------------------------------

/** Makes a new instance of this detail layout class for a specific detail view requesting it */
TSharedRef<IPropertyTypeCustomization>	FPopcornFXCustomizationAttributeSampler::MakeInstance()
{
	return MakeShareable(new FPopcornFXCustomizationAttributeSampler);
}

//----------------------------------------------------------------------------

void FPopcornFXCustomizationAttributeSampler::UpdatePreviewEmitter()
{
	if (m_Effect && m_Effect->PreviewEmitter)
	{
		m_Effect->PreviewEmitter->ResetSamplersToDefault();
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXCustomizationAttributeSampler::RebuildProperties()
{
	if (!PK_VERIFY(m_CachedPropertyUtilities != null))
		return;
	m_CachedPropertyUtilities->RequestForceRefresh();
}

//----------------------------------------------------------------------------

void	FPopcornFXCustomizationAttributeSampler::ResolveParents(TSharedRef<IPropertyHandle> PropertyHandle)
{
	TArray<UObject *> outerObjects;
	PropertyHandle->GetOuterObjects(outerObjects);
	PK_VERIFY(outerObjects.Num() > 0);
	// If it's the preview/asset editor it will retrieve the effect's sampler from UPopcornFXEffect::DefaultSamplers
	// If it's an emitter in a level, it will retrieve the emitter's sampler from UPopcornFXEmitterComponent::Samplers
	m_Sampler = Cast<UPopcornFXAttributeSampler>(outerObjects[0]);
	if (m_Sampler)
	{
		// If it's the preview/asset editor, outer of the sampler is the effect
		// If it's an emitter in a level, outer of the sampler is the emitter
		m_Effect = Cast<UPopcornFXEffect>(m_Sampler->GetOuter());
		if (m_Effect)
		{
			m_Emitter = m_Effect->PreviewEmitter;
			if (!PK_VERIFY(m_Emitter != null))
			{
				UE_LOG(LogPopcornFXCustomizationAttributeSampler, Error, TEXT("Could not find preview emitter of effect '%s'"),
					*m_Effect->GetName());
				return;
			}

			int32	sampleri = 0;
			bool	samplerFound = false;
			while (sampleri < m_Effect->DefaultSamplers.Num())
			{
				if (m_Effect->DefaultSamplers[sampleri] == m_Sampler)
				{
					samplerFound = true;
					break;
				}
				sampleri++;
			}

			PK_VERIFY(samplerFound && sampleri < m_Emitter->Samplers.Num());
			// Set m_Sampler to be the emitter's one because it will contain the unsupported properties
			m_Sampler = m_Emitter->Samplers[sampleri];
			PK_VERIFY(m_Sampler != null);
		}
		else
			m_Emitter = Cast<UPopcornFXEmitterComponent>(m_Sampler->GetOuter());

		m_Sampler->OnSamplerValidStateChanged.AddThreadSafeSP(this, &FPopcornFXCustomizationAttributeSampler::RebuildProperties);
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXCustomizationAttributeSampler::AddErrorableProperty(TSharedPtr<IPropertyHandle> PropertyHandle, const FString &PropertyName, IDetailChildrenBuilder &ChildBuilder, bool editCondition, bool editConditionHides)
{
	if (!PK_VERIFY(m_Sampler != null))
		return;
	TSharedPtr<IPropertyHandle>	property = PropertyHandle->GetChildHandle(*PropertyName);
	if (!property.IsValid())
	{
		UE_LOG(LogPopcornFXCustomizationAttributeSampler, Error, TEXT("Could not retrieve property '%s'"), *PropertyName);
		return;
	}

	// Check if this property is valid and compatible with this sampler's emitter
	if (!m_Sampler->m_UnsupportedProperties.Contains(*PropertyName) &&
		(!m_Sampler->m_IncompatibleProperties.Contains(m_Emitter) || !m_Sampler->m_IncompatibleProperties[m_Emitter].m_Properties.Contains(PropertyName)))
	{
		ChildBuilder.AddProperty(property.ToSharedRef()).
			EditCondition(editCondition, FOnBooleanValueChanged()).EditConditionHides(editConditionHides);
		return;
	}

	FText tooltipMessage = property->GetToolTipText();
	// Property incompatible with a particular emitter
	if (m_Emitter &&
		(m_Sampler->m_IncompatibleProperties.Contains(m_Emitter) && m_Sampler->m_IncompatibleProperties[m_Emitter].m_Properties.Contains(PropertyName)))
	{
		tooltipMessage = FText::FromString(m_Sampler->m_IncompatibleProperties[m_Emitter].m_Properties[PropertyName]);
	}
	// Unsupported property in the sampler
	if (m_Sampler->m_UnsupportedProperties.Contains(PropertyName))
	{
		tooltipMessage = FText::FromString(m_Sampler->m_UnsupportedProperties[PropertyName]);
	}
	TSharedPtr<SWidget> nameWidget;
	TSharedPtr<SWidget> valueWidget;
	IDetailPropertyRow &SyncDisplayProperty = ChildBuilder.AddProperty(property.ToSharedRef()).
		EditCondition(editCondition, FOnBooleanValueChanged()).EditConditionHides(editConditionHides);
	SyncDisplayProperty.GetDefaultWidgets(nameWidget, valueWidget);

	// This erases the current Name, Value and ResetToDefault widgets, don't forget to set them back if you want them!
	SyncDisplayProperty.CustomWidget(true)
	.NameContent()
	[
		SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
					.Text(property->GetPropertyDisplayName())
					.ToolTipText(tooltipMessage)
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.ColorAndOpacity(USlateThemeManager::Get().GetColor(EStyleColor::Error))
			]
	]
	.ValueContent() // Set the default Value widget back
	[
		valueWidget.ToSharedRef()
	];

	// Use this to add a row with a custom message underneath
	/*ChildBuilder.AddCustomRow(FText::FromString("Sampler setup error")).WholeRowContent()
	[
		SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
					.Text(FText::FromString("The sampler setup is invalid because XXXXXXXXX is not supported"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.ColorAndOpacity(USlateThemeManager::Get().GetColor(EStyleColor::Error))
			]
	];*/
}

//----------------------------------------------------------------------------

void	FPopcornFXCustomizationAttributeSampler::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Do it every time, references get broken
	ResolveParents(PropertyHandle);

	PropertyHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSampler::UpdatePreviewEmitter));
}

//----------------------------------------------------------------------------

void	FPopcornFXCustomizationAttributeSampler::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	if (!PropertyHandle->IsValidHandle())
	{
		return;
	}

	m_CachedPropertyUtilities = CustomizationUtils.GetPropertyUtilities();
	// Do it every time, references get broken
	ResolveParents(PropertyHandle);
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
