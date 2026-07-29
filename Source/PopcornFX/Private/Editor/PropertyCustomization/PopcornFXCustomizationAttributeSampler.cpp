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
		case	EPopcornFXAttributeSamplerType::VectorField:
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
:	m_Properties(null)
,	m_Sampler(null)
,	m_Emitter(null)
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

	void	*propertyData = nullptr;
	PropertyHandle->GetValueData(propertyData);
	m_Properties = static_cast<FPopcornFXAttributeSamplerProperties*>(propertyData);
	PK_ASSERT(m_Properties != nullptr);

	// The FPopcornFXSamplerDesc handle (in emitters, twice the parent handle to skip the TOptional)
	TSharedPtr<IPropertyHandle> samplerDescHandle = PropertyHandle->GetParentHandle()->GetParentHandle();

	FPopcornFXSamplerDesc *desc = nullptr;
	if (samplerDescHandle)
	{
		void *samplerDescData = nullptr;
		samplerDescHandle->GetValueData(samplerDescData);
		desc = static_cast<FPopcornFXSamplerDesc *>(samplerDescData);
	}
	if (desc != nullptr)
	{
		UPopcornFXEmitterComponent *emitter = Cast<UPopcornFXEmitterComponent>(outerObjects[0]);
		if (emitter)
		{
			m_Sampler = emitter->AttributeList.ResolveAttributeSampler(samplerDescHandle->GetArrayIndex());
			m_Emitter = emitter;
			return;
		}

		// We're editing the effect asset, fetch its preview emitter
		UPopcornFXEffect *effect = Cast<UPopcornFXEffect>(outerObjects[0]);
		if (effect)
		{
			m_Sampler = effect->PreviewEmitter->AttributeList.ResolveAttributeSampler(samplerDescHandle->GetArrayIndex());
			m_Emitter = effect->PreviewEmitter;
			return;
		}
	}
	else
	{
		// We are in an external sampler
		// TODO
		if (Cast<UPopcornFXAttributeSamplerImageAsset>(outerObjects[0]))
		{
			//m_Properties = static_cast<FPopcornFXAttributeSamplerPropertiesImage *>(samplerData);
		}
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXCustomizationAttributeSampler::AddErrorableProperty(TSharedPtr<IPropertyHandle> PropertyHandle, const FString &PropertyName, IDetailChildrenBuilder &ChildBuilder, bool editCondition, bool editConditionHides)
{
	if (!PK_VERIFY(m_Properties != null))
		return;
	TSharedPtr<IPropertyHandle>	property = PropertyHandle->GetChildHandle(*PropertyName);
	if (!property.IsValid())
	{
		UE_LOG(LogPopcornFXCustomizationAttributeSampler, Error, TEXT("Could not retrieve property '%s'"), *PropertyName);
		return;
	}
	// TODO: need to bind this? or can bind this in PopcornFXCustomizationAttributeList::BuildSampler, SetOnChildPropertyValueChanged
	// Calls PostEditChangeProperty on the given sampler when one of its property is modified
	property->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSampler::PropagatePostEditChangeProperty, property));

	// Check if this property is valid and compatible with this sampler's emitter
	if (!m_Properties->m_UnsupportedProperties.Contains(PropertyName) &&
		(!m_Emitter || !m_Emitter->m_IncompatibleProperties.Contains(m_Properties) || !m_Emitter->m_IncompatibleProperties[m_Properties].m_Properties.Contains(PropertyName)))
	{
		ChildBuilder.AddProperty(property.ToSharedRef()).
			EditCondition(editCondition, FOnBooleanValueChanged()).EditConditionHides(editConditionHides);
		return;
	}

	FText tooltipMessage = property->GetToolTipText();
	// Property incompatible with a particular emitter
	if (m_Emitter &&
		(m_Emitter->m_IncompatibleProperties.Contains(m_Properties) && m_Emitter->m_IncompatibleProperties[m_Properties].m_Properties.Contains(PropertyName)))
	{
		tooltipMessage = FText::FromString(m_Emitter->m_IncompatibleProperties[m_Properties].m_Properties[PropertyName]);
	}
	// Unsupported property in the sampler
	if (m_Properties->m_UnsupportedProperties.Contains(PropertyName))
	{
		tooltipMessage = FText::FromString(m_Properties->m_UnsupportedProperties[PropertyName]);
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

void	FPopcornFXCustomizationAttributeSampler::PropagatePostEditChangeProperty(TSharedPtr<IPropertyHandle> Property)
{
	// Sampler properties asset: scroll through samplers using these properties and update them
	FPropertyChangedEvent propertyChangedEvent(Property->GetProperty());
	if (m_Properties && !m_Sampler)
	{
		for (auto &emitterSamplers : m_Properties->m_EmitterSamplersUsingThis)
		{
			TSoftObjectPtr<UPopcornFXEmitterComponent>	&emitter = emitterSamplers.Key;
			if (emitter.IsValid())
			{
				TArray<FString> &samplerNames = emitterSamplers.Value.m_SamplerNames;
				for (const FString &samplerName : samplerNames)
				{
					int32 samplerIdx = emitter->AttributeList.FindSamplerIndex(samplerName);
					if (samplerIdx != -1)
					{
						emitter->AttributeList.ResolveAttributeSampler(samplerIdx)->PostEditChangeProperty(propertyChangedEvent);
					}
				}
				emitter->RestartEmitter();
			}
		}
		return;
	}

	// Inline sampler properties: update its sampler
	FPopcornFXAttributeSampler *sampler = static_cast<FPopcornFXAttributeSampler *>(m_Sampler);
	if (sampler != nullptr)
	{
		sampler->CopyPropertiesFrom(m_Properties);
	}
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
