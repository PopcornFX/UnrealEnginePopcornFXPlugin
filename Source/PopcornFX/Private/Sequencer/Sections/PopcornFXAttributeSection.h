//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"

#include "ISequencerSection.h"

// Hugo: NOTE -- This is ~exact copy of ParameterSection.cpp/h which is private right now
// Ideally, thirdparty could rely on the default implementation

/**
 * A movie scene section for effect Attributes.
 */
class FPopcornFXAttributeSection : public FSequencerSection
{
public:

	FPopcornFXAttributeSection(UMovieSceneSection& InSectionObject, const FText& SectionName)
		: FSequencerSection(InSectionObject) { }

public:
	//~ ISequencerSection interface
	virtual bool	RequestDeleteCategory(const TArray<FName>& CategoryNamePath) override;
	virtual bool	RequestDeleteKeyArea(const TArray<FName>& KeyAreaNamePath) override;
};

#endif //WITH_EDITOR
