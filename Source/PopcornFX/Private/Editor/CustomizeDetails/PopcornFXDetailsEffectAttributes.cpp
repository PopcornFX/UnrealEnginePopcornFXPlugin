//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsEffectAttributes.h"

#include "Assets/PopcornFXEffect.h"
#include "PopcornFXEmitterComponent.h"
#include "Editor/EditorHelpers.h"
#include "Editor/PopcornFXStyle.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"

#include "Styling/StyleColors.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXDetailsEmitterComponent"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXDetailsEffectAttributes, Log, All);

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEffectAttributes::BuildSampler(const FPopcornFXSamplerDesc *desc, const TSharedPtr<IPropertyHandle> samplerPty, const TSharedPtr<IPropertyHandle> samplerDescPty, const UPopcornFXAttributeList *attrList, uint32 sampleri, uint32 iCategory)
{
	const FString		&name = desc->m_SamplerName;

	FString				defNode;
	FName				samplerIconName;

	UPopcornFXEffect *effect = ResolveEffect(attrList);
	if (effect == null)
		return;

	// TODO(Attributes refactor): This should not use 'CParticleNodeSamplerData'
	const PopcornFX::CParticleAttributeSamplerDeclaration *particleSampler = static_cast<const PopcornFX::CParticleAttributeSamplerDeclaration *>(attrList->GetParticleSampler(effect, sampleri));
	if (particleSampler == null)
		return;

	const char *nodeName = ResolveAttribSamplerNodeName(particleSampler, desc->m_SamplerType);
	if (nodeName != null)
	{
		defNode = nodeName;
		samplerIconName = FName(*("PopcornFX.Node." + defNode));
	}
	else
	{
		defNode = "?????";
		samplerIconName = FName(TEXT("PopcornFX.BadIcon32"));
	}

	if (!samplerPty.IsValid() || !samplerPty->IsValidHandle())
	{
		return;
	}

	UPopcornFXEmitterComponent *emitter = Cast<UPopcornFXEmitterComponent>(effect->PreviewEmitter);
	if (!PK_VERIFY(emitter != null))
	{
		UE_LOG(LogPopcornFXDetailsEffectAttributes, Error, TEXT("Could not retrieve the preview emitter"));
		return;
	}

	UPopcornFXAttributeSampler *sampler = desc->ResolveAttributeSampler(emitter, null);
	if (sampler)
		sampler->OnSamplerValidStateChanged.AddThreadSafeSP(this, &FPopcornFXDetailsEffectAttributes::RebuildIFN);

	FSlateColor		samplerNameColor = USlateThemeManager::Get().GetColor(EStyleColor::Foreground);
	FText			tooltipText = FText::FromString(name + ": " + defNode);
	if (!sampler ||
		(sampler->m_IncompatibleProperties.Contains(emitter) && !sampler->m_IncompatibleProperties[emitter].m_Properties.IsEmpty())
		|| sampler->m_UnsupportedProperties.Num() > 0)
	{
		samplerNameColor = USlateThemeManager::Get().GetColor(EStyleColor::Error);
		tooltipText = FText::FromString("One or more properties are unsupported. Default values exported from PopcornFX will be used");
	}

	IDetailGroup &newGroup = m_IGroups[iCategory]->AddGroup(desc->SamplerFName(), FText::FromString(desc->m_SamplerName));
	newGroup.HeaderRow()
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(1.f)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
						.WidthOverride(16)
						.HeightOverride(16)
						[
							SNew(SImage)
								.Image(FPopcornFXStyle::GetBrush(samplerIconName))
								.ColorAndOpacity(samplerNameColor)
						]
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(name))
						.ToolTipText(tooltipText)
						.Font(m_DetailLayoutBuilder->GetDetailFont())
						.ColorAndOpacity(samplerNameColor)
				]
		];

	TSharedPtr<IPropertyHandle>	samplerStructPty = ResolveSamplerProperties(samplerPty, desc->SamplerType(), defNode);
	if (!samplerStructPty.IsValid() || !samplerStructPty->IsValidHandle())
		return;

	newGroup.AddPropertyRow(samplerStructPty.ToSharedRef());
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEffectAttributes::CustomizeDetails(class IDetailLayoutBuilder &DetailLayout)
{
	// calling EditCategory reorder Categories in the Editor
	DetailLayout.HideCategory("PopcornFX RendererMaterials");
	DetailLayout.HideCategory("PopcornFX AssetDependencies");
	DetailLayout.HideCategory("Source");
	DetailLayout.HideCategory("UserDatas");
	DetailLayout.HideProperty("DefaultSamplers");

	m_PropertyUtilities = DetailLayout.GetPropertyUtilities();
	DetailLayout.GetObjectsBeingCustomized(m_BeingCustomized);

	m_DetailLayoutBuilder = &DetailLayout;

	m_AttributeListPty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPopcornFXEffect, DefaultAttributeList));
	if (!PK_VERIFY(IsValidHandle(m_AttributeListPty)))
	{
		UE_LOG(LogPopcornFXDetailsEffectAttributes, Error, TEXT("Can't retrieve default attribute list property!"));
		return;
	}
	m_SamplersDescPty = m_AttributeListPty->GetChildHandle(GET_MEMBER_NAME_CHECKED(UPopcornFXAttributeList, m_Samplers));
	if (!PK_VERIFY(IsValidHandle(m_SamplersDescPty)))
	{
		UE_LOG(LogPopcornFXDetailsEffectAttributes, Error, TEXT("Can't retrieve samplers description property!"));
		return;
	}

	m_SamplersPty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPopcornFXEffect, DefaultSamplers));
	if (!PK_VERIFY(IsValidHandle(m_SamplersPty)))
	{
		UE_LOG(LogPopcornFXDetailsEffectAttributes, Error, TEXT("Can't retrieve default samplers property!"));
		return;
	}

	m_AttributeListCategory = &DetailLayout.EditCategory("PopcornFX Default Attributes");
	m_AttributeListCategory->SetDisplayName(FText::FromString("Default attribute values"));

	FTextBlockStyle style;
	style.SetColorAndOpacity(FSlateColor::UseForeground());
	style.SetFont(DetailLayout.GetDetailFont());

	TSharedPtr<SWidget> hbox;
	SAssignNew(hbox, SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
		[
			SNew(SScaleBox)
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("Icons.Help"))
				]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
		[
			SNew(SButton).Text(FText::FromString("Help")).TextStyle(&style).OnClicked_Lambda([]()
				{
					FPlatformProcess::LaunchURL(TEXT("https://documentation.popcornfx.com/PopcornFX/latest/plugins/ue-plugin/effects.html#_effect_attributes"), NULL, NULL);
					return FReply::Handled();
				})
		];
	m_AttributeListCategory->HeaderContent(hbox.ToSharedRef());

	m_AttributeListCategory->AddCustomRow(FText::FromString("Tooltip"))
		.NameContent()
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(EVerticalAlignment::VAlign_Center)
				[
					SNew(SScaleBox)
						[
							SNew(SImage).Image(FCoreStyle::Get().GetBrush("Icons.Info"))
						]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(EVerticalAlignment::VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(" Note"))
						.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
		]
		.ValueContent()
		[
			SNew(STextBlock)
				.AutoWrapText(true)
				.Font(DetailLayout.GetDetailFont())
				.Text(FText::FromString("Here are the default attribute and attribute sampler values of your effect.\n"
										"Every future emitter using this effect will have these values by default.\n"
										"They also affect the emitter in the viewport above to preview your changes.\n"
										"This can be changed per instance by editing emitters"))
		];

	Rebuild();
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
