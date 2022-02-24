//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#if WITH_EDITOR

#include "PopcornFXAttributeTrackEditor.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXSDK.h"

#include "SequencerUtilities.h"
#include "Sections/MovieSceneParameterSection.h"

#include "PopcornFXEmitter.h"
#include "PopcornFXEmitterComponent.h"
#include "PopcornFXAttributeList.h"
#include "PopcornFXAttributeFunctions.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "Sequencer/Sections/PopcornFXAttributeSection.h"
#include "Sequencer/Tracks/PopcornFXAttributeTrack.h"

#include <pk_maths/include/pk_maths.h>
#include <pk_kernel/include/kr_base_types.h>

#define LOCTEXT_NAMESPACE "PopcornFXAttributeTrackEditor"

static FName	trackName("PopcornFXAttribute");

//----------------------------------------------------------------------------

FPopcornFXAttributeTrackEditor::FPopcornFXAttributeTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FMovieSceneTrackEditor(InSequencer)
{
}

//----------------------------------------------------------------------------

TSharedRef<ISequencerTrackEditor>	FPopcornFXAttributeTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> owningSequencer)
{
	return MakeShareable(new FPopcornFXAttributeTrackEditor(owningSequencer));
}

//----------------------------------------------------------------------------

TSharedRef<ISequencerSection>	FPopcornFXAttributeTrackEditor::MakeSectionInterface(UMovieSceneSection &sectionObject, UMovieSceneTrack &track, FGuid objectBinding)
{
	UMovieSceneParameterSection	*parameterSection = Cast<UMovieSceneParameterSection>(&sectionObject);
	checkf(parameterSection != nullptr, TEXT("Unsupported section type."));

	return MakeShareable(new FPopcornFXAttributeSection(*parameterSection, FText::FromName(parameterSection->GetFName())));
}

//----------------------------------------------------------------------------

TSharedPtr<SWidget>	FPopcornFXAttributeTrackEditor::BuildOutlinerEditWidget(const FGuid &objectBinding, UMovieSceneTrack *track, const FBuildEditWidgetParams &params)
{
	UPopcornFXAttributeTrack	*attributeTrack = Cast<UPopcornFXAttributeTrack>(track);

	// Create a container edit box
	return FSequencerUtilities::MakeAddButton(LOCTEXT("AttributeText", "Attribute"),
		FOnGetContent::CreateSP(this, &FPopcornFXAttributeTrackEditor::OnGetAddParameterMenuContent, objectBinding, attributeTrack),
		params.NodeIsHovered,
		GetSequencer());
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeTrackEditor::BuildObjectBindingTrackMenu(FMenuBuilder &menuBuilder, const TArray<FGuid> &objectBindings, const UClass *objectClass)
{
	if (objectClass->IsChildOf(APopcornFXEmitter::StaticClass()) ||
		objectClass->IsChildOf(UPopcornFXEmitterComponent::StaticClass()))
	{
		const TSharedPtr<ISequencer>	parentSequencer = GetSequencer();

		menuBuilder.AddMenuEntry(
			LOCTEXT( "AddAttributeTrack", "Effect Attributes Track" ),
			LOCTEXT( "AddAttributeTrackTooltip", "Adds a track for controlling attribute values." ),
			FSlateIcon(),
			FUIAction
			(
				FExecuteAction::CreateSP(this, &FPopcornFXAttributeTrackEditor::AddAttributeTrack, objectBindings),
				FCanExecuteAction::CreateSP(this, &FPopcornFXAttributeTrackEditor::CanAddAttributeTrack, objectBindings[0])
			));
	}
}

//----------------------------------------------------------------------------

bool	FPopcornFXAttributeTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> type) const
{
	return type == UPopcornFXAttributeTrack::StaticClass();
}

//----------------------------------------------------------------------------

TSharedRef<SWidget>	FPopcornFXAttributeTrackEditor::OnGetAddParameterMenuContent(FGuid objectBinding, UPopcornFXAttributeTrack *attributeTrack)
{
	FMenuBuilder	addParameterMenuBuilder(true, null);

	TSharedPtr<ISequencer>	sequencerPtr = GetSequencer();
	UObject						*obj = sequencerPtr.IsValid() ? sequencerPtr->FindSpawnedObjectOrTemplate(objectBinding) : null;
	APopcornFXEmitter			*emitter = Cast<APopcornFXEmitter>(obj);
	UPopcornFXEmitterComponent	*emitterComponent = emitter != null ? emitter->PopcornFXEmitterComponent : Cast<UPopcornFXEmitterComponent>(obj);

	if (emitterComponent == null ||
		emitterComponent->Effect == null)
		return addParameterMenuBuilder.MakeWidget();

	UPopcornFXAttributeList	*attributeList = emitterComponent->GetAttributeList();
	PK_ASSERT(attributeList != null);

	if (!attributeList->Valid() ||
		!attributeList->IsUpToDate(emitterComponent->Effect))
		return addParameterMenuBuilder.MakeWidget();

	const u32	attrCount = attributeList->AttributeCount();
	for (u32 iAttr = 0; iAttr < attrCount; ++iAttr)
	{
		const FPopcornFXAttributeDesc	*desc = attributeList->GetAttributeDesc(iAttr);

		if (!PK_VERIFY(desc != null))
			continue;
		const FName		attrName = desc->m_AttributeFName;
		const u32		attrType = desc->m_AttributeType;
		FUIAction		attrAction;
		bool			supportedType = true;
		switch (attrType)
		{
			case	PopcornFX::EBaseTypeID::BaseType_Float:
				attrAction.ExecuteAction = FExecuteAction::CreateSP(this, &FPopcornFXAttributeTrackEditor::AddScalarAttribute, objectBinding, attributeTrack, attrName, iAttr);
				break;
			case	PopcornFX::EBaseTypeID::BaseType_Float3:
				attrAction.ExecuteAction = FExecuteAction::CreateSP(this, &FPopcornFXAttributeTrackEditor::AddVectorAttribute, objectBinding, attributeTrack, attrName, iAttr);
				break;
			case	PopcornFX::EBaseTypeID::BaseType_Float4:
				attrAction.ExecuteAction = FExecuteAction::CreateSP(this, &FPopcornFXAttributeTrackEditor::AddColorAttribute, objectBinding, attributeTrack, attrName, iAttr);
				break;
			default: // Every other types are not supported right now
				supportedType = false;
				break;
		}
		if (supportedType)
			addParameterMenuBuilder.AddMenuEntry(FText::FromName(attrName), FText(), FSlateIcon(), attrAction);
	}
	return addParameterMenuBuilder.MakeWidget();
}

//----------------------------------------------------------------------------

bool	FPopcornFXAttributeTrackEditor::CanAddAttributeTrack(FGuid objectBinding)
{
	return GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene()->FindTrack(UPopcornFXAttributeTrack::StaticClass(), objectBinding, trackName) == null;
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeTrackEditor::AddAttributeTrack(TArray<FGuid> objectBindings)
{
	const FScopedTransaction	transaction(LOCTEXT("AddPopcornFXAttributeTrack", "Add PopcornFX attribute track"));

	for (FGuid objectBinding : objectBindings)
		FindOrCreateTrackForObject(objectBinding, UPopcornFXAttributeTrack::StaticClass(), trackName, true);
	GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeTrackEditor::AddScalarAttribute(FGuid objectBinding, UPopcornFXAttributeTrack *attributeTrack, FName attrName, u32 attrIndex)
{
	const FScopedTransaction	tr(LOCTEXT("AddFloatAttribute", "Add float attribute"));

	for (TWeakObjectPtr<> object : GetSequencer()->FindObjectsInCurrentSequence(objectBinding))
	{
		APopcornFXEmitter			*emitter = Cast<APopcornFXEmitter>(object.Get());
		UPopcornFXEmitterComponent	*emitterComponent = emitter != null ? emitter->PopcornFXEmitterComponent : Cast<UPopcornFXEmitterComponent>(object.Get());

		if (emitterComponent == null ||
			emitterComponent->Effect == null)
			continue;

		float		defaultValue;
		if (!UPopcornFXAttributeFunctions::GetAttributeAsFloat(emitterComponent, attrIndex, defaultValue, false))
			continue;

		attributeTrack->Modify();
		attributeTrack->AddScalarAttributeKey(attrName, GetTimeForKey(), defaultValue);
	}
	GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeTrackEditor::AddVectorAttribute(FGuid objectBinding, UPopcornFXAttributeTrack *attributeTrack, FName attrName, u32 attrIndex)
{
	const FScopedTransaction	tr(LOCTEXT("AddVectorAttribute", "Add vector attribute"));

	for (TWeakObjectPtr<> object : GetSequencer()->FindObjectsInCurrentSequence(objectBinding))
	{
		APopcornFXEmitter			*emitter = Cast<APopcornFXEmitter>(object.Get());
		UPopcornFXEmitterComponent	*emitterComponent = emitter != null ? emitter->PopcornFXEmitterComponent : Cast<UPopcornFXEmitterComponent>(object.Get());

		if (emitterComponent == null ||
			emitterComponent->Effect == null)
			continue;

		FVector		defaultValue;
		if (!UPopcornFXAttributeFunctions::GetAttributeAsVector(emitterComponent, attrIndex, defaultValue, false))
			continue;

		attributeTrack->Modify();
		attributeTrack->AddVectorAttributeKey(attrName, GetTimeForKey(), defaultValue);
	}
	GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeTrackEditor::AddColorAttribute(FGuid objectBinding, UPopcornFXAttributeTrack *attributeTrack, FName attrName, u32 attrIndex)
{
	const FScopedTransaction	tr(LOCTEXT("AddVectorAttribute", "Add vector attribute"));

	for (TWeakObjectPtr<> object : GetSequencer()->FindObjectsInCurrentSequence(objectBinding))
	{
		APopcornFXEmitter			*emitter = Cast<APopcornFXEmitter>(object.Get());
		UPopcornFXEmitterComponent	*emitterComponent = emitter != null ? emitter->PopcornFXEmitterComponent : Cast<UPopcornFXEmitterComponent>(object.Get());
		if (emitterComponent == null ||
			emitterComponent->Effect == null)
			continue;

		FLinearColor		defaultValue;
		if (!UPopcornFXAttributeFunctions::GetAttributeAsLinearColor(emitterComponent, attrIndex, defaultValue, false))
			continue;

		attributeTrack->Modify();
		attributeTrack->AddColorAttributeKey(attrName, GetTimeForKey(), defaultValue);
	}
	GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
}

//----------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR
