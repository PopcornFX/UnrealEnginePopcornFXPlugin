//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "MovieSceneNameableTrack.h"
#if (ENGINE_MINOR_VERSION >= 26)
#include "Compilation/IMovieSceneTrackTemplateProducer.h"
#endif // (ENGINE_MINOR_VERSION >= 26)
#include "PopcornFXSDK.h"
#include "PopcornFXPlayTrack.generated.h"

/**
 * Handles Start/Stop of emitters.
 */
UCLASS(MinimalAPI)
class UPopcornFXPlayTrack
	: public UMovieSceneNameableTrack
	, public IMovieSceneTrackTemplateProducer
{
	GENERATED_UCLASS_BODY()
public:
	// UMovieSceneTrack interface
	virtual UMovieSceneSection			*CreateNewSection() override;
	FMovieSceneEvalTemplatePtr			CreateTemplateForSection(const UMovieSceneSection &inSection) const override;

	virtual void	RemoveAllAnimationData() override { Sections.Empty(); }
	virtual bool	HasSection(const UMovieSceneSection &section) const override { return Sections.Contains(&section); }
	virtual void	AddSection(UMovieSceneSection &section) override { Sections.Add(&section); }
	virtual void	RemoveSection(UMovieSceneSection &section) override { Sections.Remove(&section); }
	virtual bool	IsEmpty() const override { return Sections.Num() == 0; }

	void					AddNewSection(FFrameNumber sectionTime);

	virtual const TArray<UMovieSceneSection*>	&GetAllSections() const override { return Sections; }

#if WITH_EDITORONLY_DATA
	virtual FText GetDefaultDisplayName() const override;
#endif
private:
	UPROPERTY()
	TArray<UMovieSceneSection*>	Sections;
};
