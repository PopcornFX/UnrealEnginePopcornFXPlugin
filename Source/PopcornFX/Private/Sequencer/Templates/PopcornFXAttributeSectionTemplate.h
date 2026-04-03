//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "Evaluation/MovieSceneParameterTemplate.h"
#include "PopcornFXAttributeSectionTemplate.generated.h"

USTRUCT()
struct FPopcornFXAttributeSectionTemplate : public FMovieSceneParameterSectionTemplate
{
	GENERATED_BODY()

	FPopcornFXAttributeSectionTemplate() {}
	FPopcornFXAttributeSectionTemplate(const UMovieSceneParameterSection &section, const class UPopcornFXAttributeTrack &track);

private:
	virtual UScriptStruct	&GetScriptStructImpl() const override { return *StaticStruct(); }

	virtual void			Evaluate(const FMovieSceneEvaluationOperand &operand,
		const FMovieSceneContext &context,
		const FPersistentEvaluationData &persistentData,
		FMovieSceneExecutionTokens &executionTokens) const override;
};
