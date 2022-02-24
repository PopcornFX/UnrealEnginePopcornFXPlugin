//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#if WITH_EDITOR

#include "PopcornFXPlayTrackEditor.h"

#include "PopcornFXPlugin.h"

#include "SequencerUtilities.h"

#include "KeyDrawParams.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "PopcornFXEmitter.h"
#include "PopcornFXEmitterComponent.h"

#include "Sequencer/Tracks/PopcornFXPlayTrack.h"
#include "Sequencer/Templates/PopcornFXPlaySectionTemplate.h"
#include "EditorStyleSet.h"

#include "CommonMovieSceneTools.h"

#include "Sections/MovieSceneParticleSection.h"
#include "SequencerSectionPainter.h"

#define LOCTEXT_NAMESPACE "PopcornFXPlayTrackEditor"

//----------------------------------------------------------------------------
//
//	Play Track Section Interface (handles rendering of play sections)
//
//----------------------------------------------------------------------------

extern UPopcornFXEmitterComponent	*TryGetEmitterComponent(UObject *object);

//----------------------------------------------------------------------------

FPopcornFXPlaySection::FPopcornFXPlaySection(UMovieSceneSection &inSection, TSharedRef<ISequencer> inOwningSequencer)
:	m_Section(inSection)
,	m_OwningSequencerPtr(inOwningSequencer)
{
}

//----------------------------------------------------------------------------

int32	FPopcornFXPlaySection::OnPaintSection(FSequencerSectionPainter &inPainter) const
{
	return inPainter.LayerId + 1;
}

//----------------------------------------------------------------------------
//
//	Play Track Editor
//
//----------------------------------------------------------------------------

FPopcornFXPlayTrackEditor::FPopcornFXPlayTrackEditor(TSharedRef<ISequencer> InSequencer)
:	FMovieSceneTrackEditor(InSequencer)
{
}

//----------------------------------------------------------------------------

TSharedRef<ISequencerTrackEditor>	FPopcornFXPlayTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> owningSequencer)
{
	return MakeShareable(new FPopcornFXPlayTrackEditor(owningSequencer));
}

//----------------------------------------------------------------------------

TSharedRef<ISequencerSection>	FPopcornFXPlayTrackEditor::MakeSectionInterface(UMovieSceneSection &sectionObject, UMovieSceneTrack &track, FGuid objectBinding)
{
	check(SupportsType(sectionObject.GetOuter()->GetClass()));

	const TSharedPtr<ISequencer>	owningSequencer = GetSequencer();
	return MakeShareable(new FPopcornFXPlaySection(sectionObject, owningSequencer.ToSharedRef()));
}

//----------------------------------------------------------------------------

void	FPopcornFXPlayTrackEditor::BuildObjectBindingTrackMenu(FMenuBuilder &menuBuilder, const TArray<FGuid> &objectBindings, const UClass *objectClass)
{
	if (objectClass->IsChildOf(APopcornFXEmitter::StaticClass()) ||
		objectClass->IsChildOf(UPopcornFXEmitterComponent::StaticClass()))
	{
		const TSharedPtr<ISequencer>	parentSequencer = GetSequencer();

		menuBuilder.AddMenuEntry(
			LOCTEXT("AddPlayTrack", "Emitter Play Track"),
			LOCTEXT("AddPlayTrackTooltip", "Adds a track for controlling emitter Start/Stop."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FPopcornFXPlayTrackEditor::AddStateKey, objectBindings))
		);
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXPlayTrackEditor::AddStateKey(TArray<FGuid> objectBindings)
{
	TSharedPtr<ISequencer>	sequencerPtr = GetSequencer();
	if (!sequencerPtr.IsValid())
		return;

	for (FGuid objectBinding : objectBindings)
	{
		UObject	*object = sequencerPtr->FindSpawnedObjectOrTemplate(objectBinding);

		if (object != null)
			AnimatablePropertyChanged(FOnKeyProperty::CreateRaw(this, &FPopcornFXPlayTrackEditor::AddStateKeyInternal, object));
	}

}

//----------------------------------------------------------------------------

FKeyPropertyResult	FPopcornFXPlayTrackEditor::AddStateKeyInternal(FFrameNumber keyTime, UObject *object)
{
	FKeyPropertyResult			keyPropertyResult;
	FFindOrCreateHandleResult	handleResult = FindOrCreateHandleToObject(object);
	FGuid						objectHandle = handleResult.Handle;

	keyPropertyResult.bHandleCreated |= handleResult.bWasCreated;

	if (objectHandle.IsValid())
	{
		FFindOrCreateTrackResult	trackResult = FindOrCreateTrackForObject(objectHandle, UPopcornFXPlayTrack::StaticClass());
		UMovieSceneTrack			*track = trackResult.Track;

		keyPropertyResult.bTrackCreated |= trackResult.bWasCreated;
		if (keyPropertyResult.bTrackCreated && ensure(track != null))
		{
			UPopcornFXPlayTrack	*playTrack = Cast<UPopcornFXPlayTrack>(track);
			playTrack->AddNewSection(keyTime);
			playTrack->SetDisplayName(LOCTEXT("TrackName", "Start/Stop/Toggle Emitter"));
			keyPropertyResult.bTrackModified = true;
		}
	}
	return keyPropertyResult;
}

//----------------------------------------------------------------------------

bool	FPopcornFXPlayTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> type) const
{
	return type == UPopcornFXPlayTrack::StaticClass();
}

//----------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR
