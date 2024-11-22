//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "PopcornFXAudioSamplerSubmixActor.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXSDK.h"
#include "PopcornFXSceneActor.h"
#include "PopcornFXSceneComponent.h"
#include "EngineUtils.h"
#include "PopcornFXSubmixListener.h"
#include "DSP/AudioFFT.h"

//----------------------------------------------------------------------------

APopcornFXAudioSamplerSubmixActor::APopcornFXAudioSamplerSubmixActor(const class FObjectInitializer& PCIP)
	: Super(PCIP)
	, SceneName(TEXT("PopcornFX_DefaultScene"))
{
	const u32	kInitialNumChannels = 2;
	const float	kInitialSampleRate = 48000.0f;

	SetAudioFormat(kInitialNumChannels, kInitialSampleRate);

	ResizeCQT(1.0f, 20000.0f, 1024);

	SetNoiseFloor(-60.0f);
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::BeginPlay()
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
	OnUpdateSubmix();
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::BeginDestroy()
{
	check(IsInGameThread());
	m_DeviceCreatedHandle.Unbind();
	m_DeviceDestroyedHandle.Unbind();

	if (m_IsSubmixListenerRegistered)
		UnregisterFromAllAudioDevices();

	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::PreUpdate()
{
	UpdateWaveformAndSpectrum();
	FPopcornFXFillAudioBuffers::PreUpdate();
}

//----------------------------------------------------------------------------

const float	*APopcornFXAudioSamplerSubmixActor::GetRawSpectrumBuffer(const FName &ChannelName, uint32 &OutBufferSize)
{
	PK_ASSERT(IsInGameThread());
	OutBufferSize = m_ChannelSpectrumBuffers[m_ChannelIndex].Num();
	return m_ChannelSpectrumBuffers[m_ChannelIndex].GetData();
}

//----------------------------------------------------------------------------

const float	*APopcornFXAudioSamplerSubmixActor::GetRawWaveformBuffer(const FName &ChannelName, uint32 &OutBufferSize)
{
	PK_ASSERT(IsInGameThread());
	
	OutBufferSize = m_PopBuffer.Num();
	return m_PopBuffer.GetData();
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::UpdateWaveformAndSpectrum()
{
	const s32	SourceNumChannels = GetNumChannels();
	const float	SourceSampleRate = GetSampleRate();

	// Perform internal update if new samplerate or channel count encountered.
	if ((SourceNumChannels != m_NumChannels) || (SourceSampleRate != m_SampleRate))
		SetAudioFormat(SourceNumChannels, SourceSampleRate); // This will update values of NumChannels and SampleRate member variables.
	
	// Set data to zero if we are skipping update due to bad samplerate or channel count.
	if (m_NumChannels == 0 || FMath::IsNearlyZero(m_SampleRate))
	{
		for (s32 ChannelIndex = 0; ChannelIndex < m_ChannelSpectrumBuffers.Num(); ChannelIndex++)
		{
			Audio::FAlignedFloatBuffer&	Buffer = m_ChannelSpectrumBuffers[ChannelIndex];
			if (Buffer.Num() > 0)
			{
				FMemory::Memset(Buffer.GetData(), 0, sizeof(float) * Buffer.Num());
			}
		}
		return;
	}
	
	// Grab audio from buffer.
	s32	NumSamplesToPop = GetNumSamplesAvailable();
	NumSamplesToPop -= (NumSamplesToPop % m_NumChannels);
	m_PopBuffer.Reset();
	
	if (NumSamplesToPop > 0)
	{
		m_PopBuffer.AddUninitialized(NumSamplesToPop);

		bool	bUseLatestAudio = false;
		s32	NumPopped = PopAudio(m_PopBuffer.GetData(), NumSamplesToPop, bUseLatestAudio);
	}
	
	// If we're in a bad internal state, give up here.
	if (!m_FFTAlgorithm.IsValid() || !m_CQTKernel.IsValid())
		return;

	// Run sliding window over available audio.
	FSlidingWindow	SlidingWindow(*m_SlidingBuffer, m_PopBuffer, m_InterleavedBuffer);

	s32	NumWindows = 0;
	for (Audio::FAlignedFloatBuffer& InterleavedWindow : SlidingWindow)
	{
		if (0 == NumWindows)
		{
			// Zero output on first window.  We need to wait for at least one window to process
			// because we do not want to zero out data if no windows are available.
			for (s32 ChannelIndex = 0; ChannelIndex < m_NumChannels; ChannelIndex++)
			{
				FMemory::Memset(m_ChannelSpectrumBuffers[ChannelIndex].GetData(), 0, sizeof(float) * m_NumBands);
			}
		}

		NumWindows++;

		// Run deinterleave view over window.
		FDeinterleaveView	DeinterleaveView(InterleavedWindow, m_DeinterleavedBuffer, m_NumChannels);
		for(FChannel	Channel : DeinterleaveView)
		{
			Audio::ArrayMultiplyInPlace(m_WindowBuffer, Channel.Values);

			// Perform FFT
			FMemory::Memcpy(m_FFTInputBuffer.GetData(), Channel.Values.GetData(), sizeof(float) * Channel.Values.Num());

			m_FFTAlgorithm->ForwardRealToComplex(m_FFTInputBuffer.GetData(), m_FFTOutputBuffer.GetData());

			// Transflate FFTOutput to power spectrum
			Audio::ArrayComplexToPower(m_FFTOutputBuffer, m_PowerSpectrumBuffer);

			Audio::ArraySqrtInPlace(m_PowerSpectrumBuffer);
			// Take CQT of power spectrum
			for (int32 i = 0; i < m_SpectrumBuffer.Num(); i++)
			{
				m_SpectrumBuffer[i] = m_PowerSpectrumBuffer[i];
			}

			// Accumulate power spectrum CQT output.
			Audio::ArrayAddInPlace(m_SpectrumBuffer, m_ChannelSpectrumBuffers[Channel.ChannelIndex]);
		}
	}
	if (NumWindows > 0)
	{
		 const float	LinearScale = 1.0f / m_NumBands;
		// Apply scaling for each channel.
		for (s32 ChannelIndex = 0; ChannelIndex < m_NumChannels; ChannelIndex++)
		{
			Audio::FAlignedFloatBuffer	&Buffer = m_ChannelSpectrumBuffers[ChannelIndex];
			Audio::ArrayMultiplyByConstantInPlace(Buffer, LinearScale);
		}
	}
}

//----------------------------------------------------------------------------

float	APopcornFXAudioSamplerSubmixActor::GetSpectrumValue(float InNormalizedPositionInSpectrum)
{
	check(m_ChannelIndex >= 0);

	// Check if channel index is within channel spectrum buffers
	if (m_ChannelIndex >= m_ChannelSpectrumBuffers.Num())
		return 0.0f;

	const Audio::FAlignedFloatBuffer	&Buffer = m_ChannelSpectrumBuffers[m_ChannelIndex];
	const s32							MaxIndex = Buffer.Num() - 1;

	if (MaxIndex < 0)
		return 0.0f;

	InNormalizedPositionInSpectrum = FMath::Clamp(InNormalizedPositionInSpectrum, 0.0f, 1.0f);

	float	Index = InNormalizedPositionInSpectrum * Buffer.Num();

	s32	LowerIndex = FMath::FloorToInt(Index);
	LowerIndex = FMath::Clamp(LowerIndex, 0, MaxIndex);

	s32	UpperIndex = FMath::CeilToInt(Index);
	UpperIndex = FMath::Clamp(UpperIndex, 0, MaxIndex);

	float	Alpha = Index - LowerIndex;
		
	return FMath::Lerp<float>(Buffer[LowerIndex], Buffer[UpperIndex], Alpha);
}

//----------------------------------------------------------------------------

s32	APopcornFXAudioSamplerSubmixActor::GetNumBands() const
{
	check(IsInRenderingThread());
	return m_NumBands;
}

//----------------------------------------------------------------------------

s32	APopcornFXAudioSamplerSubmixActor::GetNumChannels() const
{
	TArray<Audio::FDeviceId>	DeviceIds;
	m_SubmixListeners.GetKeys(DeviceIds);
	
	for (const Audio::FDeviceId DeviceId : DeviceIds)
	{
		const s32	NumChannels = m_SubmixListeners[DeviceId]->GetNumChannels();

		if (NumChannels != 0)
			return NumChannels; // return first non-zero value.
	}
	return 0;
}

//----------------------------------------------------------------------------

float	APopcornFXAudioSamplerSubmixActor::GetSampleRate() const
{
	TArray<Audio::FDeviceId>	DeviceIds;
	m_SubmixListeners.GetKeys(DeviceIds);
	for (const Audio::FDeviceId DeviceId : DeviceIds)
	{
		const float	SampleRate = m_SubmixListeners[DeviceId]->GetSampleRate();

		if (SampleRate > 0.0f)
			return SampleRate; //return first non-zero value.
	}
	return 0.0f;
}

//----------------------------------------------------------------------------

void APopcornFXAudioSamplerSubmixActor::Clamp(float& InMinimumFrequency, float& InMaximumFrequency, s32& InNumBands) 
{
	const s32	MinimumSupportedNumBands = 1;
	const s32	MaximumSupportedNumBands = 16384;
	const float	MinimumSupportedFrequency = 20.0f;
	const float	MaximumSupportedFrequency = 20000.0f;

	InMinimumFrequency = FMath::Max(InMinimumFrequency, MinimumSupportedFrequency);
	InMaximumFrequency = FMath::Min(InMaximumFrequency, MaximumSupportedFrequency);
	InMaximumFrequency = FMath::Max(InMinimumFrequency, InMaximumFrequency);

	InNumBands = FMath::Clamp(InNumBands, MinimumSupportedNumBands, MaximumSupportedNumBands);
}

//----------------------------------------------------------------------------

float	APopcornFXAudioSamplerSubmixActor::GetNumOctaves(float InMinimumFrequency, float InMaximumFrequency) 
{
	const float MinimumSupportedNumOctaves = 0.01f;

	float NumOctaves = FMath::Log2(InMaximumFrequency) - FMath::Log2(InMinimumFrequency);
	NumOctaves = FMath::Max(MinimumSupportedNumOctaves, NumOctaves);

	return NumOctaves;
}

//----------------------------------------------------------------------------

float	APopcornFXAudioSamplerSubmixActor::GetNumBandsPerOctave(s32 InNumBands, float InNumOctaves)
{
	InNumOctaves = FMath::Max(0.01f, InNumOctaves);
	const float MinimumSupportedNumBandsPerOctave = 0.01f;

	float NumBandsPerOctave = FMath::Max(MinimumSupportedNumBandsPerOctave, static_cast<float>(InNumBands) / InNumOctaves);

	return NumBandsPerOctave;
}

//----------------------------------------------------------------------------

float	APopcornFXAudioSamplerSubmixActor::GetBandwidthStretch(float InSampleRate, float InFFTSize, float InNumBandsPerOctave, float InMinimumFrequency)
{
	InFFTSize = FMath::Max(1.f, InFFTSize);

	float MinimumFrequencySpacing = (FMath::Pow(2.f, 1.f / InNumBandsPerOctave) - 1.f) * InMinimumFrequency;
	MinimumFrequencySpacing = FMath::Max(0.01f, MinimumFrequencySpacing);

	float FFTBinSpacing = InSampleRate / InFFTSize;
	float BandwidthStretch = FFTBinSpacing / MinimumFrequencySpacing;

	return FMath::Clamp(BandwidthStretch, 0.5f, 2.f);
}

//----------------------------------------------------------------------------

Audio::FPseudoConstantQKernelSettings	APopcornFXAudioSamplerSubmixActor::GetConstantQSettings(float InMinimumFrequency, float InMaximumFrequency, s32 InNumBands, float InNumBandsPerOctave, float InBandwidthStretch)
{
	Audio::FPseudoConstantQKernelSettings CQTKernelSettings;

	CQTKernelSettings.NumBands = InNumBands;
	CQTKernelSettings.KernelLowestCenterFreq = InMinimumFrequency; 
	CQTKernelSettings.NumBandsPerOctave = InNumBandsPerOctave;
	CQTKernelSettings.BandWidthStretch = InBandwidthStretch;
	CQTKernelSettings.Normalization = Audio::EPseudoConstantQNormalization::EqualEnergy;

	return CQTKernelSettings;
}

//----------------------------------------------------------------------------

Audio::FFFTSettings	APopcornFXAudioSamplerSubmixActor::GetFFTSettings(float InMinimumFrequency, float InSampleRate, float InNumBandsPerOctave)
{
	const s32	MaximumSupportedLog2FFTSize = 13;
	const s32	MinimumSupportedLog2FFTSize = 8;

	InNumBandsPerOctave = FMath::Max(0.01f, InNumBandsPerOctave);

	float	MinimumFrequencySpacing = (FMath::Pow(2.f, 1.f / InNumBandsPerOctave) - 1.f) * InMinimumFrequency;

	MinimumFrequencySpacing = FMath::Max(0.01f, MinimumFrequencySpacing);

	const s32	DesiredFFTSize = FMath::CeilToInt(3.f * InSampleRate / MinimumFrequencySpacing);

	s32	Log2FFTSize = MinimumSupportedLog2FFTSize;
	while ((DesiredFFTSize > (1 << Log2FFTSize)) && (Log2FFTSize < MaximumSupportedLog2FFTSize))
	{
		Log2FFTSize++;
	}

	Audio::FFFTSettings	FFTSettings;

	FFTSettings.Log2Size = Log2FFTSize;
	FFTSettings.bArrays128BitAligned = true;
	FFTSettings.bEnableHardwareAcceleration = true;

	return FFTSettings;
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::OnUpdateSubmix()
{
	check(IsInGameThread());
	
	if (m_IsSubmixListenerRegistered)
		UnregisterFromAllAudioDevices();

	if (!m_DeviceCreatedHandle.IsBound())
		m_DeviceCreatedHandle.BindUObject(this, &APopcornFXAudioSamplerSubmixActor::OnNewDeviceCreated);
	if (!m_DeviceDestroyedHandle.IsBound())
		m_DeviceDestroyedHandle.BindUObject(this, &APopcornFXAudioSamplerSubmixActor::OnDeviceDestroyed);
	
	RegisterToAllAudioDevices();
	m_IsSubmixListenerRegistered = true;
}

//----------------------------------------------------------------------------

s32	APopcornFXAudioSamplerSubmixActor::PopAudio(float *OutBuffer, s32 NumSamples, bool bUseLatestAudio)
{
	return m_PatchMixer.PopAudio(OutBuffer, NumSamples, bUseLatestAudio);
}

//----------------------------------------------------------------------------

s32	APopcornFXAudioSamplerSubmixActor::GetNumSamplesAvailable()
{
	return m_PatchMixer.MaxNumberOfSamplesThatCanBePopped();
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::SetAudioFormat(s32 InNumChannels, float InSampleRate)
{
	if (InSampleRate != m_SampleRate)
		ResizeAudioTransform(m_MinimumFrequency, m_MaximumFrequency, InSampleRate, m_NumBands);

	if (InNumChannels != m_NumChannels)
		ResizeSpectrumBuffer(InNumChannels, m_NumBands);

	if ((InSampleRate != m_SampleRate) || (InNumChannels != m_NumChannels))
	{
		s32	FFTSize = m_FFTAlgorithm.IsValid() ? m_FFTAlgorithm->Size() : 0;

		ResizeWindow(InNumChannels, FFTSize);
	}

	m_NumChannels = InNumChannels;
	m_SampleRate = InSampleRate;
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::SetNoiseFloor(float InNoiseFloorDb)
{
	m_NoiseFloorDb = FMath::Clamp(InNoiseFloorDb, -200.0f, 10.0f);
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::ResizeCQT(float InMinimumFrequency, float InMaximumFrequency, s32 InNumBands)
{
	Clamp(InMinimumFrequency, InMaximumFrequency, InNumBands);

	// Check which values were changed
	const bool	bUpdatedMinimumFrequency = InMinimumFrequency != m_MinimumFrequency;
	const bool	bUpdatedMaximumFrequency = InMaximumFrequency != m_MaximumFrequency;
	const bool	bUpdatedNumBands = InNumBands != m_NumBands;

	const bool	bUpdatedValues = bUpdatedMinimumFrequency || bUpdatedMaximumFrequency || bUpdatedNumBands;

	if (bUpdatedNumBands)
		ResizeSpectrumBuffer(m_NumChannels, InNumBands);

	if (bUpdatedValues)
	{
		const s32	OriginalFFTSize = m_FFTAlgorithm.IsValid() ? m_FFTAlgorithm->Size() : 0;

		ResizeAudioTransform(InMinimumFrequency, InMaximumFrequency, m_SampleRate, InNumBands);

		const s32	NewFFTSize = m_FFTAlgorithm.IsValid() ? m_FFTAlgorithm->Size() : 0;

		if (NewFFTSize != OriginalFFTSize)
			ResizeWindow(m_NumChannels, NewFFTSize);

	}

	// Copy values internally
	m_MinimumFrequency 	= InMinimumFrequency;
	m_MaximumFrequency 	= InMaximumFrequency;
	m_NumBands			= InNumBands;
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::ResizeSpectrumBuffer(s32 InNumChannels, s32 InNumBands)
{
	for (s32 ChannelIndex = m_ChannelSpectrumBuffers.Num(); ChannelIndex < InNumChannels; ChannelIndex++)
		m_ChannelSpectrumBuffers.AddDefaulted();

	for (s32 ChannelIndex = 0; ChannelIndex < InNumChannels; ChannelIndex++)
	{
		Audio::FAlignedFloatBuffer	&Buffer = m_ChannelSpectrumBuffers[ChannelIndex];
		Buffer.Reset();
		if (InNumBands > 0)
			Buffer.AddUninitialized(InNumBands);
	}
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::ResizeAudioTransform(float InMinimumFrequency, float InMaximumFrequency,
																float InSampleRate, s32 InNumBands)
{
	m_FFTAlgorithm.Reset();
	m_CQTKernel.Reset();
	m_FFTInputBuffer.Reset();
	m_FFTOutputBuffer.Reset();
	m_PowerSpectrumBuffer.Reset();
	m_SpectrumBuffer.Reset();

	m_FFTScale = 1.f;

	if (InSampleRate <= 0.0f)
		return;

	if (InNumBands < 1)
		return;

	const float	NumOctaves = GetNumOctaves(InMinimumFrequency, InMaximumFrequency);
	const float	NumBandsPerOctave = GetNumBandsPerOctave(InNumBands, NumOctaves);

	Audio::FFFTSettings	FFTSettings;
	FFTSettings.bArrays128BitAligned = true;
	FFTSettings.bEnableHardwareAcceleration = true;
	FFTSettings.Log2Size = 11; // Output 1024 samples
	if (!Audio::FFFTFactory::AreFFTSettingsSupported(FFTSettings))
	{
		UE_LOG(LogTemp, Warning, TEXT("FFT settings are not supported"));
		return;
	}

	m_FFTAlgorithm = Audio::FFFTFactory::NewFFTAlgorithm(FFTSettings);

	if (!m_FFTAlgorithm.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create FFT"));
		return;
	}

	m_FFTInputBuffer.AddZeroed(m_FFTAlgorithm->NumInputFloats());
	m_FFTOutputBuffer.AddUninitialized(m_FFTAlgorithm->NumOutputFloats());
	m_PowerSpectrumBuffer.AddUninitialized(m_FFTOutputBuffer.Num() / 2);
	m_SpectrumBuffer.AddUninitialized(InNumBands);

	const float	FFTSize = static_cast<float>(m_FFTAlgorithm->Size());
	check(FFTSize > 0.0f);

	// We want to have all FFT implementations
	// return the same scaling so that the energy conservatino property of the fourier transform
	// is supported.  This scaling factor is applied to the power spectrum, so we square the 
	// scaling we would have performed on the magnitude spectrum.
	switch (m_FFTAlgorithm->ForwardScaling())
	{
		case Audio::EFFTScaling::MultipliedByFFTSize:
			m_FFTScale = 1.f / (FFTSize * FFTSize);
			break;
			
		case Audio::EFFTScaling::MultipliedBySqrtFFTSize:
			m_FFTScale = 1.f / FFTSize;
			break;

		case Audio::EFFTScaling::DividedByFFTSize:
			m_FFTScale = FFTSize * FFTSize;
			break;

		case Audio::EFFTScaling::DividedBySqrtFFTSize:
			m_FFTScale = FFTSize;
			break;

		default:
			m_FFTScale = 1.f;
			break;
	}

	const float	BandwidthStretch = GetBandwidthStretch(InSampleRate, FFTSize, NumBandsPerOctave, InMinimumFrequency);

	const Audio::FPseudoConstantQKernelSettings	CQTSettings = GetConstantQSettings(InMinimumFrequency, InMaximumFrequency, InNumBands, NumBandsPerOctave, BandwidthStretch);
	m_CQTKernel = Audio::NewPseudoConstantQKernelTransform(CQTSettings, m_FFTAlgorithm->Size(), InSampleRate);

	if (!m_CQTKernel.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create CQT kernel."));
		return;
	}
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::ResizeWindow(s32 InNumChannels, s32 InFFTSize)
{
	s32	NumWindowFrames = FMath::Max(1, InFFTSize);

	NumWindowFrames = FMath::Min(NumWindowFrames, 512);

	m_WindowBuffer.Reset();
	m_WindowBuffer.AddUninitialized(NumWindowFrames);

	Audio::GenerateBlackmanWindow(m_WindowBuffer.GetData(), NumWindowFrames, 1, true);

	s32	NumWindowSamples = NumWindowFrames;

	if (InNumChannels > 0)
		NumWindowSamples *= InNumChannels;

	// 50% overlap
	s32	NumHopSamples = FMath::Max(1, NumWindowSamples / 2);

	m_SlidingBuffer = MakeUnique<FSlidingBuffer>(NumWindowSamples, NumHopSamples);
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::AddSubmixListener(const Audio::FDeviceId InDeviceId)
{
	check(!m_SubmixListeners.Contains(InDeviceId));
	m_SubmixListeners.Emplace(InDeviceId, MakeShared<FPopcornFXSubmixListener>(m_PatchMixer,
																			   m_NumSamplesToBuffer,
																			   InDeviceId));
	m_SubmixListeners[InDeviceId]->RegisterToSubmix();
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::RemoveSubmixListener(const Audio::FDeviceId InDeviceId)
{
	if (m_SubmixListeners.Contains(InDeviceId))
	{
		m_SubmixListeners.Remove(InDeviceId);
	}	
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::RegisterToAllAudioDevices()
{
	if (FAudioDeviceManager *DeviceManager = FAudioDeviceManager::Get())
	{
		// Register a new submix listener for every audio device that currently exists.
		DeviceManager->IterateOverAllDevices([&](Audio::FDeviceId DeviceId, FAudioDevice* InDevice)
		{
			AddSubmixListener(DeviceId);
		});
	}	
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::UnregisterFromAllAudioDevices()
{
	m_SubmixListeners.Empty(0);
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::OnNewDeviceCreated(Audio::FDeviceId InDeviceId)
{
	if (m_IsSubmixListenerRegistered)
	{
		AddSubmixListener(InDeviceId);
	}
}

//----------------------------------------------------------------------------

void	APopcornFXAudioSamplerSubmixActor::OnDeviceDestroyed(Audio::FDeviceId InDeviceId)
{
	if (m_IsSubmixListenerRegistered)
		RemoveSubmixListener(InDeviceId);
}