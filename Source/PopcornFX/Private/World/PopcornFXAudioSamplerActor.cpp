//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "PopcornFXAudioSamplerActor.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXSDK.h"
#include "PopcornFXSceneActor.h"
#include "PopcornFXSceneComponent.h"
#include "EngineUtils.h"

//----------------------------------------------------------------------------

APopcornFXAudioSamplerActor::APopcornFXAudioSamplerActor(const class FObjectInitializer& PCIP)
	: Super(PCIP)
	, SceneName(TEXT("PopcornFX_DefaultScene"))
{
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerActor::BeginPlay()
{
	Super::BeginPlay();

	RegisterAudioChannel("Master");

	for (TActorIterator<APopcornFXSceneActor> sceneIt(GetWorld()); sceneIt; ++sceneIt)
	{
		if (sceneIt->GetSceneName() != SceneName)
			continue;
		UPopcornFXSceneComponent	*sceneComponent = (*sceneIt)->PopcornFXSceneComponent;
		if (!PK_VERIFY(sceneComponent != null))
			continue;
		sceneComponent->SetAudioSamplingInterface(this);
		break;
	}
}

//----------------------------------------------------------------------------

const float	*APopcornFXAudioSamplerActor::GetRawSpectrumBuffer(const FName &ChannelName, uint32 &OutBufferSize)
{
	PK_ASSERT(IsInGameThread());

	OutBufferSize = m_SpectrumData.Num();
	return m_SpectrumData.GetData();
}

//----------------------------------------------------------------------------

const float	*APopcornFXAudioSamplerActor::GetRawWaveformBuffer(const FName &ChannelName, uint32 &OutBufferSize)
{
	PK_ASSERT(IsInGameThread());

	OutBufferSize = m_WaveformData.Num();
	return m_WaveformData.GetData();
}

//----------------------------------------------------------------------------
