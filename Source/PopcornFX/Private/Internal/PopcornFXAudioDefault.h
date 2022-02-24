//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXAudio.h"
#include "PopcornFXSDK.h"

#include <pk_kernel/include/kr_containers_array_chunked.h>

#include "UObject/WeakObjectPtrTemplates.h"
#include "Components/AudioComponent.h" // required for TWeakObjectPtr to work

struct	SPopcornFXSoundInstance
{
	PopcornFX::CGuid				m_Index;
	TWeakObjectPtr<UAudioComponent>	m_AudioComponent;

	PK_FORCEINLINE bool				Valid() const { return m_Index.Valid(); }
	static SPopcornFXSoundInstance	Invalid; // needed by TChunkedSlotArray
};

// PopcornFX Default implementation that uses UE4 sound assets
class	FPopcornFXAudioDefault : public IPopcornFXAudio
{
public:
	FPopcornFXAudioDefault() { }
	virtual ~FPopcornFXAudioDefault();

	void	PreUpdate() override;
	void	*StartNewSound(const FPopcornFXSoundDescriptor &descriptor, UWorld *world) override;
	void	UpdateSound(void *instance, const FVector &worldLocation, float volume) override;
	bool	IsPlaying(void *instance) const override;
	void	RemoveSound(void *instance) override;

private:
	PopcornFX::TChunkedSlotArray<SPopcornFXSoundInstance, 64>	m_SoundInstances;
};
