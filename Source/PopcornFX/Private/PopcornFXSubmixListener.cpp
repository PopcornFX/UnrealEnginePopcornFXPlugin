// Fill out your copyright notice in the Description page of Project Settings.


#include "PopcornFXSubmixListener.h"

#include "AudioDevice.h"

#include "PopcornFXMinimal.h"
#include "PopcornFXPlugin.h"

FPopcornFXSubmixListener::FPopcornFXSubmixListener(	Audio::FPatchMixer	&InMixer,
													int32				InNumSamplesToBuffer,
													Audio::FDeviceId	InDeviceId):
													m_NumChannelsInSubmix(0),
													m_SubmixSampleRate(0),
													m_MixerInput(InMixer.AddNewInput(InNumSamplesToBuffer, 1.0f)),
													m_AudioDeviceId(InDeviceId),
													m_IsRegistered(false)
{
}

FPopcornFXSubmixListener::FPopcornFXSubmixListener(FPopcornFXSubmixListener&& Other)
{
	UnregisterFromSubmix();
	Other.UnregisterFromSubmix();

	m_NumChannelsInSubmix		= Other.m_NumChannelsInSubmix.Load();
	Other.m_NumChannelsInSubmix	= 0;

	m_SubmixSampleRate			= Other.m_SubmixSampleRate.Load();
	Other.m_SubmixSampleRate	= 0;

	m_MixerInput				= MoveTemp(Other.m_MixerInput);

	m_AudioDeviceId				= Other.m_AudioDeviceId;
	Other.m_AudioDeviceId		= INDEX_NONE;

	RegisterToSubmix();
}

FPopcornFXSubmixListener::FPopcornFXSubmixListener():	m_NumChannelsInSubmix(0),
														m_SubmixSampleRate(0),
														m_MixerInput(),
														m_AudioDeviceId(INDEX_NONE),
														m_IsRegistered(false)
{
}

FPopcornFXSubmixListener::~FPopcornFXSubmixListener()
{
	UnregisterFromSubmix();
}

float	FPopcornFXSubmixListener::GetSampleRate() const
{
	return static_cast<float>(m_SubmixSampleRate.Load());
}

int32	FPopcornFXSubmixListener::GetNumChannels() const
{
	return m_NumChannelsInSubmix;
}

void	FPopcornFXSubmixListener::RegisterToSubmix()
{
	if (FAudioDevice* AudioDevice = FAudioDeviceManager::Get()->GetAudioDeviceRaw(m_AudioDeviceId))
	{
		m_IsRegistered = true;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
		USoundSubmix* SubmixToRegister = &AudioDevice->GetMainSubmixObject();
		AudioDevice->RegisterSubmixBufferListener(AsShared(), *SubmixToRegister);
#else
		AudioDevice->RegisterSubmixBufferListener(this, nullptr);
#endif
		
		// RegisterSubmixBufferListener lazily enqueues the registration on the audio thread,
		// so we have to wait for the audio thread to destroy.
		FAudioCommandFence Fence;
		Fence.BeginFence();
		Fence.Wait();
	}
}

void	FPopcornFXSubmixListener::UnregisterFromSubmix()
{
	if (m_IsRegistered)
	{
		FAudioDevice	*DeviceHandle = FAudioDeviceManager::Get()->GetAudioDeviceRaw(m_AudioDeviceId);
		
		m_IsRegistered = false;
		if (DeviceHandle != null)
		{
			if (IsInGameThread())
			{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
				DeviceHandle->UnregisterSubmixBufferListener(AsShared(), DeviceHandle->GetMainSubmixObject());
#else
				DeviceHandle->UnregisterSubmixBufferListener(this, nullptr);
#endif
			}
			else
			{
				UE::Tasks::FTaskEvent CompletionEvent{ UE_SOURCE_LOCATION };
				FAudioThread::RunCommandOnAudioThread([this, DeviceHandle, &CompletionEvent]()
					{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
						DeviceHandle->UnregisterSubmixBufferListener(AsShared(), DeviceHandle->GetMainSubmixObject());
#else
						DeviceHandle->UnregisterSubmixBufferListener(this, nullptr);
#endif
						CompletionEvent.Trigger();
					});
				CompletionEvent.Wait();
			}
		}
	}
}

void	FPopcornFXSubmixListener::OnNewSubmixBuffer(	const USoundSubmix	*OwningSubmix,
														float				*AudioData,
														int32				NumSamples,
														int32				NumChannels,
														const int32			SampleRate,
														double				AudioClock)
{
	m_NumChannelsInSubmix	= NumChannels;
	m_SubmixSampleRate		= SampleRate;
	
	m_MixerInput.PushAudio(AudioData, NumSamples);
}

const FString& FPopcornFXSubmixListener::GetListenerName() const
{
	static const FString ListenerName = TEXT("PopcornFXSubmixListener");
	return ListenerName;
}