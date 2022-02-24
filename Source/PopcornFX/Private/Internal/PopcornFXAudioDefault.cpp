//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXAudioDefault.h"
#include "PopcornFXPlugin.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAudio, Log, All);

SPopcornFXSoundInstance		SPopcornFXSoundInstance::Invalid = SPopcornFXSoundInstance();

//----------------------------------------------------------------------------

FPopcornFXAudioDefault::~FPopcornFXAudioDefault()
{
	PK_ASSERT(m_SoundInstances.UsedCount() == 0);
}

//----------------------------------------------------------------------------

void	FPopcornFXAudioDefault::PreUpdate()
{
	PK_ASSERT(IsInGameThread());
}

//----------------------------------------------------------------------------

void	*FPopcornFXAudioDefault::StartNewSound(const FPopcornFXSoundDescriptor &descriptor, UWorld *world)
{
	PK_ASSERT(IsInGameThread());
	PK_ASSERT(world != null);

	PopcornFX::CGuid	soundIndex = m_SoundInstances.Insert(SPopcornFXSoundInstance());
	if (!PK_VERIFY(soundIndex.Valid()))
		return null;
	m_SoundInstances[soundIndex].m_Index = soundIndex;
	PK_ASSERT(m_SoundInstances[soundIndex].m_Index.Valid());

	PK_ASSERT(m_SoundInstances[soundIndex].m_AudioComponent.Get() == null);
	const FString	soundPath = FPopcornFXPlugin::Get().BuildPathFromPkPath(descriptor.m_SoundPath, true);
	USoundBase		*sound = ::LoadObject<USoundBase>(null, *soundPath);
	if (sound == null)
	{
		UE_LOG(LogPopcornFXAudio, Warning, TEXT("Couldn't play '%s', sound wasn't found"), descriptor.m_SoundPath);
		return null;
	}
	UAudioComponent	*audioComponent = null;
	{
		FAudioDevice::FCreateComponentParams	params(world);
		params.bPlay = false;
		params.bStopWhenOwnerDestroyed = true;
		audioComponent = FAudioDevice::CreateComponent(sound, params);
	}
	if (!PK_VERIFY(audioComponent != null))
		return null;

	audioComponent->bAutoDestroy = true;
	m_SoundInstances[soundIndex].m_AudioComponent = audioComponent;

	UpdateSound(&m_SoundInstances[soundIndex], descriptor.m_SoundWorldLocation, descriptor.m_SoundVolume);

	audioComponent->Play(descriptor.m_SoundStartTime);
	// @TODO m_PlayTimeInSeconds

	return &m_SoundInstances[soundIndex];
}

//----------------------------------------------------------------------------

void	FPopcornFXAudioDefault::UpdateSound(void *instance, const FVector &worldLocation, float volume)
{
	PK_ASSERT(IsInGameThread());
	PK_ASSERT(instance != null);

	SPopcornFXSoundInstance	*soundInstance = static_cast<SPopcornFXSoundInstance*>(instance);
	PK_ASSERT(soundInstance->m_AudioComponent.IsValid());

	UAudioComponent	*audioComponent = soundInstance->m_AudioComponent.Get();
	audioComponent->SetWorldLocation(worldLocation);
	audioComponent->SetVolumeMultiplier(volume);
}

//----------------------------------------------------------------------------

bool	FPopcornFXAudioDefault::IsPlaying(void *instance) const
{
	PK_ASSERT(IsInGameThread());
	PK_ASSERT(instance != null);

	SPopcornFXSoundInstance	*soundInstance = static_cast<SPopcornFXSoundInstance*>(instance);

	UAudioComponent	*audioComponent = soundInstance->m_AudioComponent.Get();
	if (audioComponent == null || !audioComponent->IsPlaying())
		return false; // will RemoveSound()
	return true;
}

//----------------------------------------------------------------------------

void	FPopcornFXAudioDefault::RemoveSound(void *instance)
{
	PK_ASSERT(IsInGameThread());
	PK_ASSERT(instance != null);

	SPopcornFXSoundInstance	*soundInstance = static_cast<SPopcornFXSoundInstance*>(instance);

	if (soundInstance->m_AudioComponent.IsValid())
		soundInstance->m_AudioComponent->DestroyComponent();
	soundInstance->m_AudioComponent = null;
	m_SoundInstances.Remove(soundInstance->m_Index);
}

//----------------------------------------------------------------------------
