//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsEmitterComponent.h"

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

#include "SAssetDropTarget.h"
#include "EditorReimportHandler.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXDetailsEmitterComponent"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXDetailsEmitterComponent, Log, All);

//----------------------------------------------------------------------------

FPopcornFXDetailsEmitterComponent::FPopcornFXDetailsEmitterComponent()
{

}

//----------------------------------------------------------------------------

TSharedRef<IDetailCustomization>	FPopcornFXDetailsEmitterComponent::MakeInstance()
{
	return MakeShareable(new FPopcornFXDetailsEmitterComponent);
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEmitterComponent::GatherEmitters(TArray<UPopcornFXEmitterComponent*> &outComponents) const
{
	const TArray< TWeakObjectPtr<UObject> >		&objects = m_BeingCustomized;
	for (int32 obji = 0; obji < objects.Num(); ++obji)
	{
		if (objects[obji].IsValid())
		{
			UPopcornFXEmitterComponent* emitterComponent = Cast<UPopcornFXEmitterComponent>(objects[obji].Get());
			if (emitterComponent == null)
			{
				APopcornFXEmitter* actor = Cast<APopcornFXEmitter>(objects[obji].Get());
				if (actor != null)
					emitterComponent = actor->PopcornFXEmitterComponent;
			}
			if (emitterComponent != null)
				outComponents.Add(emitterComponent);
		}
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEmitterComponent::GatherEffects(TArray<UPopcornFXEffect*> &outEffects)
{
	const TArray< TWeakObjectPtr<UObject> >		&objects = m_BeingCustomized;
	for (int32 obji = 0; obji < objects.Num(); ++obji)
	{
		if (objects[obji].IsValid())
		{
			UPopcornFXEmitterComponent	*emitterComponent = Cast<UPopcornFXEmitterComponent>(objects[obji].Get());
			if (emitterComponent == null)
			{
				APopcornFXEmitter		*actor = Cast<APopcornFXEmitter>(objects[obji].Get());
				if (actor != null)
					emitterComponent = actor->PopcornFXEmitterComponent;
			}
			if (emitterComponent != null && emitterComponent->Effect != null)
				outEffects.AddUnique(emitterComponent->Effect);
		}
	}
}

//----------------------------------------------------------------------------

FReply		FPopcornFXDetailsEmitterComponent::OnStartEmitter()
{
	TArray<UPopcornFXEmitterComponent*> emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
	{
		if (emitters[i]->IsRegistered())
			emitters[i]->StartEmitter();
	}
	return FReply::Handled();
}

//----------------------------------------------------------------------------

FReply		FPopcornFXDetailsEmitterComponent::OnStopEmitter()
{
	TArray<UPopcornFXEmitterComponent*> emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
	{
		if (emitters[i]->IsRegistered())
			emitters[i]->StopEmitter();
	}
	return FReply::Handled();
}

//----------------------------------------------------------------------------

FReply		FPopcornFXDetailsEmitterComponent::OnKillParticles()
{
	TArray<UPopcornFXEmitterComponent*> emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
	{
		if (emitters[i]->IsRegistered())
			emitters[i]->KillParticles();
	}
	return FReply::Handled();
}

//----------------------------------------------------------------------------

FReply		FPopcornFXDetailsEmitterComponent::OnRestartEmitter()
{
	TArray<UPopcornFXEmitterComponent*> emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
	{
		if (emitters[i]->IsRegistered())
			emitters[i]->RestartEmitter(true);
	}
	return FReply::Handled();
}

//----------------------------------------------------------------------------

bool	FPopcornFXDetailsEmitterComponent::IsStartEnabled() const
{
	TArray<UPopcornFXEmitterComponent*> emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
	{
		if (!emitters[i]->IsEmitterStarted())
			return true;
	}
	return false;
}

//----------------------------------------------------------------------------

bool	FPopcornFXDetailsEmitterComponent::IsStopEnabled() const
{
	TArray<UPopcornFXEmitterComponent*> emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
	{
		if (emitters[i]->IsEmitterEmitting())
			return true;
	}
	return false;
}

//----------------------------------------------------------------------------

FReply	FPopcornFXDetailsEmitterComponent::OnReloadEffect()
{
	TArray<UPopcornFXEmitterComponent*>	emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
	{
		emitters[i]->Effect->ReloadFile();
	}
	return FReply::Handled();
}

//----------------------------------------------------------------------------

FReply	FPopcornFXDetailsEmitterComponent::OnReimportEffect()
{
	TArray<UPopcornFXEmitterComponent *>	emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
	{
		FReimportManager::Instance()->Reimport(emitters[i]->Effect);
	}
	return FReply::Handled();
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEmitterComponent::BuildSampler(const FPopcornFXSamplerDesc *desc, const TSharedPtr<IPropertyHandle> samplerPty, const TSharedPtr<IPropertyHandle> samplerDescPty, const UPopcornFXAttributeList *attrList, uint32 sampleri, uint32 iCategory)
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

	UPopcornFXEmitterComponent *emitter = Cast<UPopcornFXEmitterComponent>(m_BeingCustomized[0].Get());
	if (!PK_VERIFY(emitter != null))
	{
		UE_LOG(LogPopcornFXDetailsEmitterComponent, Error, TEXT("Could not retrieve the emitter associated with this sampler"));
		return;
	}

	UPopcornFXAttributeSampler *sampler = desc->ResolveAttributeSampler(emitter, null);
	if (sampler)
		sampler->OnSamplerValidStateChanged.AddThreadSafeSP(this, &FPopcornFXDetailsEmitterComponent::RebuildIFN);

	FSlateColor		samplerNameColor = USlateThemeManager::Get().GetColor(EStyleColor::Foreground);
	FText			tooltipText = FText::FromString(name + ": " + defNode);
	if (!sampler ||
		(sampler->m_IncompatibleProperties.Contains(emitter) && !sampler->m_IncompatibleProperties[emitter].m_Properties.IsEmpty())
		|| sampler->m_UnsupportedProperties.Num() > 0)
	{
		samplerNameColor = USlateThemeManager::Get().GetColor(EStyleColor::Error);
		tooltipText = FText::FromString("One or more properties are not supported. Default values exported from PopcornFX will be used");
	}

	IDetailGroup	&newGroup = m_IGroups[iCategory]->AddGroup(FName(desc->m_SamplerName), FText::FromString(desc->m_SamplerName));
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

	if (!samplerPty.IsValid() || !samplerPty->IsValidHandle()
		|| !samplerDescPty.IsValid() || !samplerDescPty->IsValidHandle())
	{
		return;
	}

	TSharedPtr<IPropertyHandle>	useExternalSamplerPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_UseExternalSampler));
	PK_ASSERT(useExternalSamplerPty.IsValid() && useExternalSamplerPty->IsValidHandle());
	if (!useExternalSamplerPty.IsValid() || !useExternalSamplerPty->IsValidHandle())
		return;

	IDetailPropertyRow &useExternalSamplerPtyRow = newGroup.AddPropertyRow(useExternalSamplerPty.ToSharedRef()).DisplayName(FText::FromString("Use external sampler?"));
	TSharedPtr<SWidget> defaultNameWidget;
	TSharedPtr<SWidget> defaultValueWidget;
	useExternalSamplerPtyRow.GetDefaultWidgets(defaultNameWidget, defaultValueWidget);

	// This erases the current Name, Value and ResetToDefault widgets, don't forget to set them back if you want them!
	useExternalSamplerPtyRow.CustomWidget(true)
	.NameContent()
	[
		SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
					.Text(useExternalSamplerPty->GetPropertyDisplayName())
					.ToolTipText(useExternalSamplerPty->GetToolTipText())
					.Font(m_DetailLayoutBuilder->GetDetailFont())
					.ColorAndOpacity(USlateThemeManager::Get().GetColor(EStyleColor::Foreground))
			]
	]
	.ValueContent() // Set the default Value widget back
	[
		defaultValueWidget.ToSharedRef()
	];

	useExternalSamplerPty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsEmitterComponent::RebuildIFN));

	TSharedPtr<IPropertyHandle>	restartWhenSamplerChangesPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_RestartWhenSamplerChanges));
	PK_ASSERT(restartWhenSamplerChangesPty.IsValid() && restartWhenSamplerChangesPty->IsValidHandle());
	newGroup.AddPropertyRow(restartWhenSamplerChangesPty.ToSharedRef()).DisplayName(FText::FromString("Restart when sampler changes?"))
		.EditCondition(desc->m_UseExternalSampler, FOnBooleanValueChanged()).EditConditionHides(true);

	if (desc->m_UseExternalSampler)
	{
		// Adds properties to reference an external sampler

		TSharedPtr<IPropertyHandle>		samplerActorPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_AttributeSamplerActor));
		PK_ASSERT(samplerActorPty.IsValid() && samplerActorPty->IsValidHandle());
		newGroup.AddPropertyRow(samplerActorPty.ToSharedRef()).DisplayName(FText::FromString("Target actor"));

		TSharedPtr<IPropertyHandle>		samplerCpntPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_AttributeSamplerComponentName));
		PK_ASSERT(samplerCpntPty.IsValid() && samplerCpntPty->IsValidHandle());
		IDetailPropertyRow &samplerComponentPtyRow = newGroup.AddPropertyRow(samplerCpntPty.ToSharedRef()).DisplayName(FText::FromString("Sampler component name"));

		TSharedPtr<IPropertyHandle>		validSamplerComponentPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_IsSamplerComponentValid));
		bool isSamplerComponentValid;
		validSamplerComponentPty->GetValue(isSamplerComponentValid);

		// If we can't find a sampler component with this name, customize the property to set its name font to red
		if (!isSamplerComponentValid)
		{
			samplerComponentPtyRow.GetDefaultWidgets(defaultNameWidget, defaultValueWidget);
			// This erases the current Name, Value and ResetToDefault widgets, don't forget to set them back if you want them!
			samplerComponentPtyRow.CustomWidget(true)
			.NameContent()
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(samplerCpntPty->GetPropertyDisplayName())
							.ToolTipText(samplerCpntPty->GetToolTipText())
							.Font(m_DetailLayoutBuilder->GetDetailFont())
							.ColorAndOpacity(USlateThemeManager::Get().GetColor(EStyleColor::Error))
					]
			]
			.ValueContent() // Set the default Value widget back
			[
				defaultValueWidget.ToSharedRef()
			];

			// Add a new row with an error message
			newGroup.AddWidgetRow().WholeRowContent()
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(FText::FromString("Can't find a valid sampler component with this name in the target actor"))
							.Font(m_DetailLayoutBuilder->GetDetailFont())
							.ColorAndOpacity(USlateThemeManager::Get().GetColor(EStyleColor::Error))
					]
			];
		}
		else if (sampler && sampler->m_IncompatibleProperties.Contains(emitter))
		{
			// Look for properties that are incompatible with this emitter's effect
			for (const auto& elem : sampler->m_IncompatibleProperties[emitter].m_Properties)
			{
				newGroup.AddWidgetRow().WholeRowContent()
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text(FText::FromString(elem.Value))
								.Font(m_DetailLayoutBuilder->GetDetailFont())
								.ColorAndOpacity(USlateThemeManager::Get().GetColor(EStyleColor::Error))
						]
				];
			}
		}
	}
	else
	{

		TSharedPtr<IPropertyHandle>	samplerStructPty = ResolveSamplerProperties(samplerPty, desc->SamplerType(), defNode);
		if (!samplerStructPty.IsValid() || !samplerStructPty->IsValidHandle())
			return;

		// Adds a custom property row, see PropertyCustomization/PopcornFXCustomizationAttributeSamplerXXX for each sampler
		newGroup.AddPropertyRow(samplerStructPty.ToSharedRef());
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEmitterComponent::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	m_PropertyUtilities = DetailLayout.GetPropertyUtilities();
	DetailLayout.GetObjectsBeingCustomized(m_BeingCustomized);

	// Keep this EditCategory order since it reorder categories in the editor
	IDetailCategoryBuilder &fxEditorCategory = DetailLayout.EditCategory("PopcornFX Emitter");
	m_AttributeListCategory = &DetailLayout.EditCategory("PopcornFX Attributes");
	m_AttributeListCategory->SetDisplayName(FText::FromString("PopcornFX Attributes"));

	DetailLayout.HideProperty("Samplers");

	FTextBlockStyle style;
	style.SetColorAndOpacity(FSlateColor::UseForeground());
	style.SetFont(DetailLayout.GetDetailFont());

	// Add some buttons
	fxEditorCategory.AddCustomRow(LOCTEXT("Emitter Actions", "Emitter Actions"), false)
		.NameContent()
		[
			SNew(STextBlock)
				.Font(DetailLayout.GetDetailFont())
				.Text(LOCTEXT("EmitterActions", "Emitter Actions"))
		]
		.ValueContent()
		.MinDesiredWidth(125.0f * 3.0f)
		.MaxDesiredWidth(125.0f * 4.0f).HAlign(EHorizontalAlignment::HAlign_Fill)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[SNew(SButton)
				.Text(LOCTEXT("Start", "Start")).ContentPadding(FMargin(0.0f, 1.0f))
				.TextStyle(&style)
				.ToolTipText(LOCTEXT("Start_ToolTip", "Starts the emitter. Available if the emitter is not \"IsEmitterStarted\"."))
				.OnClicked(this, &FPopcornFXDetailsEmitterComponent::OnStartEmitter)
				.IsEnabled(this, &FPopcornFXDetailsEmitterComponent::IsStartEnabled)]
				+ SHorizontalBox::Slot()
				[SNew(SButton)
				.Text(LOCTEXT("Stop", "Stop")).ContentPadding(FMargin(0.0f, 1.0f))
				.TextStyle(&style)
				.ToolTipText(LOCTEXT("Stop_ToolTip", "Stops the emitter. Available if the emitter is \"IsEmitterEmitting\"."))
				.OnClicked(this, &FPopcornFXDetailsEmitterComponent::OnStopEmitter)
				.IsEnabled(this, &FPopcornFXDetailsEmitterComponent::IsStopEnabled)
				]
				+ SHorizontalBox::Slot()
				[SNew(SButton)
				.Text(LOCTEXT("Restart", "Restart")).ContentPadding(FMargin(0.0f, 1.0f))
				.TextStyle(&style)
				.ToolTipText(LOCTEXT("Restart_ToolTip", "Terminates then starts the emitter."))
				.OnClicked(this, &FPopcornFXDetailsEmitterComponent::OnRestartEmitter)]
				+ SHorizontalBox::Slot()
				[SNew(SButton)
				.Text(LOCTEXT("Kill Particles", "Kill Particles")).HAlign(EHorizontalAlignment::HAlign_Fill).ContentPadding(FMargin(0.0f, 1.0f))
				.TextStyle(&style)
				.ToolTipText(LOCTEXT("kill_ToolTip", "Kill emitter's particles and stop the emitter."))
				.OnClicked(this, &FPopcornFXDetailsEmitterComponent::OnKillParticles)]
		]
		;
	// Add some buttons
	fxEditorCategory.AddCustomRow(LOCTEXT("Effect Actions", "Effect Actions"), false)
		.NameContent()
		[
			SNew(STextBlock)
				.Font(DetailLayout.GetDetailFont())
				.Text(LOCTEXT("EffectActions", "Effect Actions"))
		]
		.ValueContent()
		.MinDesiredWidth(125.0f * 3.0f)
		.MaxDesiredWidth(125.0f * 4.0f)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[SNew(SButton)
				.Text(LOCTEXT("Reload", "Reload")).OnClicked(this, &FPopcornFXDetailsEmitterComponent::OnReloadEffect)
				.TextStyle(&style)
				.ToolTipText(LOCTEXT("Reload_ToolTip", "Reloads the PopcornFXEffect, will kill all particles related to this effect."))]
				+ SHorizontalBox::Slot()
				[SNew(SButton)
				.Text(LOCTEXT("Reimport", "Reimport")).OnClicked(this, &FPopcornFXDetailsEmitterComponent::OnReimportEffect)
				.TextStyle(&style)
				.ToolTipText(LOCTEXT("Reimport_ToolTip", "Reimports the PopcornFXEffect."))]
		];

	m_AttributeListPty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPopcornFXEmitterComponent, AttributeList));
	if (!PK_VERIFY(IsValidHandle(m_AttributeListPty)))
	{
		UE_LOG(LogPopcornFXDetailsEmitterComponent, Error, TEXT("Can't retrieve attribute list property!"));
		return;
	}
	m_SamplersDescPty = m_AttributeListPty->GetChildHandle(GET_MEMBER_NAME_CHECKED(UPopcornFXAttributeList, m_Samplers));
	if (!PK_VERIFY(IsValidHandle(m_SamplersDescPty)))
	{
		UE_LOG(LogPopcornFXDetailsEmitterComponent, Error, TEXT("Can't retrieve samplers description property!"));
		return;
	}

	m_SamplersPty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPopcornFXEmitterComponent, Samplers));
	if (!PK_VERIFY(IsValidHandle(m_SamplersPty)))
	{
		UE_LOG(LogPopcornFXDetailsEmitterComponent, Error, TEXT("Can't retrieve samplers property!"));
		return;
	}

	TSharedPtr<IPropertyHandle> effectPty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPopcornFXEmitterComponent, Effect));
	if (!PK_VERIFY(IsValidHandle(effectPty)))
	{
		UE_LOG(LogPopcornFXDetailsEmitterComponent, Error, TEXT("Can't retrieve effect property!"));
		return;
	}
	effectPty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsEmitterComponent::RebuildIFN));

	// We are in the blueprint editor, the object being edited is a temporary UI only instance, modifying attributes will have no effect on the viewport
	if (m_BeingCustomized[0]->GetName().EndsWith("_GEN_VARIABLE"))
	{
		// Use this to find the actual object in the viewport 
		//TArray<UObject*> instances;
		//m_BeingCustomized[0]->GetArchetypeInstances(instances);
		//TODO: try this
		//DetailLayout.GetDetailsView()->SetObject(instances[0]);
		//DetailLayout.GetDetailsViewSharedPtr()->SetObject(instances[0]);
	}

	Rebuild();
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
