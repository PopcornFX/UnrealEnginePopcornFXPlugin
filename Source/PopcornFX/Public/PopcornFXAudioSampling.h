//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

// Generic audio buffer fill interface, inherit this one if you want full control
class	IPopcornFXFillAudioBuffers
{
public:
	virtual void	PreUpdate() = 0;
	virtual void	PostUpdate() = 0;

	virtual const float * const	*AsyncGetAudioSpectrum(const FName &channelName, uint32 &outBaseCount) const = 0;
	virtual const float * const	*AsyncGetAudioWaveform(const FName &channelName, uint32 &outBaseCount) const = 0;
};

// PopcornFX default implementation
class	FPopcornFXFillAudioBuffers : public IPopcornFXFillAudioBuffers
{
public:
	FPopcornFXFillAudioBuffers();
	virtual	~FPopcornFXFillAudioBuffers();

	// Called from main thread
	virtual const float	*GetRawSpectrumBuffer(const FName &ChannelName, uint32 &OutBufferSize) = 0;
	virtual const float	*GetRawWaveformBuffer(const FName &ChannelName, uint32 &OutBufferSize) = 0;

	// Called from main thread, override them if needed but make sure to call this class implementation
	virtual void	PreUpdate() override;
	virtual void	PostUpdate() override;
public:
	// Called during PKFX simulation
	const float * const	*AsyncGetAudioSpectrum(const FName &channelName, uint32 &outBaseCount) const override;
	const float * const	*AsyncGetAudioWaveform(const FName &channelName, uint32 &outBaseCount) const override;
protected:
	void			RegisterAudioChannel(FName ChannelName);
	void			UnregisterAudioChannel(FName ChannelName);

	TArray<FName>	m_AudioChannels;
private:
	struct	SAudioPyramid
	{
		bool			m_Valid;
		uint32			m_AudioSampleCount;
		TArray<float*>	m_ConvolutionPyramid;

		SAudioPyramid() : m_Valid(false), m_AudioSampleCount(0) { }
	};

	void	CleanAudioPyramid(SAudioPyramid &pyramid);
	void	BuildAudioPyramid(SAudioPyramid &audioPyramid, const float * const rawBuffer, uint32 bufferSize);

	TArray<SAudioPyramid>	m_AudioSpectrumPyramids;
	TArray<SAudioPyramid>	m_AudioWaveformPyramids;
};
