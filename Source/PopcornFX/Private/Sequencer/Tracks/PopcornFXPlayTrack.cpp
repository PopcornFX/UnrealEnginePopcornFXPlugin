//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "Sequencer/Tracks/PopcornFXPlayTrack.h"
#include "PopcornFXPlugin.h"
#include "Sequencer/Templates/PopcornFXPlaySectionTemplate.h"

#include "MovieSceneCommonHelpers.h"

#define LOCTEXT_NAMESPACE "PopcornFXPlayTrack"

//----------------------------------------------------------------------------

UPopcornFXPlayTrack::UPopcornFXPlayTrack(const FObjectInitializer &objectInitializer)
	: Super(objectInitializer)
{
#if WITH_EDITORONLY_DATA
	TrackTint = FColor(255, 255, 255, 160);
#endif
}

//----------------------------------------------------------------------------

FMovieSceneEvalTemplatePtr	UPopcornFXPlayTrack::CreateTemplateForSection(const UMovieSceneSection &inSection) const
{
	return FPopcornFXPlaySectionTemplate(*CastChecked<UMovieSceneParticleSection>(&inSection));
}

//----------------------------------------------------------------------------

UMovieSceneSection	*UPopcornFXPlayTrack::CreateNewSection()
{
	return NewObject<UMovieSceneParticleSection>(this);
}

//----------------------------------------------------------------------------

void	UPopcornFXPlayTrack::AddNewSection(FFrameNumber sectionTime)
{
	if (MovieSceneHelpers::FindSectionAtTime(Sections, sectionTime) == null)
	{
		UMovieSceneParticleSection	*newSection = Cast<UMovieSceneParticleSection>(CreateNewSection());
		newSection->SetRange(TRange<FFrameNumber>::Inclusive(sectionTime, sectionTime));
		Sections.Add(newSection);
	}
}

//----------------------------------------------------------------------------

#if WITH_EDITORONLY_DATA
FText	UPopcornFXPlayTrack::GetDefaultDisplayName() const
{
	return LOCTEXT("DisplayName", "Start/Stop/Toggle Emitter");
}
#endif

//----------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
