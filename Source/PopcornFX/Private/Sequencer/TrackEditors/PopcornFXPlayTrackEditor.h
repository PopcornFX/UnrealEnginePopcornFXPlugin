//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "Runtime/Launch/Resources/Version.h"
#include "MovieSceneTrackEditor.h"

/**
 * Track editor for effect attributes.
 */
class FPopcornFXPlayTrackEditor : public FMovieSceneTrackEditor
{
public:
	FPopcornFXPlayTrackEditor(TSharedRef<ISequencer> InSequencer);
	virtual ~FPopcornFXPlayTrackEditor() { }

	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer);

public:
	// ISequencerTrackEditor interface
	virtual void							BuildObjectBindingTrackMenu(FMenuBuilder &menuBuilder, const TArray<FGuid> &objectBindings, const UClass *objectClass) override;
	virtual TSharedRef<ISequencerSection>	MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;
	virtual bool							SupportsType(TSubclassOf<UMovieSceneTrack> Type) const override;

	void									AddStateKey(TArray<FGuid> objectBindings);

	FKeyPropertyResult						AddStateKeyInternal(FFrameNumber keyTime, UObject *object);
};


/**
*	Handles rendering of popcornfx play/stop sections.
*/
class	FPopcornFXPlaySection
	: public ISequencerSection
	, public TSharedFromThis<FPopcornFXPlaySection>
{
public:
	FPopcornFXPlaySection(UMovieSceneSection &inSection, TSharedRef<ISequencer> inOwningSequencer);
	~FPopcornFXPlaySection() { }

	// ISequencerSection interface
	virtual UMovieSceneSection			*GetSectionObject() override { return &m_Section; }
	virtual float						GetSectionHeight() const override { return 20.0f; }
	virtual int32						OnPaintSection(FSequencerSectionPainter& InPainter) const override;
	virtual bool						SectionIsResizable() const override { return false; }

private:
	UMovieSceneSection		&m_Section;
	TWeakPtr<ISequencer>	m_OwningSequencerPtr;
};

#endif // WITH_EDITOR
