//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsEmitterComponent.h"

#include "Assets/PopcornFXEffect.h"
#include "PopcornFXEmitterComponent.h"
#include "Editor/EditorHelpers.h"
#include "PopcornFXSDK.h"

#include "DetailLayoutBuilder.h"
#include "IDetailsView.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "EditorReimportHandler.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Input/SButton.h"

#include "SAssetDropTarget.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXDetailsEmitterComponent"

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
	const TArray< TWeakObjectPtr<UObject> >		&objects = m_SelectedObjectsList;
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
	const TArray< TWeakObjectPtr<UObject> >		&objects = m_SelectedObjectsList;
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
		if (emitters[i]->IsRegistered())
			emitters[i]->StartEmitter();
	return FReply::Handled();
}

//----------------------------------------------------------------------------

FReply		FPopcornFXDetailsEmitterComponent::OnStopEmitter()
{
	TArray<UPopcornFXEmitterComponent*> emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
		if (emitters[i]->IsRegistered())
			emitters[i]->StopEmitter();
	return FReply::Handled();
}

//----------------------------------------------------------------------------

FReply		FPopcornFXDetailsEmitterComponent::OnKillParticles()
{
	TArray<UPopcornFXEmitterComponent*> emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
		if (emitters[i]->IsRegistered())
			emitters[i]->KillParticles();
	return FReply::Handled();
}

//----------------------------------------------------------------------------

FReply		FPopcornFXDetailsEmitterComponent::OnRestartEmitter()
{
	TArray<UPopcornFXEmitterComponent*> emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
		if (emitters[i]->IsRegistered())
			emitters[i]->RestartEmitter(true);
	return FReply::Handled();
}

//----------------------------------------------------------------------------

bool	FPopcornFXDetailsEmitterComponent::IsStartEnabled() const
{
	TArray<UPopcornFXEmitterComponent*> emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
		if (!emitters[i]->IsEmitterStarted())
			return true;
	return false;
}

//----------------------------------------------------------------------------

bool	FPopcornFXDetailsEmitterComponent::IsStopEnabled() const
{
	TArray<UPopcornFXEmitterComponent*> emitters;
	GatherEmitters(emitters);
	for (int32 i = 0; i < emitters.Num(); ++i)
		if (emitters[i]->IsEmitterEmitting())
			return true;
	return false;
}

//----------------------------------------------------------------------------

FReply	FPopcornFXDetailsEmitterComponent::OnReloadEffect()
{
	TArray<UPopcornFXEffect*>		effects;
	GatherEffects(effects);
	for (int32 i = 0; i < effects.Num(); ++i)
	{
		effects[i]->ReloadFile();
	}
	return FReply::Handled();
}

//----------------------------------------------------------------------------

FReply	FPopcornFXDetailsEmitterComponent::OnReimportEffect()
{
	TArray<UPopcornFXEffect*>		effects;
	GatherEffects(effects);
	for (int32 i = 0; i < effects.Num(); ++i)
	{
		FReimportManager::Instance()->Reimport(effects[i]);
	}
	return FReply::Handled();
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEmitterComponent::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
	m_SelectedObjectsList = DetailLayout.GetDetailsViewSharedPtr()->GetSelectedObjects();
#else
	m_SelectedObjectsList = DetailLayout.GetDetailsView()->GetSelectedObjects();
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)

	IDetailCategoryBuilder	&fxEditorCategory = DetailLayout.EditCategory("PopcornFX Emitter");

	// Add some buttons
	fxEditorCategory.AddCustomRow(LOCTEXT("Emitter Actions", "Emitter Actions"), false)
	.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("EmitterActions", "Emitter Actions"))
		]
	.ValueContent()
		.MinDesiredWidth(125.0f * 3.0f)
		.MaxDesiredWidth(125.0f * 4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
				[SNew(SButton)
				.Text(LOCTEXT("Start", "Start"))
				.ToolTipText(LOCTEXT("Start_ToolTip", "Starts the emitter. Available if the emitter is not \"IsEmitterStarted\"."))
				.OnClicked(this, &FPopcornFXDetailsEmitterComponent::OnStartEmitter)
				.IsEnabled(this, &FPopcornFXDetailsEmitterComponent::IsStartEnabled)]
			+ SHorizontalBox::Slot()
				[SNew(SButton)
				.Text(LOCTEXT("Stop", "Stop"))
				.ToolTipText(LOCTEXT("Stop_ToolTip", "Stops the emitter. Available if the emitter is \"IsEmitterEmitting\"."))
				.OnClicked(this, &FPopcornFXDetailsEmitterComponent::OnStopEmitter)
				.IsEnabled(this, &FPopcornFXDetailsEmitterComponent::IsStopEnabled)
				]
			+ SHorizontalBox::Slot()
				[SNew(SButton)
				.Text(LOCTEXT("Restart", "Restart"))
				.ToolTipText(LOCTEXT("Restart_ToolTip", "Terminates then starts the emitter."))
				.OnClicked(this, &FPopcornFXDetailsEmitterComponent::OnRestartEmitter)]
			+ SHorizontalBox::Slot()
				[SNew(SButton)
				.Text(LOCTEXT("Kill Particles", "Kill Particles"))
				.ToolTipText(LOCTEXT("kill_ToolTip", "Kill emitter's particles and stop the emitter."))
				.OnClicked(this, &FPopcornFXDetailsEmitterComponent::OnKillParticles)]
		]
	;
	// Add some buttons
	fxEditorCategory.AddCustomRow(LOCTEXT("Effect Actions", "Effect Actions"), false)
		.NameContent()
		[
			SNew(STextBlock)
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
			.ToolTipText(LOCTEXT("Reload_ToolTip", "Reloads the PopcornFXEffect, will kill all particles related to this effect."))]
			+ SHorizontalBox::Slot()
			[SNew(SButton)
			.Text(LOCTEXT("Reimport", "Reimport")).OnClicked(this, &FPopcornFXDetailsEmitterComponent::OnReimportEffect)
			.ToolTipText(LOCTEXT("Reimport_ToolTip", "Reimports the PopcornFXEffect."))]
		]
	;
}

//----------------------------------------------------------------------------

TOptional<float>	FPopcornFXDetailsEmitterComponent::OnGetValue(float field) const
{
	return field;
}

//----------------------------------------------------------------------------

void	FPopcornFXDetailsEmitterComponent::OnSetValue(float newValue, ETextCommit::Type commitInfo, float *field)
{
	*field = newValue;
}

//----------------------------------------------------------------------------

int32	FPopcornFXDetailsEmitterComponent::GetVectorDimension(int32 field) const
{
	return field;
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
