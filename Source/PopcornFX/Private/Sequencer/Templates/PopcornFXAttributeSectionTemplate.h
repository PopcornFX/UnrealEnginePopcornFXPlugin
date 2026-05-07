//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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
