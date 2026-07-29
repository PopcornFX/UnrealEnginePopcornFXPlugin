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
#include "IPropertyUtilities.h"

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

void	FPopcornFXDetailsEmitterComponent::RebuildIFN()
{
	if (PK_VERIFY(m_PropertyUtilities.IsValid()))
		m_PropertyUtilities->ForceRefresh();
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

	TSharedPtr<IPropertyHandle> effectPty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPopcornFXEmitterComponent, Effect));
	if (!PK_VERIFY(IsValidHandle(effectPty)))
	{
		UE_LOG(LogPopcornFXDetailsEmitterComponent, Error, TEXT("Can't retrieve effect property!"));
		return;
	}
	effectPty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsEmitterComponent::RebuildIFN));
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
