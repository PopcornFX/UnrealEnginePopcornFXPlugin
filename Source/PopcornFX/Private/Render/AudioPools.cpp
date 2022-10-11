//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "AudioPools.h"

#if (PLATFORM_PS4)
//	Since 4.19.1/2.. can't include AudioDevice.h anymore because of AudioDeviceManager.h
//	raising:
//		Runtime/Engine/Public/AudioDeviceManager.h(135,2): error : declaration does not declare anything [-Werror,-Wmissing-declarations]
//		          void TrackResource(USoundWave* SoundWave, FSoundBuffer* Buffer);
//		          ^~~~
//	So we now use GameplayStatics directly

#	include "Kismet/GameplayStatics.h"
#else
#	include "AudioDevice.h"
#endif // PLATFORM_PS4

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

	//float		startpos = insertDesc.m_StartPos;
	//startpos -= FMath::Fmod(startpos, 1.0 / 120.0);
	PK_ASSERT(m_SelfID == insertDesc.m_SelfID);

	UAudioComponent		*comp = GetAudioComponentIFP();
	if (comp == null)
	{
		// Create one
#if (PLATFORM_PS4)
		comp = UGameplayStatics::SpawnSoundAtLocation(updCtx.m_World, updCtx.m_Sound, insertDesc.m_Position, FRotator::ZeroRotator, 1.f, 1.f, 0.f, null, null, false);
		if (!PK_VERIFY(comp != null))
			return;
		comp->bStopWhenOwnerDestroyed = true;
		m_AudioComponent = comp;
#else
		FAudioDevice::FCreateComponentParams	params(updCtx.m_World);
		params.bAutoDestroy = false;	// TODO: Test how this affects 4.15
		params.bPlay = false;
		params.bStopWhenOwnerDestroyed = true;
		params.SetLocation(insertDesc.m_Position);
		comp = FAudioDevice::CreateComponent(updCtx.m_Sound, params);
		PK_ASSERT(comp != null);
		m_AudioComponent = comp;
		comp->Play(0);//insertDesc.m_Age);
#endif // (PLATFORM_PS4)
		//PopcornFX::CLog::Log(PK_INFO, "Sound Update Spawned %d %p %f", m_UsedUpdateId, comp, insertDesc.m_Age);
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

//	comp->ComponentVelocity = insertDesc.m_Velocity; // don't really care, FAudioDevice compute it's own Pos- PrevPos

	bool	isPlaying = comp->IsPlaying();
	// @TODO if playing access FActiveSound for faster seek without full re-setup ?
	if (!isPlaying)
	{
		//PopcornFX::CLog::Log(PK_INFO, "Sound Update Start %d cp:%d %p %f", m_UsedUpdateId, comp, isPlaying, insertDesc.m_Age);
		//comp->Play(insertDesc.m_Age);
		comp->Play(0);
		//float	time = (PK_VERIFY(comp->Sound != null) ? FMath::Fmod(m_StartPos, comp->Sound->Duration) : 0);
		//comp->Play(m_StartPos);
	}
}

//----------------------------------------------------------------------------

void	CSoundDescriptor::Spawn(SUpdateCtx &updCtx, const SSoundInsertDesc &insertDesc)
{
	UWorld				*world = updCtx.m_World;
	USoundBase			*sound = updCtx.m_Sound;

	m_UsedUpdateId = updCtx.m_CurrentUpdateId;

	//float		startpos = insertDesc.m_StartPos;
	//startpos -= FMath::Fmod(startpos, 1.0 / 120.0);
	//m_LastAge = insertDesc.m_Age;
	m_SelfID = insertDesc.m_SelfID;

	UAudioComponent		*comp = GetAudioComponentIFP();
	if (comp != null)
	{
		//comp->Stop();
		comp->ConditionalBeginDestroy();
		comp = null;
		m_AudioComponent = null;
		//comp->FadeOut(0.1f, 0.f);
	}

	if (comp == null)
	{
		// Create one
#if (PLATFORM_PS4)
		comp = UGameplayStatics::SpawnSoundAtLocation(updCtx.m_World, updCtx.m_Sound, insertDesc.m_Position, FRotator::ZeroRotator, 1.f, 1.f, 0.f, null, null, false);
		if (!PK_VERIFY(comp != null))
			return;
		comp->bStopWhenOwnerDestroyed = true;
		m_AudioComponent = comp;
#else
		FAudioDevice::FCreateComponentParams	params(updCtx.m_World);
		params.bAutoDestroy = false;	// TODO: Test how this affects 4.15
		params.bPlay = false;
		params.bStopWhenOwnerDestroyed = true;
		params.SetLocation(insertDesc.m_Position);
		comp = FAudioDevice::CreateComponent(updCtx.m_Sound, params);
		PK_ASSERT(comp != null);
		m_AudioComponent = comp;
		comp->Play(0/*m_LastAge*/);
#endif // (PLATFORM_PS4)
		//PopcornFX::CLog::Log(PK_INFO, "Sound Spawn Spawned %d %p %f", m_UsedUpdateId, comp, 0.0f/*m_LastAge*/);
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

	//	comp->ComponentVelocity = insertDesc.m_Velocity; // don't really care, FAudioDevice compute it's own Pos- PrevPos

	bool	isPlaying = comp->IsPlaying();
	// @TODO if playing access FActiveSound for faster seek without full re-setup ?
	if (!isPlaying)
	{
		//PopcornFX::CLog::Log(PK_INFO, "Sound Spawn Start cp:%d %p p:%d %f", m_UsedUpdateId, comp, isPlaying, 0.0f/*m_LastAge*/);
		comp->Play(0/*m_LastAge*/);
		//float	time = (PK_VERIFY(comp->Sound != null) ? FMath::Fmod(m_StartPos, comp->Sound->Duration) : 0);
		//comp->Play(m_StartPos);
	}
}

//----------------------------------------------------------------------------

void	CSoundDescriptor::Unuse(SUpdateCtx &updCtx)
{
	//if (UsedThisUpdate(updCtx.m_CurrentUpdateId))
	//	return;
	UAudioComponent		*comp = GetAudioComponentIFP();
	if (comp == null)
		return;

	m_UsedUpdateId = 0;
	m_SelfID = CInt2(0);
	//m_UsedUpdateId = updCtx.m_CurrentUpdateId;
	//m_UsedUpdateId = 0;
	//m_StartPos = insertDesc.m_StartPos;

	if (comp->IsPlaying())
	{
		//PopcornFX::CLog::Log(PK_INFO, "Sound Unuse %d(last use: %d) %p %f", updCtx.m_CurrentUpdateId, m_UsedUpdateId, comp, 0.0f/*m_LastAge*/);
		comp->Stop();
	}

	//comp->Stop();
}

//----------------------------------------------------------------------------

void	CSoundDescriptor::Stop()
{
	UAudioComponent		*comp = GetAudioComponentIFP();
	if (comp == null)
		return;
	//comp->bAutoDestroy = true;
	//comp->Stop();
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
	m_SoundPath = FPopcornFXPlugin::Get().BuildPathFromPkPath(soundProperty->ValuePath().Data(), true);

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

//	const float		dt = m_PoolCollection->CurrentUpdateDt();
//	const float		dt = m_PoolCollection->World()->GetAudioDevice()->GetDeviceDeltaTime();
//	const float		maxAllowedDelta = dt + 0.9e-2f;

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
	{
		m_Pools[i].BeginInsert(world);
	}
}

//----------------------------------------------------------------------------

void	CSoundDescriptorPoolCollection::EndInsert(UWorld *world)
{
	PK_ASSERT(m_World == world);
	for (u32 i = 0; i < m_Pools.Count(); ++i)
	{
		m_Pools[i].EndInsert(world);
	}
}

//----------------------------------------------------------------------------
