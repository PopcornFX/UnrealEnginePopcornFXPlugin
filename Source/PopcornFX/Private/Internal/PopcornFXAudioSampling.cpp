//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXAudioSampling.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXStats.h"
#include "PopcornFXSDK.h"

//----------------------------------------------------------------------------

FPopcornFXFillAudioBuffers::FPopcornFXFillAudioBuffers()
{

}

//----------------------------------------------------------------------------

FPopcornFXFillAudioBuffers::~FPopcornFXFillAudioBuffers()
{
	for (s32 iAudioPyramid = 0; iAudioPyramid < m_AudioSpectrumPyramids.Num(); ++iAudioPyramid)
		CleanAudioPyramid(m_AudioSpectrumPyramids[iAudioPyramid]);
	for (s32 iAudioPyramid = 0; iAudioPyramid < m_AudioWaveformPyramids.Num(); ++iAudioPyramid)
		CleanAudioPyramid(m_AudioWaveformPyramids[iAudioPyramid]);
	m_AudioSpectrumPyramids.Empty();
	m_AudioWaveformPyramids.Empty();
}

//----------------------------------------------------------------------------

void	FPopcornFXFillAudioBuffers::RegisterAudioChannel(FName ChannelName)
{
	if (!ChannelName.IsValid())
		return;
	m_AudioChannels.AddUnique(ChannelName);
}

//----------------------------------------------------------------------------

void	FPopcornFXFillAudioBuffers::UnregisterAudioChannel(FName ChannelName)
{
	if (!ChannelName.IsValid())
		return;
	m_AudioChannels.Remove(ChannelName);
}

//----------------------------------------------------------------------------

void	FPopcornFXFillAudioBuffers::CleanAudioPyramid(SAudioPyramid &audioPyramid)
{
	TArray<float*>		&pyramid = audioPyramid.m_ConvolutionPyramid;
	const u32			pyramidSize = pyramid.Num();
	for (u32 iLevel = 0; iLevel < pyramidSize; ++iLevel)
	{
		if (pyramid[iLevel] == null)
			break;
		PK_FREE(pyramid[iLevel]);
		pyramid[iLevel] = null;
	}
	pyramid.Empty();

	audioPyramid.m_AudioSampleCount = 0;
	audioPyramid.m_Valid = false;
}

//----------------------------------------------------------------------------
//
//					Game thread, before/after PKFX simulation
//
//----------------------------------------------------------------------------

void	FPopcornFXFillAudioBuffers::BuildAudioPyramid(SAudioPyramid &audioPyramid, const float * const rawBuffer, uint32 bufferSize)
{
	PK_NAMEDSCOPEDPROFILE_C("FPopcornFXFillAudioBuffers::BuildAudioPyramid", POPCORNFX_UE_PROFILER_COLOR);

	TArray<float*>	&pyramid = audioPyramid.m_ConvolutionPyramid;
	if (pyramid.Num() == 0 || audioPyramid.m_AudioSampleCount != bufferSize)
	{
		CleanAudioPyramid(audioPyramid);

		const u32	pyramidSize = PopcornFX::IntegerTools::Log2(bufferSize) + 1;
		pyramid.SetNum(pyramidSize);

		u32	sampleCount = bufferSize;
		for (u32 iLevel = 0; iLevel < pyramidSize; ++iLevel)
		{
			PK_ASSERT(sampleCount != 0);

			// Allocate border bytes to ensure filtering doesn't overflow
			const u32	byteCount = (2 + sampleCount + 2) * sizeof(float);
			pyramid[iLevel] = static_cast<float*>(PK_MALLOC_ALIGNED(byteCount, 0x10));
			if (!PK_VERIFY(pyramid[iLevel] != null))
			{
				CleanAudioPyramid(audioPyramid);
				return;
			}
			PopcornFX::Mem::Clear(pyramid[iLevel], byteCount);
			sampleCount >>= 1;
		}
		audioPyramid.m_AudioSampleCount = bufferSize;
	}

	float	*highResAudioData = pyramid[0];
	PK_ASSERT(highResAudioData != null);

	PopcornFX::Mem::Copy(highResAudioData + 2, rawBuffer, sizeof(*highResAudioData) * bufferSize);

	// Compute convolution pyramid
	u32			sampleCount = bufferSize;
	const u32	pyramidSize = pyramid.Num();
	for (u32 iLevel = 0; iLevel < pyramidSize; ++iLevel)
	{
		PK_ASSERT(sampleCount > 0);

		float	* __restrict dstRawAudioData = 2 + pyramid[iLevel];
		if (iLevel > 0)
		{
			const float	* __restrict srcRawAudioData = 2 + pyramid[iLevel - 1];

			for (u32 iSample = 0; iSample < sampleCount; ++iSample)
				dstRawAudioData[iSample] = 0.5f * (srcRawAudioData[iSample * 2 + 0] + srcRawAudioData[iSample * 2 + 1]);
		}

		const float	firstValue = dstRawAudioData[0];
		const float	lastValue = dstRawAudioData[bufferSize - 1];

		// Fill border with same values
		dstRawAudioData[-1] = firstValue;
		dstRawAudioData[-2] = firstValue;
		dstRawAudioData[sampleCount + 0] = lastValue;
		dstRawAudioData[sampleCount + 1] = lastValue;

		sampleCount >>= 1;
	}
	audioPyramid.m_Valid = true;
}

//----------------------------------------------------------------------------

void	FPopcornFXFillAudioBuffers::PreUpdate()
{
	PK_ASSERT(IsInGameThread());
	PK_ASSERT(m_AudioSpectrumPyramids.Num() == m_AudioWaveformPyramids.Num());

	const u32	audioChannelCount = m_AudioChannels.Num();
	if (m_AudioSpectrumPyramids.Num() != audioChannelCount)
	{
		m_AudioSpectrumPyramids.SetNum(audioChannelCount);
		m_AudioWaveformPyramids.SetNum(audioChannelCount);
	}

	for (u32 iChannel = 0; iChannel < audioChannelCount; ++iChannel)
	{
		const FName	&audioChannelName = m_AudioChannels[iChannel];

		PK_ASSERT(!audioChannelName.IsNone() && audioChannelName.IsValid());

		uint32		spectrumBufferSize = 0;
		uint32		waveformBufferSize = 0;
		const float	*rawSpectrumBuffer = null;
		const float	*rawWaveformBuffer = null;

		{
			SCOPE_CYCLE_COUNTER(STAT_PopcornFX_ComputeAudioSpectrumTime);
			rawSpectrumBuffer = GetRawSpectrumBuffer(audioChannelName, spectrumBufferSize);
		}
		{
			SCOPE_CYCLE_COUNTER(STAT_PopcornFX_ComputeAudioWaveformTime);
			rawWaveformBuffer = GetRawWaveformBuffer(audioChannelName, waveformBufferSize);
		}
		PK_ASSERT(spectrumBufferSize == waveformBufferSize ||
				  spectrumBufferSize == 0 ||
				  waveformBufferSize == 0);

		if (rawSpectrumBuffer != null && spectrumBufferSize > 0)
		{
			PK_ASSERT(PopcornFX::IntegerTools::IsPowerOfTwo(spectrumBufferSize));
			BuildAudioPyramid(m_AudioSpectrumPyramids[iChannel], rawSpectrumBuffer, spectrumBufferSize);
		}
		if (rawWaveformBuffer != null && waveformBufferSize > 0)
		{
			PK_ASSERT(PopcornFX::IntegerTools::IsPowerOfTwo(waveformBufferSize));
			BuildAudioPyramid(m_AudioWaveformPyramids[iChannel], rawWaveformBuffer, waveformBufferSize);
		}
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXFillAudioBuffers::PostUpdate()
{
	for (s32 iAudioPyramid = 0; iAudioPyramid < m_AudioSpectrumPyramids.Num(); ++iAudioPyramid)
		m_AudioSpectrumPyramids[iAudioPyramid].m_Valid = false;
	for (s32 iAudioPyramid = 0; iAudioPyramid < m_AudioWaveformPyramids.Num(); ++iAudioPyramid)
		m_AudioWaveformPyramids[iAudioPyramid].m_Valid = false;
}

//----------------------------------------------------------------------------
//
//					Called during PKFX simulation
//
//----------------------------------------------------------------------------

const float * const	*FPopcornFXFillAudioBuffers::AsyncGetAudioSpectrum(const FName &channelName, uint32 &outBaseCount) const
{
	PK_ASSERT(m_AudioChannels.Num() == m_AudioSpectrumPyramids.Num());
	PK_ASSERT(channelName.IsValid());

	const u32	audioChannelCount = m_AudioChannels.Num();
	for (u32 iChannel = 0; iChannel < audioChannelCount; ++iChannel)
	{
		if (m_AudioChannels[iChannel] != channelName)
			continue;
		const SAudioPyramid	&audioPyramid = m_AudioSpectrumPyramids[iChannel];
		if (!audioPyramid.m_Valid)
			break;

		outBaseCount = 1U << (audioPyramid.m_ConvolutionPyramid.Num() - 1);
		return audioPyramid.m_ConvolutionPyramid.GetData();
	}
	return null;
}

//----------------------------------------------------------------------------

const float * const	*FPopcornFXFillAudioBuffers::AsyncGetAudioWaveform(const FName &channelName, uint32 &outBaseCount) const
{
	PK_ASSERT(m_AudioChannels.Num() == m_AudioWaveformPyramids.Num());
	PK_ASSERT(channelName.IsValid());

	const u32	audioChannelCount = m_AudioChannels.Num();
	for (u32 iChannel = 0; iChannel < audioChannelCount; ++iChannel)
	{
		if (m_AudioChannels[iChannel] != channelName)
			continue;
		const SAudioPyramid	&audioPyramid = m_AudioWaveformPyramids[iChannel];
		if (!audioPyramid.m_Valid)
			break;

		outBaseCount = 1U << (audioPyramid.m_ConvolutionPyramid.Num() - 1);
		return audioPyramid.m_ConvolutionPyramid.GetData();
	}
	return null;
}

//----------------------------------------------------------------------------
