//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "ISubmixBufferListener.h"
#include "DSP/MultithreadedPatching.h"
#include "AudioDeviceManager.h"
#include "Runtime/Launch/Resources/Version.h"

class FPopcornFXSubmixListener : public ISubmixBufferListener
{
public:
	FPopcornFXSubmixListener(	Audio::FPatchMixer	&InMixer,
								int32				InNumSamplesToBuffer,
								Audio::FDeviceId	InDeviceId);
	
	
	FPopcornFXSubmixListener(FPopcornFXSubmixListener	&&Other);

	virtual	~FPopcornFXSubmixListener();
	
	float			GetSampleRate() const;
	int32			GetNumChannels() const;

	void			RegisterToSubmix();
	virtual void	OnNewSubmixBuffer(	const USoundSubmix	*OwningSubmix,
										float				*AudioData,
										int32				NumSamples,
										int32				NumChannels,
										const int32			SampleRate,
										double				AudioClock) override;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	const FString&	GetListenerName() const override;
#else
	const FString& GetListenerName() const;
#endif
	
private:
	FPopcornFXSubmixListener();

	void	UnregisterFromSubmix();

	
	TAtomic<int32>												m_NumChannelsInSubmix;
	TAtomic<int32>												m_SubmixSampleRate;
	
	Audio::FPatchInput											m_MixerInput;
	Audio::FDeviceId											m_AudioDeviceId;
	bool														m_IsRegistered;
};