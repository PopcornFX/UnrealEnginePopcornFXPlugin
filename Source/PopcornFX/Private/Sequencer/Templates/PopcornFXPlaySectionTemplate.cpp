//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "Sequencer/Templates/PopcornFXPlaySectionTemplate.h"

#include "Sequencer/Tracks/PopcornFXPlayTrack.h"

#include "PopcornFXPlugin.h"

#include "PopcornFXEmitter.h"
#include "PopcornFXEmitterComponent.h"

#include "IMovieScenePlayer.h"

DECLARE_CYCLE_STAT(TEXT("PopcornFX Play Track Token Execute"), MovieSceneEval_PopcornFXPlayTrack_TokenExecute, STATGROUP_MovieSceneEval);

//----------------------------------------------------------------------------
//
// FPopcornFXPlayPreAnimatedTokenProducer
//
//----------------------------------------------------------------------------

UPopcornFXEmitterComponent	*TryGetEmitterComponent(UObject *object)
{
	UPopcornFXEmitterComponent	*emitterComponent = Cast<UPopcornFXEmitterComponent>(object);

	if (emitterComponent == null)
	{
		APopcornFXEmitter	*emitter = Cast<APopcornFXEmitter>(object);
		emitterComponent = emitter != null ? emitter->PopcornFXEmitterComponent : null;
	}
	if (emitterComponent == null ||
		emitterComponent->Effect == null)
		return null;
	return emitterComponent;
}

//----------------------------------------------------------------------------

struct FPopcornFXPlayPreAnimatedToken : IMovieScenePreAnimatedToken
{
	FPopcornFXPlayPreAnimatedToken(UObject& inObject)
	{
		m_CachedIsAlive = false;

		UPopcornFXEmitterComponent	*emitterComponent = TryGetEmitterComponent(&inObject);
		if (emitterComponent != null)
			m_CachedIsAlive = emitterComponent->IsEmitterAlive();
	}

#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27)
	virtual void	RestoreState(UObject &object, const UE::MovieScene::FRestoreStateParams &params) override
#else
	virtual void	RestoreState(UObject &object, IMovieScenePlayer &player) override
#endif // (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27)
	{
		UPopcornFXEmitterComponent	*emitterComponent = TryGetEmitterComponent(&object);

		if (emitterComponent == null)
			return;

		const bool	isAlive = emitterComponent->IsEmitterAlive();
		if (m_CachedIsAlive && !isAlive)
			emitterComponent->StartEmitter();
		else if (!m_CachedIsAlive && isAlive)
			emitterComponent->StopEmitter();
	}

	bool	m_CachedIsAlive;
};

//----------------------------------------------------------------------------

struct FPopcornFXPlayPreAnimatedTokenProducer : IMovieScenePreAnimatedTokenProducer
{
	static FMovieSceneAnimTypeID	GetAnimTypeID()
	{
		return TMovieSceneAnimTypeID<FPopcornFXPlayPreAnimatedTokenProducer>();
	}
private:
	virtual IMovieScenePreAnimatedTokenPtr	CacheExistingState(UObject &object) const override
	{
		return FPopcornFXPlayPreAnimatedToken(object);
	}
};

struct FPopcornFXPlayStateKeyState : IPersistentEvaluationData
{
	FKeyHandle	m_LastKeyHandle;
	FKeyHandle	m_InvalidKeyHandle;
};
//----------------------------------------------------------------------------
//
// FPopcornFXPlayExecutionToken
//
//----------------------------------------------------------------------------

struct FPopcornFXPlayExecutionToken : IMovieSceneExecutionToken
{
	FPopcornFXPlayExecutionToken(EPopcornFXPlayStateKey keyType)
		: m_KeyType(keyType)
	{

	}

	virtual void	Execute(const FMovieSceneContext &context,
		const FMovieSceneEvaluationOperand &operand,
		FPersistentEvaluationData &persistentData,
		IMovieScenePlayer &player)
	{
		MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_PopcornFXPlayTrack_TokenExecute);

		for (TWeakObjectPtr<> &weakObject : player.FindBoundObjects(operand))
		{
			UPopcornFXEmitterComponent	*emitterComponent = TryGetEmitterComponent(weakObject.Get());

			if (emitterComponent == null)
				continue;

			player.SavePreAnimatedState(*emitterComponent, FPopcornFXPlayPreAnimatedTokenProducer::GetAnimTypeID(), FPopcornFXPlayPreAnimatedTokenProducer());

			switch (m_KeyType)
			{
				case	EPopcornFXPlayStateKey::Start:
				{
					emitterComponent->RestartEmitter(true);
					break;
				}
				case	EPopcornFXPlayStateKey::Stop:
				{
					emitterComponent->StopEmitter();
					break;
				}
				case	EPopcornFXPlayStateKey::Toggle:
				{
					emitterComponent->ToggleEmitter(!emitterComponent->IsEmitterStarted());
					break;
				}
			}
		}
	}

	EPopcornFXPlayStateKey	m_KeyType;
	TOptional<FKeyHandle>	m_KeyHandle;
};

//----------------------------------------------------------------------------
//
// FPopcornFXPlaySectionTemplate
//
//----------------------------------------------------------------------------

FPopcornFXPlaySectionTemplate::FPopcornFXPlaySectionTemplate(const UMovieSceneParticleSection &section)
	: Keys(section.ParticleKeys)
{
}

//----------------------------------------------------------------------------

void	FPopcornFXPlaySectionTemplate::Evaluate(const FMovieSceneEvaluationOperand &operand,
	const FMovieSceneContext &context,
	const FPersistentEvaluationData &persistentData,
	FMovieSceneExecutionTokens &executionTokens) const
{
	const bool bPlaying = context.GetDirection() == EPlayDirection::Forwards &&
		context.GetRange().Size<FFrameTime>() >= FFrameTime(0) &&
		context.GetStatus() == EMovieScenePlayerStatus::Playing;

	if (!bPlaying)
	{
		executionTokens.Add(FPopcornFXPlayExecutionToken(EPopcornFXPlayStateKey::Stop));
	}
	else
	{
		TRange<FFrameNumber>				playbackRange = context.GetFrameNumberRange();
		TMovieSceneChannelData<const uint8>	channelData = Keys.GetData();

		TArrayView<const FFrameNumber>	times = channelData.GetTimes();
		TArrayView<const uint8>			values = channelData.GetValues();

		const int32		lastKeyIndex = Algo::UpperBound(times, playbackRange.GetUpperBoundValue()) - 1;
		if (lastKeyIndex >= 0 && playbackRange.Contains(times[lastKeyIndex]))
		{
			FPopcornFXPlayExecutionToken	token((EPopcornFXPlayStateKey)values[lastKeyIndex]);
			executionTokens.Add(MoveTemp(token));
		}
	}
}

//----------------------------------------------------------------------------
