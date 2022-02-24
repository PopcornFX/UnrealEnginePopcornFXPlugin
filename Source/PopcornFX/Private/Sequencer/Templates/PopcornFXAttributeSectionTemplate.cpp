//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "Sequencer/Templates/PopcornFXAttributeSectionTemplate.h"

#include "PopcornFXAttributeList.h"
#include "Sequencer/Tracks/PopcornFXAttributeTrack.h"
#include "PopcornFXSDK.h"

#include "PopcornFXAttributeFunctions.h"
#include "PopcornFXEmitter.h"
#include "PopcornFXEmitterComponent.h"

#include "IMovieScenePlayer.h"

DECLARE_CYCLE_STAT(TEXT("PopcornFX Attribute Track Token Execute"), MovieSceneEval_PopcornFXAttributeTrack_TokenExecute, STATGROUP_MovieSceneEval);

struct FPopcornFXAttributePreAnimatedToken : IMovieScenePreAnimatedToken
{
	FPopcornFXAttributePreAnimatedToken() { }

	FPopcornFXAttributePreAnimatedToken(FPopcornFXAttributePreAnimatedToken&&) = default;
	FPopcornFXAttributePreAnimatedToken& operator=(FPopcornFXAttributePreAnimatedToken&&) = default;

#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27)
	virtual void	RestoreState(UObject &object, const UE::MovieScene::FRestoreStateParams &params) override
#else
	virtual void	RestoreState(UObject &object, IMovieScenePlayer &player) override
#endif // (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27)
	{
		UPopcornFXEmitterComponent	*emitterComponent = CastChecked<UPopcornFXEmitterComponent>(&object);
		PK_ASSERT(emitterComponent->Effect != null);

		UPopcornFXAttributeList		*attrList = emitterComponent->GetAttributeList();
		if (PK_VERIFY(attrList != null))
			attrList->RestoreAttributesFromCachedRawData(m_CachedAttributesRawData);
	}

	TArray<uint8>	m_CachedAttributesRawData;
};

struct FPopcornFXAttributePreAnimatedTokenProducer : IMovieScenePreAnimatedTokenProducer
{
	virtual IMovieScenePreAnimatedTokenPtr	CacheExistingState(UObject &object) const override
	{
		FPopcornFXAttributePreAnimatedToken	token;

		UPopcornFXEmitterComponent	*emitterComponent = CastChecked<UPopcornFXEmitterComponent>(&object);
		PK_ASSERT(emitterComponent->Effect != null);
		UPopcornFXAttributeList		*attrList = emitterComponent->GetAttributeList();

		token.m_CachedAttributesRawData.Empty();
		if (PK_VERIFY(attrList != null))
		{
			if (PK_VERIFY(attrList->CheckDataIntegrity()) &&
				PK_VERIFY(attrList->IsUpToDate(emitterComponent->Effect)))
			{
				if (attrList->m_AttributesRawData.Num() > 0)
				{
					const u32	coveredBytes = attrList->m_AttributesRawData.Num();
					token.m_CachedAttributesRawData.SetNumUninitialized(coveredBytes);
					PopcornFX::Mem::Copy(token.m_CachedAttributesRawData.GetData(), attrList->m_AttributesRawData.GetData(), coveredBytes);
				}
			}
		}
		return MoveTemp(token);
	}
};

struct FPopcornFXAttributeExecutionToken : IMovieSceneExecutionToken
{
	FPopcornFXAttributeExecutionToken() = default;

	FPopcornFXAttributeExecutionToken(FPopcornFXAttributeExecutionToken&&) = default;
	FPopcornFXAttributeExecutionToken& operator=(FPopcornFXAttributeExecutionToken&&) = default;

	virtual void	Execute(const FMovieSceneContext &context,
		const FMovieSceneEvaluationOperand &operand,
		FPersistentEvaluationData &persistentData,
		IMovieScenePlayer &player)
	{
		MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_PopcornFXAttributeTrack_TokenExecute)

		for (TWeakObjectPtr<> &weakObject : player.FindBoundObjects(operand))
		{
			UPopcornFXEmitterComponent	*emitterComponent = Cast<UPopcornFXEmitterComponent>(weakObject.Get());

			if (emitterComponent == null)
			{
				APopcornFXEmitter	*emitter = Cast<APopcornFXEmitter>(weakObject.Get());
				emitterComponent = emitter ? emitter->PopcornFXEmitterComponent : null;
			}
			if (emitterComponent == null ||
				emitterComponent->Effect == null)
				continue;

			player.SavePreAnimatedState(*emitterComponent, TMovieSceneAnimTypeID<FPopcornFXAttributeExecutionToken>(), FPopcornFXAttributePreAnimatedTokenProducer());

			for (const FScalarParameterNameAndValue &scalarNameAndValue : Values.ScalarValues)
			{
				// NOT OPTIMIZED
				int32	attrIndex = UPopcornFXAttributeFunctions::FindAttributeIndex(emitterComponent, scalarNameAndValue.ParameterName);
				if (attrIndex == -1)
					continue;
				UPopcornFXAttributeFunctions::SetAttributeAsFloat(emitterComponent, attrIndex, scalarNameAndValue.Value);
			}
			for (const FVectorParameterNameAndValue &vectorNameAndValue : Values.VectorValues)
			{
				// NOT OPTIMIZED
				int32	attrIndex = UPopcornFXAttributeFunctions::FindAttributeIndex(emitterComponent, vectorNameAndValue.ParameterName);
				if (attrIndex == -1)
					continue;
				UPopcornFXAttributeFunctions::SetAttributeAsVector(emitterComponent, attrIndex, vectorNameAndValue.Value);
			}
			for (const FColorParameterNameAndValue &colorNameAndValue : Values.ColorValues)
			{
				// NOT OPTIMIZED
				s32	attrIndex = UPopcornFXAttributeFunctions::FindAttributeIndex(emitterComponent, colorNameAndValue.ParameterName);
				if (attrIndex == -1)
					continue;
				UPopcornFXAttributeFunctions::SetAttributeAsLinearColor(emitterComponent, attrIndex, colorNameAndValue.Value);
			}
		}
	}

	FEvaluatedParameterSectionValues	Values;
};

FPopcornFXAttributeSectionTemplate::FPopcornFXAttributeSectionTemplate(const UMovieSceneParameterSection &section, const UPopcornFXAttributeTrack &track)
	: FMovieSceneParameterSectionTemplate(section)
{
}

void	FPopcornFXAttributeSectionTemplate::Evaluate(const FMovieSceneEvaluationOperand &operand,
	const FMovieSceneContext &context,
	const FPersistentEvaluationData &persistentData,
	FMovieSceneExecutionTokens &executionTokens) const
{
	FPopcornFXAttributeExecutionToken	executionToken;

	EvaluateCurves(context, executionToken.Values);

	executionTokens.Add(MoveTemp(executionToken));
}
