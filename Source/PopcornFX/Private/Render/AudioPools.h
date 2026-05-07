//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXSDK.h"
#include <pk_render_helpers/include/draw_requests/rh_sound.h>

class	UAudioComponent;
class	USoundBase;
class	UWorld;

class	CSoundDescriptorPoolCollection;
class	CSoundDescriptorPool;

struct	SSoundInsertDesc
{
	// in UE space
	CInt2		m_SelfID;
	float		m_Age;
	FVector		m_Position;
	FVector		m_Velocity;
	float		m_Radius;
	float		m_DopplerLevel;
	float		m_Volume;
	bool		m_Audible;
};

class	CSoundDescriptor
{
public:
	struct	SUpdateCtx
	{
		uint32			m_CurrentUpdateId;
		UWorld			*m_World;
		USoundBase		*m_Sound;
		SUpdateCtx(uint32 frameId, UWorld *world, USoundBase *sound)
		:	m_CurrentUpdateId(frameId)
		,	m_World(world)
		,	m_Sound(sound)
		{
		}
	};

	CSoundDescriptor() { }
	~CSoundDescriptor();

	void		Clear();

	bool		UsedThisUpdate(uint32 currentUpdateId) const { return currentUpdateId == m_UsedUpdateId; }
	uint32		LastUsedUpdateId() const { return m_UsedUpdateId; }
	CInt2		SelfID() const { return m_SelfID; }
	FVector		LastPosition() const { return m_LastPosition; }
	bool		Playing() const;

	void		Update(SUpdateCtx &updCtx, const SSoundInsertDesc &insertDesc);
	void		Spawn(SUpdateCtx &updCtx, const SSoundInsertDesc &insertDesc);
	void		Unuse(SUpdateCtx &updCtx);

	void		Stop();

private:
	UAudioComponent		*GetAudioComponentIFP() const;

private:
	CInt2		m_SelfID = CInt2(0);
	uint32		m_UsedUpdateId = 0;
	FVector		m_LastPosition;
	mutable TWeakObjectPtr<UAudioComponent>		m_AudioComponent;
};

class	CSoundDescriptorPoolCollection;

class	CSoundDescriptorPool
{
public:
	CSoundDescriptorPool() {}

	bool			Setup(CSoundDescriptorPoolCollection *poolCollection, PopcornFX::PCRendererDataBase rendererData);
	void			Clear();

	USoundBase		*GetOrLoadSound();

	void			BeginInsert(UWorld *world);
	void			InsertSoundIFP(const SSoundInsertDesc &insertDesc);
	void			EndInsert(UWorld *world);

private:
	CSoundDescriptorPoolCollection		*m_PoolCollection = null;

	u32			m_SoundsPlaying = 0;
	u32			m_LastUpdatedSlotCount = 0;

	//USoundBase	*m_SoundBase = null;
	FString		m_SoundPath;

	double		m_MaxDeltaPlaying = 0.0;

	PopcornFX::TArray<CSoundDescriptor>		m_Slots;
	PopcornFX::TArray<SSoundInsertDesc>		m_ToSpawn;
};

class	CSoundDescriptorPoolCollection
{
public:
	PopcornFX::TArray<CSoundDescriptorPool>		m_Pools;

public:
	CSoundDescriptorPoolCollection() { }

	void	Clear();

	uint32	CurrentUpdateId() const { return m_CurrentUpdateId; }
	float	CurrentUpdateDt() const { return m_DeltaTime; }
	UWorld	*World() const { return m_World; }

	void	BeginInsert(UWorld *world);
	void	EndInsert(UWorld *world);

private:
	uint32		m_CurrentUpdateId = 0;
	UWorld		*m_World = null;
	double		m_LastTime = 0;
	float		m_DeltaTime = 0;
};

