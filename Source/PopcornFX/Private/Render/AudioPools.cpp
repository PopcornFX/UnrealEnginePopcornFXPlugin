//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "AudioPools.h"

#if (ENGINE_MAJOR_VERSION == 5)
#	include "AudioDevice.h"
#else
#	if PLATFORM_PS4
#		define PK_SPAWN_SOUNDS_WITH_GAMEPLAYSTATICS	1
#	else
#		include "AudioDevice.h"
#	endif // PLATFORM_PS4
#endif // (ENGINE_MAJOR_VERSION == 5)

#ifndef PK_SPAWN_SOUNDS_WITH_GAMEPLAYSTATICS
#	define PK_SPAWN_SOUNDS_WITH_GAMEPLAYSTATICS	0
#endif

#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXStats.h"

#include <pk_render_helpers/include/render_features/rh_features_basic.h>

//----------------------------------------------------------------------------
//
//	Sound pools
//
//----------------------------------------------------------------------------

CSoundDescriptor::~CSoundDescriptor()
{
	Clear();
}

//----------------------------------------------------------------------------

void	CSoundDescriptor::Clear()
{
	UAudioComponent		*comp = GetAudioComponentIFP();
	if (comp != null)
	{
		comp->ConditionalBeginDestroy();
		m_AudioComponent = null;
	}
}

//----------------------------------------------------------------------------

UAudioComponent		*CSoundDescriptor::GetAudioComponentIFP() const
{
	UAudioComponent		*comp = m_AudioComponent.Get();
	if (comp != null)
	{
		if (!comp->IsValidLowLevel()) // TWeakObjectPtr should have checked that
		{
			comp = null;
			m_AudioComponent = null;
		}
		else if (!comp->IsRegistered())
		{
			// respawn
			comp->ConditionalBeginDestroy();
			comp = null;
			m_AudioComponent = null;
		}
	}
	return comp;
}

//----------------------------------------------------------------------------

// @TODO optiom: cache Playing ?
bool	CSoundDescriptor::Playing() const
{
	UAudioComponent		*comp = GetAudioComponentIFP();
	if (comp == null)
		return false;
	return comp->IsPlaying();
}

//----------------------------------------------------------------------------

void	CSoundDescriptor::Update(SUpdateCtx &updCtx, const SSoundInsertDesc &insertDesc)
{
	UWorld				*world = updCtx.m_World;
	USoundBase			*sound = updCtx.m_Sound;

	m_UsedUpdateId = updCtx.m_CurrentUpdateId;

	PK_ASSERT(m_SelfID == insertDesc.m_SelfID);

	UAudioComponent		*comp = GetAudioComponentIFP();
	if (comp == null)
	{
		// Create one
#if (PK_SPAWN_SOUNDS_WITH_GAMEPLAYSTATICS != 0)
		comp = UGameplayStatics::SpawnSoundAtLocation(updCtx.m_World, updCtx.m_Sound, insertDesc.m_Position, FRotator::ZeroRotator, 1.f, 1.f, 0.f, null, null, false);
		if (!PK_VERIFY(comp != null))
			return;
		comp->bStopWhenOwnerDestroyed = true;
		m_AudioComponent = comp;
#else
		FAudioDevice::FCreateComponentParams	params(updCtx.m_World);
		params.bAutoDestroy = false;
		params.bPlay = false;
		params.bStopWhenOwnerDestroyed = true;
		params.SetLocation(insertDesc.m_Position);
		comp = FAudioDevice::CreateComponent(updCtx.m_Sound, params);
		PK_ASSERT(comp != null);
		m_AudioComponent = comp;
		comp->Play(0);//insertDesc.m_Age);
#endif // (PK_SPAWN_SOUNDS_WITH_GAMEPLAYSTATICS != 0)
	}

	if (m_LastPosition != insertDesc.m_Position)
	{
		m_LastPosition = insertDesc.m_Position;
		comp->SetWorldLocation(m_LastPosition);
	}

	if (comp->VolumeMultiplier != insertDesc.m_Volume)
	{
		comp->SetVolumeMultiplier(insertDesc.m_Volume);
	}


	bool	isPlaying = comp->IsPlaying();
	// @TODO if playing access FActiveSound for faster seek without full re-setup ?
	if (!isPlaying)
		comp->Play(0);
}

//----------------------------------------------------------------------------

void	CSoundDescriptor::Spawn(SUpdateCtx &updCtx, const SSoundInsertDesc &insertDesc)
{
	UWorld				*world = updCtx.m_World;
	USoundBase			*sound = updCtx.m_Sound;

	m_UsedUpdateId = updCtx.m_CurrentUpdateId;

	m_SelfID = insertDesc.m_SelfID;

	UAudioComponent		*comp = GetAudioComponentIFP();
	if (comp != null)
	{
		comp->ConditionalBeginDestroy();
		comp = null;
		m_AudioComponent = null;
	}

	if (comp == null)
	{
		// Create one
#if (PK_SPAWN_SOUNDS_WITH_GAMEPLAYSTATICS != 0)
		comp = UGameplayStatics::SpawnSoundAtLocation(updCtx.m_World, updCtx.m_Sound, insertDesc.m_Position, FRotator::ZeroRotator, 1.f, 1.f, 0.f, null, null, false);
		if (!PK_VERIFY(comp != null))
			return;
		comp->bStopWhenOwnerDestroyed = true;
		m_AudioComponent = comp;
#else
		FAudioDevice::FCreateComponentParams	params(updCtx.m_World);
		params.bAutoDestroy = false;
		params.bPlay = false;
		params.bStopWhenOwnerDestroyed = true;
		params.SetLocation(insertDesc.m_Position);
		comp = FAudioDevice::CreateComponent(updCtx.m_Sound, params);
		PK_ASSERT(comp != null);
		m_AudioComponent = comp;
		comp->Play(0/*m_LastAge*/);
#endif // (PK_SPAWN_SOUNDS_WITH_GAMEPLAYSTATICS != 0)
	}

	if (m_LastPosition != insertDesc.m_Position)
	{
		m_LastPosition = insertDesc.m_Position;
		comp->SetWorldLocation(m_LastPosition);
	}

	if (comp->VolumeMultiplier != insertDesc.m_Volume)
	{
		comp->SetVolumeMultiplier(insertDesc.m_Volume);
	}

	bool	isPlaying = comp->IsPlaying();
	// @TODO if playing access FActiveSound for faster seek without full re-setup ?
	if (!isPlaying)
	{
		comp->Play(0/*m_LastAge*/);
	}
}

//----------------------------------------------------------------------------

void	CSoundDescriptor::Unuse(SUpdateCtx &updCtx)
{
	UAudioComponent		*comp = GetAudioComponentIFP();
	if (comp == null)
		return;

	m_UsedUpdateId = 0;
	m_SelfID = CInt2(0);

	if (comp->IsPlaying())
		comp->Stop();
}

//----------------------------------------------------------------------------

void	CSoundDescriptor::Stop()
{
	UAudioComponent		*comp = GetAudioComponentIFP();
	if (comp == null)
		return;
	comp->ConditionalBeginDestroy();
	m_AudioComponent = null;
}

//----------------------------------------------------------------------------
//
//	CSoundDescriptorPool
//
//----------------------------------------------------------------------------

void	CSoundDescriptorPool::Clear()
{
	for (u32 i = 0, slotCount = m_Slots.Count(); i < slotCount; ++i)
		m_Slots[i].Clear();
	m_SoundsPlaying = 0;
}

//----------------------------------------------------------------------------

// @FIXME cache loaded sound ? or inc ref count ? but should alreay ref by effect ?

USoundBase	*CSoundDescriptorPool::GetOrLoadSound()
{
	return ::LoadObject<USoundBase>(null, *m_SoundPath);
}

//----------------------------------------------------------------------------

bool	CSoundDescriptorPool::Setup(CSoundDescriptorPoolCollection *poolCollection, PopcornFX::PCRendererDataBase rendererData)
{
	m_PoolCollection = poolCollection;

	static const u32		kPoolSize = 10;

	const PopcornFX::SRendererFeaturePropertyValue	*soundProperty = rendererData->m_Declaration.FindProperty(PopcornFX::BasicRendererProperties::SID_Sound_SoundData());
	if (soundProperty == null || soundProperty->ValuePath().Empty())
		return false;
	m_SoundPath = FPopcornFXPlugin::Get().BuildPathFromPkPath(soundProperty->ValuePath(), true);

	if (GetOrLoadSound() == null)
		return false;
	if (!m_Slots.Resize(kPoolSize))
		return false;

	return true;
}

//----------------------------------------------------------------------------

void	CSoundDescriptorPool::BeginInsert(UWorld *world)
{
	m_SoundsPlaying = 0;
}

//----------------------------------------------------------------------------

void	CSoundDescriptorPool::InsertSoundIFP(const SSoundInsertDesc &insertDesc)
{
	UWorld			*world = m_PoolCollection->World();
	const uint32	currentUpdateId = m_PoolCollection->CurrentUpdateId();

	USoundBase		*sound = GetOrLoadSound();
	if (!PK_VERIFY(sound != null))
	{
		Clear();
		return;
	}

	CSoundDescriptor::SUpdateCtx	updCtx(currentUpdateId, world, sound);
	PK_ASSERT(m_LastUpdatedSlotCount <= m_Slots.Count());
	bool	spawnLater = true;
	if (m_SoundsPlaying < m_Slots.Count())
	{
		PopcornFX::CGuid	slot;
		const CInt2			insertSelfID = insertDesc.m_SelfID;
		for (u32 i = 0; i < m_LastUpdatedSlotCount; ++i)
		{
			const CSoundDescriptor		&sd = m_Slots[i];
			if (sd.SelfID() != insertSelfID)
				continue;
			slot = i;
			break;
		}
		if (slot.Valid())
		{
			m_SoundsPlaying++;
			m_Slots[slot].Update(updCtx, insertDesc);
			spawnLater = false;
		}
	}
	if (spawnLater)
	{
		m_ToSpawn.PushBack(insertDesc);
	}
}

//----------------------------------------------------------------------------

void	CSoundDescriptorPool::EndInsert(UWorld *world)
{
	PK_ASSERT(world == m_PoolCollection->World());

	const uint32					currentUpdateId = m_PoolCollection->CurrentUpdateId();
	USoundBase						*sound = GetOrLoadSound();
	CSoundDescriptor::SUpdateCtx	updCtx(currentUpdateId, world, sound);

	u32					tospawni = 0;

	PopcornFX::CGuid	lastUsed = 0;

	PK_ASSERT(m_LastUpdatedSlotCount <= m_Slots.Count());
	u32		i = 0;
	for (; i < m_LastUpdatedSlotCount; ++i) // after m_LastUpdatedSlotCount: already unused
	{
		CSoundDescriptor	&sd = m_Slots[i];
		if (sd.UsedThisUpdate(currentUpdateId))
		{
			lastUsed = i;
			continue;
		}
		if (sound != null && tospawni < m_ToSpawn.Count())
		{
			sd.Spawn(updCtx, m_ToSpawn[tospawni]);
			++m_SoundsPlaying;
			++tospawni;
			lastUsed = i;
		}
		else
		{
			sd.Unuse(updCtx);
		}
	}
	// new sounds:
	if (sound != null && tospawni < m_ToSpawn.Count())
	{
		const u32	remainingToSpawnCount = m_ToSpawn.Count() - tospawni;
		const u32	finalCount = i + remainingToSpawnCount;
		if (m_Slots.Count() < finalCount)
		{
			if (!PK_VERIFY(m_Slots.Resize(finalCount)))
				return;
		}
		for (; tospawni < m_ToSpawn.Count(); ++i)
		{
			CSoundDescriptor	&sd = m_Slots[i];
			sd.Spawn(updCtx, m_ToSpawn[tospawni]);
			++tospawni;
			lastUsed = i;
		}
	}

	m_LastUpdatedSlotCount = lastUsed.Valid() ? u32(lastUsed) + 1U : 0U;

	INC_DWORD_STAT_BY(STAT_PopcornFX_SoundParticleCount, m_SoundsPlaying);

	m_ToSpawn.Clear();
}

//----------------------------------------------------------------------------
//
//	CSoundDescriptorPoolCollection
//
//----------------------------------------------------------------------------

void	CSoundDescriptorPoolCollection::Clear()
{
	for (u32 i = 0, poolCount = m_Pools.Count(); i < poolCount; ++i)
		m_Pools[i].Clear();
}

//----------------------------------------------------------------------------

void	CSoundDescriptorPoolCollection::BeginInsert(UWorld *world)
{
	m_World = world;
	++m_CurrentUpdateId;

	const double	currTime = FPlatformTime::Seconds();
	m_DeltaTime = currTime - m_LastTime;
	m_LastTime = currTime;

	for (u32 i = 0; i < m_Pools.Count(); ++i)
		m_Pools[i].BeginInsert(world);
}

//----------------------------------------------------------------------------

void	CSoundDescriptorPoolCollection::EndInsert(UWorld *world)
{
	PK_ASSERT(m_World == world);
	for (u32 i = 0; i < m_Pools.Count(); ++i)
		m_Pools[i].EndInsert(world);
}

//----------------------------------------------------------------------------
