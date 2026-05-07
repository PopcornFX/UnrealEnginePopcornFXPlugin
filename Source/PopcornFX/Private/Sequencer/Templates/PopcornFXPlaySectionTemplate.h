//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "Evaluation/MovieSceneEvalTemplate.h"
#include "Sections/MovieSceneParticleSection.h"
#include "PopcornFXPlaySectionTemplate.generated.h"

//----------------------------------------------------------------------------
//
//	EPopcornFXPlayStateKey
//
//----------------------------------------------------------------------------

UENUM()
enum class EPopcornFXPlayStateKey : uint8
{
	Start = 0,
	Stop = 1,
	Toggle = 2,
};

//----------------------------------------------------------------------------
//
//	FPopcornFXPlaySectionTemplate
//
//----------------------------------------------------------------------------

USTRUCT()
struct FPopcornFXPlaySectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()

	FPopcornFXPlaySectionTemplate() {}
	FPopcornFXPlaySectionTemplate(const class UMovieSceneParticleSection &section);

	UPROPERTY()
	FMovieSceneParticleChannel	Keys;

private:
	virtual UScriptStruct	&GetScriptStructImpl() const override { return *StaticStruct(); }

	virtual void			Evaluate(const FMovieSceneEvaluationOperand &operand,
		const FMovieSceneContext &context,
		const FPersistentEvaluationData &persistentData,
		FMovieSceneExecutionTokens &executionTokens) const override;
};
