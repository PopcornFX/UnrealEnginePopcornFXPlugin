//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

// Disable this line if you intend to override with your own interface
#define USE_DEFAULT_AUDIO_INTERFACE

// Duplicate of PopcornFX::IParticleScene::SSoundDescriptor
// because we don't want to have PopcornFX types in public header files
struct	FPopcornFXSoundDescriptor
{
	const char	*m_SoundPath;
	const char	*m_SoundStartEventName;
	const char	*m_SoundStopEventName;

	FVector		m_SoundWorldLocation;

	float		m_SoundVolume;
	float		m_SoundStartTime;
	float		m_SoundDuration;

	uint32		m_UserData;
};

// Generic audio interface, inherit this one if you want full control
class	IPopcornFXAudio
{
public:
	/* Do any processing before PopcornFX update loop */
	virtual void		PreUpdate() = 0;

	/* Return a pointer to the newly created sound instance. */
	virtual void		*StartNewSound(const FPopcornFXSoundDescriptor &descriptor, UWorld *world) = 0;

	/** Update your sound instance with this new location or volume. */
	virtual void		UpdateSound(void *instance, const FVector &worldLocation, float volume) = 0;

	/* Return false when the real sound instance stopped playing or was destroyed. */
	virtual bool		IsPlaying(void *instance) const = 0;

	/* Remove or destroy this sound instance from your container (if any). */
	virtual void		RemoveSound(void *instance) = 0;
};
