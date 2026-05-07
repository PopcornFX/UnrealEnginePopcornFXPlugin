//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"
#include "PopcornFXAudioSampling.h"

#include "AudioDefines.h"
#include "DSP/ConstantQ.h"
#include "DSP/DeinterleaveView.h"
#include "DSP/FFTAlgorithm.h"
#include "DSP/FloatArrayMath.h"
#include "DSP/MultithreadedPatching.h"
#include "DSP/SlidingWindow.h"
#include "UObject/ObjectPtr.h"
#include "PopcornFXSubmixListener.h"

#include "GameFramework/Actor.h"

#include "PopcornFXAudioSamplerSubmixActor.generated.h"

class USoundSubmix;

UCLASS()
class POPCORNFX_API	APopcornFXAudioSamplerSubmixActor : public AActor, public FPopcornFXFillAudioBuffers
{
	GENERATED_UCLASS_BODY()
public:
	/** PopcornFXScene name */
	UPROPERTY(EditAnywhere, Category="PopcornFX")
	FName	SceneName;
	
	UPROPERTY(EditAnywhere, Category="PopcornFX")
	int		m_ChannelIndex;
public:
	virtual const float	*GetRawSpectrumBuffer(const FName &ChannelName, uint32 &OutBufferSize) override;
	virtual const float	*GetRawWaveformBuffer(const FName &ChannelName, uint32 &OutBufferSize) override;

	virtual void		BeginPlay() override;
	virtual void		BeginDestroy() override;

	virtual void		PreUpdate() override;

	//Submix Implementation
public:
	void				UpdateWaveformAndSpectrum();
	float				GetSpectrumValue(float InNormalizedPositionInSpectrum);
	int32				GetNumBands() const;

protected:
	typedef Audio::TSlidingBuffer<float>												FSlidingBuffer;
	typedef Audio::TAutoSlidingWindow<float, Audio::FAudioBufferAlignedAllocator>		FSlidingWindow;
	typedef Audio::TAutoDeinterleaveView<float, Audio::FAudioBufferAlignedAllocator>	FDeinterleaveView;
	typedef FDeinterleaveView::TChannel<Audio::FAudioBufferAlignedAllocator>			FChannel;
	
	TArray<float>																		m_SpectrumData;
	TArray<float>																		m_WaveformData;

	float																				m_MinimumFrequency = 0.0f;
	float																				m_MaximumFrequency = 0.0f;
	TAtomic<int32>																		m_NumBands = 0.0f;
	float																				m_NoiseFloorDb = 0.0f;
	
	int32 																				m_NumChannels = 0;
	float 																				m_SampleRate = 0.0f;
	float 																				m_FFTScale = 1.0f;
	
	TMap<Audio::FDeviceId, TSharedPtr<FPopcornFXSubmixListener>>						m_SubmixListeners;
	
	// Mixer for sending audio 				
	Audio::FPatchMixer																	m_PatchMixer;
	Audio::FAlignedFloatBuffer															m_PopBuffer;
	
	class USoundSubmix																	*m_SubmixRegisteredTo = nullptr;
	bool																				m_IsSubmixListenerRegistered = false;
	
	int32																				m_NumSamplesToBuffer = 1024;
	
	DECLARE_DELEGATE_OneParam(FDeviceCreatedHandle, Audio::FDeviceId);
	DECLARE_DELEGATE_OneParam(FDeviceDestroyedHandle, Audio::FDeviceId);

	FDeviceCreatedHandle	m_DeviceCreatedHandle;
	FDeviceDestroyedHandle	m_DeviceDestroyedHandle;

	TUniquePtr<FSlidingBuffer>															m_SlidingBuffer;

	TArray<Audio::FAlignedFloatBuffer>													m_ChannelSpectrumBuffers;

	TUniquePtr<Audio::FContiguousSparse2DKernelTransform>								m_CQTKernel;
	TUniquePtr<Audio::IFFTAlgorithm>													m_FFTAlgorithm;

	Audio::FAlignedFloatBuffer															m_InterleavedBuffer;
	Audio::FAlignedFloatBuffer															m_DeinterleavedBuffer;
	Audio::FAlignedFloatBuffer															m_FFTInputBuffer;
	Audio::FAlignedFloatBuffer															m_FFTOutputBuffer;
	Audio::FAlignedFloatBuffer															m_PowerSpectrumBuffer;
	Audio::FAlignedFloatBuffer															m_SpectrumBuffer;
	Audio::FAlignedFloatBuffer															m_WindowBuffer;

protected:
	int32											GetNumChannels() const;
	float											GetSampleRate() const;

	virtual void									OnUpdateSubmix();

	int32											PopAudio(float *OutBuffer, int32 NumSamples, bool bUseLatestAudio);

	int32											GetNumSamplesAvailable();

	void											SetAudioFormat(int32 InNumChannels, float InSampleRate);
	void											SetNoiseFloor(float InNoiseFloorDb);
	void											ResizeCQT(float InMinimumFrequency, float InMaximumFrequency, int32 InNumBands);

	
	void											ResizeSpectrumBuffer(int32 InNumChannels, int32 InNumBands);
	void											ResizeAudioTransform(	float InMinimumFrequency,
																			float InMaximumFrequency,
																			float InSampleRate,
																			int32 InNumBands);
	
	void											ResizeWindow(int32 InNumChannels, int32 InFFTSize);

	void 											AddSubmixListener(const Audio::FDeviceId InDeviceId);
	void 											RemoveSubmixListener(const Audio::FDeviceId InDeviceId);

	void 											RegisterToAllAudioDevices();
	void 											UnregisterFromAllAudioDevices();

	void											OnNewDeviceCreated(Audio::FDeviceId InDeviceId);
	void											OnDeviceDestroyed(Audio::FDeviceId InDeviceId);


	static void										Clamp(float &InMinimumFrequency, float &InMaximumFrequency, int32 &InNumBand);
	static float									GetNumOctaves(float InMinimumFrequency, float InMaximumFrequency);
	static float									GetNumBandsPerOctave(int32 InNumBands, float InNumOctaves);
	static float									GetBandwidthStretch(	float InSampleRate,
																			float InFFTSize,
																			float InNumBandsPerOctave,
																			float InMinimumFrequency);

	static Audio::FPseudoConstantQKernelSettings	GetConstantQSettings(	float InMinimumFrequency,
																			float InMaximumFrequency,
																			int32 InNumBands,
																			float InNumBandsPerOctave,
																			float InBandwidthStretch);

	static Audio::FFFTSettings						GetFFTSettings(float InMinimumFrequency, float InSampleRate, float InNumBandsPerOctave);
};