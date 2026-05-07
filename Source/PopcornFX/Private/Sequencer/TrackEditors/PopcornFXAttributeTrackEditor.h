//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXPlugin.h"
#include "MovieSceneTrackEditor.h"

/**
 * Track editor for effect attributes.
 */
class FPopcornFXAttributeTrackEditor : public FMovieSceneTrackEditor
{
public:
	/** Constructor. */
	FPopcornFXAttributeTrackEditor(TSharedRef<ISequencer> InSequencer);
	virtual ~FPopcornFXAttributeTrackEditor() { }

	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer);

	// ISequencerTrackEditor interface
	virtual TSharedPtr<SWidget>				BuildOutlinerEditWidget( const FGuid &objectBinding, UMovieSceneTrack *track, const FBuildEditWidgetParams &params) override;
	virtual TSharedRef<ISequencerSection>	MakeSectionInterface(UMovieSceneSection &sectionObject, UMovieSceneTrack &track, FGuid objectBinding) override;
	virtual bool							SupportsType(TSubclassOf<UMovieSceneTrack> type) const override;

	void					BuildObjectBindingTrackMenu(FMenuBuilder &menuBuilder, const TArray<FGuid> &objectBindings, const UClass *objectClass) override;

private:
	TSharedRef<SWidget>		OnGetAddParameterMenuContent(FGuid objectBinding, class UPopcornFXAttributeTrack *attributeTrack);

	bool	CanAddAttributeTrack(FGuid objectBinding);

	void	AddAttributeTrack(TArray<FGuid> objectBindings);

	void	AddScalarAttribute(FGuid objectBinding,
		UPopcornFXAttributeTrack *attributeTrack,
		FName attrName,
		u32 attrIndex);

	void	AddVectorAttribute(FGuid objectBinding,
		UPopcornFXAttributeTrack *attributeTrack,
		FName attrName,
		u32 attrIndex);

	void	AddColorAttribute(FGuid objectBinding,
		UPopcornFXAttributeTrack *attributeTrack,
		FName attrName,
		u32 attrIndex);
};

#endif // WITH_EDITOR
