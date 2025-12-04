//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "Sequencer/Tracks/PopcornFXAttributeTrack.h"
#include "Sequencer/Templates/PopcornFXAttributeSectionTemplate.h"

#include "PopcornFXPlugin.h"

#include "MovieSceneCommonHelpers.h"

#include "Curves/RichCurve.h"

#define LOCTEXT_NAMESPACE "PopcornFXAttributeTrack"

UPopcornFXAttributeTrack::UPopcornFXAttributeTrack(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	TrackTint = FColor(204, 70, 221, 65);
#endif
}

FMovieSceneEvalTemplatePtr	UPopcornFXAttributeTrack::CreateTemplateForSection(const UMovieSceneSection& InSection) const
{
	return FPopcornFXAttributeSectionTemplate(*CastChecked<UMovieSceneParameterSection>(&InSection), *this);
}

UMovieSceneSection	*UPopcornFXAttributeTrack::CreateNewSection()
{
	return NewObject<UMovieSceneParameterSection>(this, UMovieSceneParameterSection::StaticClass(), NAME_None, RF_Transactional);
}

#if WITH_EDITORONLY_DATA
FText	UPopcornFXAttributeTrack::GetDefaultDisplayName() const
{
	return LOCTEXT("DisplayName", "Effect Attributes");
}
#endif

void	UPopcornFXAttributeTrack::AddScalarAttributeKey(FName attrName, FFrameNumber position, float value)
{
	UMovieSceneParameterSection	*nearestSection = Cast<UMovieSceneParameterSection>(MovieSceneHelpers::FindNearestSectionAtTime(Sections, position));

	if (nearestSection == null)
	{
		nearestSection = Cast<UMovieSceneParameterSection>(CreateNewSection());
		nearestSection->SetRange(TRange<FFrameNumber>::Inclusive(position, position));
		Sections.Add(nearestSection);
	}

	nearestSection->AddScalarParameterKey(attrName, position, value);
}

void	UPopcornFXAttributeTrack::AddVectorAttributeKey(FName attrName, FFrameNumber position, FVector value)
{
	UMovieSceneParameterSection	*nearestSection = Cast<UMovieSceneParameterSection>(MovieSceneHelpers::FindNearestSectionAtTime(Sections, position));

	if (nearestSection == null)
	{
		nearestSection = Cast<UMovieSceneParameterSection>(CreateNewSection());
		nearestSection->SetRange(TRange<FFrameNumber>::Inclusive(position, position));
		Sections.Add(nearestSection);
	}

	nearestSection->AddVectorParameterKey(attrName, position, value);
}

void	UPopcornFXAttributeTrack::AddColorAttributeKey(FName attrName, FFrameNumber position, FLinearColor value)
{
	UMovieSceneParameterSection	*nearestSection = Cast<UMovieSceneParameterSection>(MovieSceneHelpers::FindNearestSectionAtTime(Sections, position));

	if (nearestSection == null)
	{
		nearestSection = Cast<UMovieSceneParameterSection>(CreateNewSection());
		nearestSection->SetRange(TRange<FFrameNumber>::Inclusive(position, position));
		Sections.Add(nearestSection);
	}

	nearestSection->AddColorParameterKey(attrName, position, value);
}

#undef LOCTEXT_NAMESPACE
