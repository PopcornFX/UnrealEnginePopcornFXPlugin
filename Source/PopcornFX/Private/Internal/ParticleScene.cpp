//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "ParticleScene.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXSceneComponent.h"
#include "World/PopcornFXSceneProxy.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "Render/RendererSubView.h"
#include "PopcornFXAudioSampling.h"
#include "HardwareInfo.h"
#include "PopcornFXPayloadHelper.h"
#include "Assets/PopcornFXEffectPriv.h"

#ifdef USE_DEFAULT_AUDIO_INTERFACE
#	include "PopcornFXAudioDefault.h"
#endif

#include "PopcornFXStats.h"
#include "GPUSim/PopcornFXGPUSim.h"
#include "Render/RenderBatchManager.h"

#if PK_WITH_PHYSX
#	ifdef WITH_APEX
#		undef WITH_APEX
#		define WITH_APEX		0
#	endif

#	include "PhysXIncludes.h"
#	include "PhysXPublic.h"
#endif

#if PK_WITH_PHYSX || PK_WITH_CHAOS
#	include "Physics/PhysicsFiltering.h"
#endif

#if PK_WITH_CHAOS
#	include "SQAccelerator.h"

#	if (ENGINE_MAJOR_VERSION == 4)
#	include "Experimental/ChaosInterfaceWrapper.h"
#	include "SQVerifier.h"
#	endif // (ENGINE_MAJOR_VERSION == 4)

#	include "PhysTestSerializer.h"

#	include "PBDRigidsSolver.h"
#	include "Chaos/ChaosScene.h"
#	include "Chaos/PBDRigidsEvolutionGBF.h"
#endif

#include "Collision.h"
#include "PhysicsPublic.h"

#include "CustomPhysXPayload.h"

#include "Kismet/GameplayStatics.h"
#include "RenderingThread.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#if WITH_EDITOR
#	include "Editor.h"
#	include "LevelEditorViewport.h"
#endif // WITH_EDITOR

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_mediums.h>
#include <pk_particles/include/ps_effect.h>
#include <pk_particles/include/ps_stats.h>
#include <pk_particles/include/ps_scene.h>

#include <pk_geometrics/include/ge_coordinate_frame.h>

#include <pk_kernel/include/kr_profiler.h>
#include <pk_kernel/include/kr_containers_onstack.h>

#include <pk_particles/include/Updaters/CPU/updater_cpu.h>
#include <pk_particles/include/Updaters/Auto/updater_auto.h>
#include <pk_particles_toolbox/include/pt_transforms.h>

#if (PK_GPU_D3D11 == 1)
#	include <d3d11.h>
#	include <pk_particles/include/Updaters/D3D11/updater_d3d11.h>
#endif
#if (PK_GPU_D3D12 == 1)

#	if PLATFORM_WINDOWS
#		ifdef WINDOWS_PLATFORM_TYPES_GUARD
#			include "Windows/HideWindowsPlatformTypes.h"
#		endif
#	elif PLATFORM_XBOXONE
#		ifdef XBOXONE_PLATFORM_TYPES_GUARD
#			include "XboxOneHidePlatformTypes.h"
#		endif
#	endif

#	include "Misc/CoreDelegates.h"

#	if defined(PK_UNKNOWN) && !defined(__d3dcommon_h__)
#		define __d3dcommon_h__
#	endif // defined(PK_UNKNOWN)

#	include "D3D12RHIPrivate.h"

#	include <pk_particles/include/Updaters/D3D12/updater_d3d12.h>
#endif

#include "PopcornFXEmitterComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXScene, Log, All);

//----------------------------------------------------------------------------

// static
CParticleScene	*CParticleScene::CreateNew(const UPopcornFXSceneComponent *sceneComp)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::CreateNew", POPCORNFX_UE_PROFILER_COLOR);

	CParticleScene	*newScene = null;
	if (IPopcornFXPlugin::IsAvailable())
	{
		// we dono if SafeDelete() will be called before PopcornFX shutdown,
		// so use `new` because PK_DELETE might not be available at SafeDelete() time
		newScene = new CParticleScene;
		if (newScene != null)
		{
			if (!newScene->InternalSetup(sceneComp))
			{
				delete newScene;
				newScene = null;
			}
		}
	}
	return newScene;
}

//----------------------------------------------------------------------------

// static
void	CParticleScene::SafeDelete(CParticleScene *&scene)
{
	if (scene != null)
		delete scene;
	scene = null;
}

//----------------------------------------------------------------------------

CParticleScene::CParticleScene()
:	m_ParticleMediumCollection(null)
,	m_FillAudioBuffers(null)
,	m_AudioInterface(null)
#if (PK_GPU_D3D11 == 1)
,	m_EnableD3D11(false)
#endif
#if (PK_GPU_D3D12 == 1)
,	m_EnableD3D12(false)
#endif
{
	// We could have this as a global to the plugin, medium collection agnostic
#ifdef USE_DEFAULT_AUDIO_INTERFACE
	m_AudioInterfaceDefault = new FPopcornFXAudioDefault();
	m_AudioInterface = m_AudioInterfaceDefault;
#endif // (USE_DEFAULT_AUDIO_INTERFACE)
}

//----------------------------------------------------------------------------

CParticleScene::~CParticleScene()
{
	Clear();

#if (PK_GPU_D3D12 == 1)
	if (m_EnableD3D12)
	{
		CParticleScene	*self = this;

		// Before cleaning the medium collection, we need to flush remaining command lists
		// Internally the medium collection will wait on fences, which will deadlock
		ENQUEUE_RENDER_COMMAND(PopcornFXFlushGPUSimTasks)(
			[self](FRHICommandListImmediate& RHICmdList)
			{
				self->D3D12_SubmitSimCommandLists();
			});

		FlushRenderingCommands();
	}
#endif

	m_RenderBatchManager->Clean();
	m_RenderBatchManager = null;

#if (PK_HAS_GPU != 0)
	GPU_Destroy();
#endif // (PK_HAS_GPU != 0)

	if (m_ParticleMediumCollection != null)
	{
		PK_DELETE(m_ParticleMediumCollection);
		m_ParticleMediumCollection = null;
	}

	m_FillAudioBuffers = null;
	m_AudioInterface = null;

#ifdef USE_DEFAULT_AUDIO_INTERFACE
	delete m_AudioInterfaceDefault;
	m_AudioInterfaceDefault = null;
#endif // (USE_DEFAULT_AUDIO_INTERFACE == 1)
}

//----------------------------------------------------------------------------

bool	CParticleScene::InternalSetup(const UPopcornFXSceneComponent *sceneComp)
{
	PK_ASSERT(sceneComp != null);
	m_SceneComponent = sceneComp;

	PopcornFX::CParticleUpdateManager_Auto	*updateManager = null;

#if (PK_HAS_GPU != 0)
	const FString				hardwareDetails = FHardwareInfo::GetHardwareDetailsString();
	SUERenderContext::ERHIAPI		API = SUERenderContext::_Unsupported;

#define	PK_NEW_UPDATER_IFN(__name) \
	m_Enable ## __name = FModuleManager::Get().IsModuleLoaded(PK_STRINGIFY(__name) "RHI") && hardwareDetails.Contains(PK_STRINGIFY(__name)); \
	if (m_Enable ## __name) \
	{ \
		PK_ASSERT(updateManager == null); \
		updateManager = PopcornFX::CParticleUpdateManager_Auto::New(PopcornFX::EGPUContext::Context ## __name); \
		API = SUERenderContext:: ## __name; \
	} \

#	if (PK_GPU_D3D11 == 1)
	PK_NEW_UPDATER_IFN(D3D11);
#	endif
#	if (PK_GPU_D3D12 == 1)
	PK_NEW_UPDATER_IFN(D3D12);
#	endif

	// This is actually an error in PopcornFX.Build.cs, PK_HAS_GPU shouldn't be defined on platforms where it's not supported!
	// See #5574
	if (updateManager != null)
	{
		updateManager->SetPreferredSimLocation(PopcornFX::CParticleUpdateManager_Auto::SimLocation_Auto); // Enable GPU but does not force it
		SetupPopcornFXRHIAPI(API);
	}
#endif

	m_ParticleMediumCollection = PK_NEW(PopcornFX::CParticleMediumCollection(this, updateManager));
	if (!PK_VERIFY(m_ParticleMediumCollection != null))
		return false;

	m_RenderBatchManager = PK_NEW(CRenderBatchManager);
	if (!PK_VERIFY(m_RenderBatchManager != null))
		return false;

	m_RenderBatchManager->Setup(this, m_ParticleMediumCollection
#if (PK_HAS_GPU != 0)
		, API
#endif // (PK_HAS_GPU != 0)
		);

	m_ParticleMediumCollection->EnableBounds(true);

	// Hook the simulation dispatch filtering callback.
	// This is what allows us to forbid certain layers from being run on the GPU
	// Here, we don't support the light, ribbon, mesh, and sound renderers in GPU sims,
	// so this callback makes layers fallback on the CPU whenever they have any of those.
	m_ParticleMediumCollection->SetSimDispatchMaskCallback(PopcornFX::CParticleMediumCollection::CbSimDispatchMask(&_SimDispatchMask));

	if (updateManager != null)
	{
#if (PK_HAS_GPU != 0)
		if (!GPU_InitIFN())
#endif // (PK_HAS_GPU != 0)
			updateManager->SetPreferredSimLocation(PopcornFX::CParticleUpdateManager_Auto::SimLocation_CPU); // Disable GPU sim
	}

	m_CurrentPayloadView = new SPopcornFXPayloadView();

	return true;
}

//----------------------------------------------------------------------------
// Simulation callback dispatch mask: Disable renderers not supported on GPU

bool	CParticleScene::_SimDispatchMask(const PopcornFX::CParticleDescriptor *descriptor, PopcornFX::SSimDispatchHint &outHint)
{
	const PopcornFX::SParticleDeclaration	&decl = descriptor->ParticleDeclaration();

#if (PK_HAS_GPU != 0)
	if (!decl.m_LightRenderers.Empty() ||
		!decl.m_SoundRenderers.Empty() ||
		!decl.m_RibbonRenderers.Empty() ||
		!decl.m_TriangleRenderers.Empty() ||
		!decl.m_DecalRenderers.Empty())
	{
		outHint.m_AllowedDispatchMask &= ~(1 << PopcornFX::SSimDispatchHint::Location_GPU);	// can't draw any of these on the GPU
	}
	const bool	preferGPU = descriptor->PreferredSimLocation() == PopcornFX::SimLocationHint_GPU;

	// if we are allowed on GPU & layer is set to 'GPU', prefer GPU, otherwise prefer CPU:
	if (outHint.m_AllowedDispatchMask & (1 << PopcornFX::SSimDispatchHint::Location_GPU) && preferGPU)
		outHint.m_PreferredDispatchMask = (1 << PopcornFX::SSimDispatchHint::Location_GPU);
	else
		outHint.m_PreferredDispatchMask = (1 << PopcornFX::SSimDispatchHint::Location_CPU);
#else	// (PK_HAS_GPU != 0)
	outHint.m_PreferredDispatchMask = (1 << PopcornFX::SSimDispatchHint::Location_CPU);
	outHint.m_AllowedDispatchMask = outHint.m_PreferredDispatchMask;
#endif	// (PK_HAS_GPU != 0)

	return true;
}

//----------------------------------------------------------------------------

void	CParticleScene::Clear()
{
	PK_ASSERT(FPopcornFXPlugin::IsMainThread());

	PK_ASSERT(m_ParticleMediumCollection != null && !m_ParticleMediumCollection->UpdatePending());

	PK_SCOPEDLOCK(m_UpdateLock);

	m_RenderBatchManager->Clear();

	PK_ASSERT(m_ParticleMediumCollection != null);
	m_ParticleMediumCollection->ClearAllViews();
	m_ParticleMediumCollection->Clear();

	FPopcornFXPlugin::IncTotalParticleCount(-m_LastTotalParticleCount);
	m_LastTotalParticleCount = 0;

	_Clear();
}

//----------------------------------------------------------------------------

const UPopcornFXSceneComponent		*CParticleScene::SceneComponent() const
{
	PK_ASSERT(m_SceneComponent != null && m_SceneComponent->IsValidLowLevel());
	return m_SceneComponent;
}

//----------------------------------------------------------------------------

u32	CParticleScene::_InstanceCount(const PopcornFX::CParticleEffect *effect)
{
	PK_ASSERT(effect != null);
	PK_SCOPEDLOCK(m_EmittersLock);

	u32	instanceCount = 0;
	for (u32 i = 0; i < m_Emitters.Count(); ++i)
	{
		const SEmitterRegister	&emitter = m_Emitters[i];
		if (!emitter.Valid() ||
			!emitter.m_Emitter->IsEmitterAlive())
			continue;
		PopcornFX::CParticleEffectInstance	*effectInstance = emitter.m_Emitter->_GetEffectInstance();
		if (!PK_VERIFY(effectInstance != null))
			continue;
		if (effectInstance->ParentEffect() == effect)
			++instanceCount;
	}

	return instanceCount;
}

//----------------------------------------------------------------------------

void	CParticleScene::StartUpdate(float dt, enum ELevelTick tickType)
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::StartUpdate", POPCORNFX_UE_PROFILER_COLOR);

	PK_ASSERT(FPopcornFXPlugin::IsMainThread());

	{
		bool		enableLocalizedPages = false;
		bool		enableByDefault = false;
		switch (m_SceneComponent->ResolvedSimulationSettings().LocalizedPagesMode)
		{
		case EPopcornFXLocalizedPagesMode::Disable:
			break;
		case EPopcornFXLocalizedPagesMode::EnableDefaultsToOn:
			enableByDefault = true; // no break !
		case EPopcornFXLocalizedPagesMode::EnableDefaultsToOff:
			enableLocalizedPages = true;
			break;
		}
		if (enableLocalizedPages != m_ParticleMediumCollection->HasLocalizedPages() ||
			enableByDefault != m_ParticleMediumCollection->PagesAreLocalizedByDefault())
			m_ParticleMediumCollection->EnableLocalizedPages(enableLocalizedPages, enableByDefault);
	}

	_PreUpdate(dt, tickType);

	// By default, register a view at world's origin
	if (m_ParticleMediumCollection->RawViews().Empty())
	{
		const PopcornFX::CGuid	viewId = m_ParticleMediumCollection->RegisterView();
		if (PK_VERIFY(viewId.Valid()))
		{
			static const CInt2	kViewportSize = CInt2(128, 128);
			m_ParticleMediumCollection->UpdateView(0, CFloat4x4::IDENTITY, CFloat4x4::IDENTITY, kViewportSize);
		}
	}

	// start PopcornFX update
	{
		SCOPE_CYCLE_COUNTER(STAT_PopcornFX_StartUpdateTime);

		INC_DWORD_STAT_BY(STAT_PopcornFX_NewParticleCount, m_ParticleMediumCollection->Stats().m_TotalNewParticleCount.Load());
		INC_DWORD_STAT_BY(STAT_PopcornFX_DeadParticleCount, m_ParticleMediumCollection->Stats().m_TotalDeadParticleCount.Load());
		INC_DWORD_STAT_BY(STAT_PopcornFX_MediumCount, m_ParticleMediumCollection->Stats().m_TotalParticleMediums);

#if (PK_HAS_GPU != 0)
		// BEFORE m_UpdateLock LOCK
		GPU_PreUpdate();
#endif // (PK_HAS_GPU != 0)

		PK_SCOPEDLOCK(m_UpdateLock);

		m_LastUpdate = GFrameCounter;

		const UPopcornFXSceneComponent	*sceneComponent = SceneComponent();
		if (!PK_VERIFY(sceneComponent != null))
			return;

		PK_ASSERT(sceneComponent->GetWorld() != null);
		{
			m_RenderBatchManager->GameThread_PreUpdate(sceneComponent->ResolvedRenderSettings());
		}

		m_ParticleMediumCollection->Stats().Reset();
		// m_ParticleMediumCollection->EnableBounds(true); // enabled only once at startup

#if	(PK_PARTICLES_HAS_STATS != 0)
		PopcornFX::CTimer	updateTimer;
		updateTimer.Start();
#endif //	(PK_PARTICLES_HAS_STATS != 0)

		m_ParticleMediumCollection->Update(dt);

#if (PK_HAS_GPU != 0)
		GPU_PreUpdateFence();
#endif // (PK_HAS_GPU != 0)

		m_ParticleMediumCollection->UpdateFence();

#if	(PK_PARTICLES_HAS_STATS != 0)
		m_MediumCollectionUpdateTime_Sum += updateTimer.Stop();
		m_MediumCollectionParticleCount_CPU_Sum += m_ParticleMediumCollection->Stats().m_TotalParticleCount_CPU.Load();
		m_MediumCollectionParticleCount_GPU_Sum += m_ParticleMediumCollection->Stats().m_TotalParticleCount_GPU.Load();
		m_MediumCollectionInstanceCount_Sum += m_ParticleMediumCollection->EffectList().UsedCount();

		const UPopcornFXSettings	*settings = FPopcornFXPlugin::Get().Settings();
		check(settings);

		bool		storeTimingsSum = false;
		const u32	frameCount = settings->HUD_UpdateTimeFrameCount;
		if (++m_MediumCollectionUpdateTime_FrameCount >= frameCount)
		{
			storeTimingsSum = true;
			m_MediumCollectionUpdateTime_Average = m_MediumCollectionUpdateTime_Sum / frameCount;
			m_MediumCollectionParticleCount_CPU_Average = m_MediumCollectionParticleCount_CPU_Sum / frameCount;
			m_MediumCollectionParticleCount_GPU_Average = m_MediumCollectionParticleCount_GPU_Sum / frameCount;
			m_MediumCollectionInstanceCount_Average = m_MediumCollectionInstanceCount_Sum / frameCount;
			m_MediumCollectionUpdateTime_Sum = 0.0f;
			m_MediumCollectionParticleCount_CPU_Sum = 0;
			m_MediumCollectionParticleCount_GPU_Sum = 0;
			m_MediumCollectionInstanceCount_Sum = 0;
			m_MediumCollectionUpdateTime_FrameCount = 0;
		}
#endif //	(PK_PARTICLES_HAS_STATS != 0)

		_PostUpdate_Emitters(dt, tickType);

#if (PK_HAS_GPU != 0)
		GPU_PostUpdate();
#endif // (PK_HAS_GPU != 0)

		// update bounds, timings, and total particle count
		{
			PK_SCOPEDLOCK(m_ParticleMediumCollection->_ActiveMediums_Lock());
			SCOPE_CYCLE_COUNTER(STAT_PopcornFX_UpdateBoundsTime);
			s32					totalParticleCount = 0;
			bool				boundsInit = false;
			PopcornFX::CAABB	bounds = PopcornFX::CAABB::DEGENERATED;
			for (uint32 i = 0; i < m_ParticleMediumCollection->Mediums().Count(); i++)
			for (const PopcornFX::PParticleMedium &medium : m_ParticleMediumCollection->_ActiveMediums_NoLock()) // 2.11+: Browse render mediums directly
			{
				PK_ASSERT(medium != null);

				// Active medium: some can have zero particles, skip them. Also skip mediums not having any renderers
				const u32	particleCount = medium->ParticleStorage()->ActiveParticleCount();
				const bool	hasMeaningfulBounds = medium->Descriptor() != null && !medium->Descriptor()->Renderers().Empty();
				if (particleCount == 0 || !hasMeaningfulBounds)
					continue;

				const PopcornFX::CAABB	&mediumBounds = medium->CachedBounds();
				const bool				validBBox = mediumBounds.Valid() && !mediumBounds.Empty() && mediumBounds.Extent() != CFloat3::ZERO; // PK2.10: bbox.Empty() behavior will change
				const bool				validBBoxRange = PopcornFX::All(mediumBounds.Min() > -1.0e15f) && PopcornFX::All(mediumBounds.Max() < 1.0e15f);

				if (validBBox &&
					validBBoxRange) // If this is not checked and bouding boxes have extremely large values, it crashes later in UE code because the bounds have nan values
				{
					bounds.Add(mediumBounds);
					boundsInit = true;
				}
				totalParticleCount += particleCount;
			}
			PK_RELEASE_ASSERT(!boundsInit || (bounds.Valid() && !bounds.Empty() && bounds.Extent() != CFloat3::ZERO));

			// Safety net for incorrect global bounds to make sure this doesn't crash in release.
			// This will make the scene actor re-use the previous valid frame's bounds, until new bounds are valid.
			//ensureMsgf(!Primitive->Bounds.BoxExtent.ContainsNaN() && !Primitive->Bounds.Origin.ContainsNaN() && !FMath::IsNaN(Primitive->Bounds.SphereRadius) && FMath::IsFinite(Primitive->Bounds.SphereRadius),
			//	TEXT("Nans found on Bounds for Primitive %s: Origin %s, BoxExtent %s, SphereRadius %f"), *Primitive->GetName(), *Primitive->Bounds.Origin.ToString(), *Primitive->Bounds.BoxExtent.ToString(), Primitive->Bounds.SphereRadius);

			FBoxSphereBounds	_boundsCheck = ToUE(m_CachedBounds.CachedBounds() * FPopcornFXPlugin::GlobalScale());
			if (PK_VERIFY(!_boundsCheck.BoxExtent.ContainsNaN()) &&
				PK_VERIFY(!_boundsCheck.Origin.ContainsNaN()) &&
				PK_VERIFY(!FMath::IsNaN(_boundsCheck.SphereRadius)) &&
				PK_VERIFY(FMath::IsFinite(_boundsCheck.SphereRadius)))
			{
				if (boundsInit)
					m_CachedBounds.SetExactBounds(bounds);
				else
					m_CachedBounds.SetExactBounds(PopcornFX::CAABB::ZERO);
				m_CachedBounds.Update();

				m_Bounds = _boundsCheck;
			}

			FPopcornFXPlugin::IncTotalParticleCount(totalParticleCount - m_LastTotalParticleCount);
			m_LastTotalParticleCount = totalParticleCount;

			INC_DWORD_STAT_BY(STAT_PopcornFX_ParticleCount, m_ParticleMediumCollection->Stats().TotalParticleCount());
			INC_DWORD_STAT_BY(STAT_PopcornFX_ParticleCount_CPU, m_ParticleMediumCollection->Stats().m_TotalParticleCount_CPU.Load());

#if (PK_GPU_D3D11 != 0)
			if (m_EnableD3D11)
			{
				INC_DWORD_STAT_BY(STAT_PopcornFX_ParticleCount_GPU_D3D11, m_ParticleMediumCollection->Stats().m_TotalParticleCount_GPU.Load());
			}
#endif // (PK_GPU_D3D11 != 0)

#if (PK_GPU_D3D12 != 0)
			if (m_EnableD3D12)
			{
				INC_DWORD_STAT_BY(STAT_PopcornFX_ParticleCount_GPU_D3D12, m_ParticleMediumCollection->Stats().m_TotalParticleCount_GPU.Load());
			}
#endif // (PK_GPU_D3D12 != 0)
		}

		m_UpdateSubView.Setup_PostUpdate();

		m_RenderBatchManager->GameThread_EndUpdate(m_UpdateSubView, sceneComponent->GetWorld());

#if POPCORNFX_RENDER_DEBUG
		m_RenderBatchManager->GameThread_SetDebugMode(sceneComponent->HeavyDebugMode);
#endif // POPCORNFX_RENDER_DEBUG

		// Update scene timings
#if	(PK_PARTICLES_HAS_STATS != 0)
		if (FPopcornFXPlugin::Get().EffectsProfilerActive())
		{
			for (u32 iMedium = 0; iMedium < m_ParticleMediumCollection->Mediums().Count(); iMedium++)
			{
				const PopcornFX::CParticleMedium	*medium = m_ParticleMediumCollection->Mediums()[iMedium].Get();
				const PopcornFX::CMediumStats		*mediumStats = medium->Stats();
				const PopcornFX::CParticleEffect	*effect = medium->Descriptor()->ParentEffect();

				bool				newRegister = false;
				PopcornFX::CGuid	effectTimingsId = m_EffectTimings.IndexOf(effect);
				if (!effectTimingsId.Valid())
				{
					newRegister = true;
					effectTimingsId = m_EffectTimings.PushBack();
				}
				if (!PK_VERIFY(effectTimingsId.Valid()))
					break;
				SPopcornFXEffectTimings	&effectTimings = m_EffectTimings[effectTimingsId];

				if (newRegister)
				{
					effectTimings.m_Effect = effect;
					effectTimings.m_EffectPath = effect->File()->Path();
					effectTimings.m_InstanceCount = _InstanceCount(effect); // only compute that for the first medium of a given effect.
				}
				PopcornFX::SEvolveStatsReport	mediumStatsReport;
				mediumStats->ComputeGlobalStats(mediumStatsReport);

				effectTimings.m_TotalStatsReport += mediumStatsReport;
				if (PK_VERIFY(medium->ParticleStorage() != null) &&
					PK_VERIFY(medium->ParticleStorage()->Manager() != null))
				{
					if (medium->ParticleStorage()->Manager()->StorageClass() == PopcornFX::CParticleStorageManager_MainMemory::DefaultStorageClass())
						effectTimings.m_TotalParticleCount_CPU += medium->ParticleStorage()->ActiveParticleCount();
					else
						effectTimings.m_TotalParticleCount_GPU += medium->ParticleStorage()->ActiveParticleCount();
				}
			}
			if (storeTimingsSum)
			{
				m_EffectTimings_Sum = m_EffectTimings;
				m_EffectTimings.Clear();
			}
		}
#endif //	(PK_PARTICLES_HAS_STATS != 0)

	}
}

//----------------------------------------------------------------------------

void	CParticleScene::ApplyWorldOffset(const FVector &inOffset)
{
	if (!PK_VERIFY(m_ParticleMediumCollection != null))
		return;
	PopcornFX::ParticleToolbox::SSceneTransformDescriptor	desc;
	desc.m_WorldOffset = ToPk(inOffset) * FPopcornFXPlugin::GlobalScaleRcp();

	PopcornFX::ParticleToolbox::TransformDeferred(m_ParticleMediumCollection, desc);
}

//----------------------------------------------------------------------------

#if	(PK_PARTICLES_HAS_STATS != 0)
bool	operator==(const CParticleScene::SPopcornFXEffectTimings &other, const PopcornFX::CParticleEffect *effect)
{
	return effect == other.m_Effect;
}
#endif // (PK_PARTICLES_HAS_STATS != 0)

//----------------------------------------------------------------------------

void	CParticleScene::SendRenderDynamicData_Concurrent()
{
	FPopcornFXPlugin::RegisterCurrentThreadAsUserIFN(); // SendRenderDynamicData_Concurrent can be run un UE's task threads

	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::SendRenderDynamicData", POPCORNFX_UE_PROFILER_COLOR);

	{
		PK_SCOPEDLOCK(m_UpdateLock); // should not happen, but just in case
		m_RenderBatchManager->ConcurrentThread_SendRenderDynamicData();
	}
}

//----------------------------------------------------------------------------

void	CParticleScene::_PreUpdate(float dt, enum ELevelTick tickType)
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::_PreUpdate", POPCORNFX_UE_PROFILER_COLOR);

	_PreUpdate_Views();
	_PreUpdate_Collisions();
	_PreUpdate_Emitters(dt, tickType);

	if (m_FillAudioBuffers != null)
		m_FillAudioBuffers->PreUpdate();
}

//----------------------------------------------------------------------------

void	CParticleScene::_Clear()
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::_Clear", POPCORNFX_UE_PROFILER_COLOR);

	_Clear_Emitters();
}

//----------------------------------------------------------------------------
//
//
//
// Render stuff
//
//
//
//----------------------------------------------------------------------------

#if RHI_RAYTRACING
void	CParticleScene::GetDynamicRayTracingInstances(const FPopcornFXSceneProxy *sceneProxy, FRayTracingMaterialGatheringContext &context, TArray<FRayTracingInstance> &outRayTracingInstances)
{
	// Right now, uses a reference view (first view).
	// First thing called during render loop, before init views
	// With RT enabled call flow can be : GDRTI -> GDME (main - all views) -> GDME (shadows)
	if (!IsInActualRenderingThread()) // GetDynamicMeshElements is called on the game thread when resizing viewports
		return;

	FPopcornFXPlugin::RegisterRenderThreadIFN();

	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::GetDynamicRayTracingInstances", POPCORNFX_UE_PROFILER_COLOR);

	SCOPE_CYCLE_COUNTER(STAT_PopcornFX_GDRTITime);

#if (PK_HAS_GPU != 0)
	GPU_PreRender();
#endif // PK_HAS_GPU != 0

	if (!m_RenderSubView.Setup_GetDynamicRayTracingInstances(sceneProxy, context, outRayTracingInstances))
		return;
	if (!PK_VERIFY(m_RenderSubView.BBViews().Count() > 0))
		return;

	// TODO: Freeze BB matrix for debug
	m_RenderBatchManager->RenderThread_DrawCalls(m_RenderSubView);
}
#endif // RHI_RAYTRACING

//----------------------------------------------------------------------------

void	CParticleScene::GetDynamicMeshElements(
	const FPopcornFXSceneProxy *sceneProxy,
	const ::TArray<const FSceneView*>& Views,
	const FSceneViewFamily& ViewFamily,
	uint32 VisibilityMap,
	FMeshElementCollector& Collector)
{
	if (!IsInActualRenderingThread()) // GetDynamicMeshElements is called on the game thread when resizing viewports
		return;

	FPopcornFXPlugin::RegisterRenderThreadIFN();

	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::GetDynamicMeshElements", POPCORNFX_UE_PROFILER_COLOR);

	SCOPE_CYCLE_COUNTER(STAT_PopcornFX_GDMETime);

#if (PK_HAS_GPU != 0)
	GPU_PreRender();
#endif // (PK_HAS_GPU != 0)

	INC_DWORD_STAT_BY(STAT_PopcornFX_ViewCount, Views.Num());

	if (!m_RenderSubView.Setup_GetDynamicMeshElements(sceneProxy, Views, ViewFamily, VisibilityMap, Collector))
		return;

	if (!PK_VERIFY(m_RenderSubView.BBViews().Count() > 0))
		return;

#if POPCORNFX_RENDER_DEBUG
	// only for main pass
	// ! unsafe access to m_SceneComponent ?!
	if (m_SceneComponent->bRender_FreezeBillboardingMatrix)
	{
		if (!m_IsFreezedBillboardingMatrix)
		{
			m_IsFreezedBillboardingMatrix = true;
			m_FreezedBillboardingMatrix = m_RenderSubView.BBView(0).m_BillboardingMatrix;
		}
		else
		{
			m_RenderSubView.BBView(0).m_BillboardingMatrix = m_FreezedBillboardingMatrix;
		}
	}
	else if (m_IsFreezedBillboardingMatrix)
	{
		m_IsFreezedBillboardingMatrix = false;
	}
#endif

	m_RenderBatchManager->RenderThread_DrawCalls(m_RenderSubView);
}

//----------------------------------------------------------------------------

void	CParticleScene::GatherSimpleLights(const FSceneViewFamily& ViewFamily, FSimpleLightArray& OutParticleLights) const
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::GatherSimpleLights", POPCORNFX_UE_PROFILER_COLOR);
	SCOPE_CYCLE_COUNTER(STAT_PopcornFX_GatherSimpleLightsTime);
	PK_ASSERT(IsInRenderingThread());

	m_RenderBatchManager->GatherSimpleLights(ViewFamily, OutParticleLights);
}

//----------------------------------------------------------------------------

bool	CParticleScene::PostUpdate_ShouldMarkRenderStateDirty() const
{
	return m_RenderBatchManager->ShouldMarkRenderStateDirty();
}

//----------------------------------------------------------------------------

void	CParticleScene::GetUsedMaterials(TArray<UMaterialInterface*> &outMaterials, bool bGetDebugMaterials)
{
	// @FIXME: GetUsedMaterials will missmatch with the truly used material in render for a frame or two.
	// because it happens we sometime render a frame late/early.
	// (this why FPopcornFXSceneProxy::bVerifyUsedMaterials = false)

	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::GetUsedMaterials", POPCORNFX_UE_PROFILER_COLOR);
	if (m_ParticleMediumCollection == null)
		return;
	const u32	matCount = m_RenderBatchManager->LastUsedMaterialCount();
	if (matCount == 0)
		return;
	outMaterials.Reserve(outMaterials.Num() + matCount);
	for (u32 iMat = 0; iMat < matCount; ++iMat)
	{
		UMaterialInterface	*usedMat = m_RenderBatchManager->GetLastUsedMaterial(iMat);
		// Material could have been deleted between the collect and GetUsedMaterials
		// Found ~reproducable crash when reimporting while UPopcornFXEffect editor and
		// one the material of the effect is open (with renderer mesh)
		if (usedMat == null)
			continue;
		outMaterials.Push(usedMat);
	}
}

//----------------------------------------------------------------------------
//
//
//
// Internal emitters management
//
//
//
//----------------------------------------------------------------------------

// static
CParticleScene::SEmitterRegister	CParticleScene::SEmitterRegister::Invalid = CParticleScene::SEmitterRegister();

//----------------------------------------------------------------------------

bool	CParticleScene::Emitter_PreInitRegister(class UPopcornFXEmitterComponent *emitter)
{
	PK_ASSERT(!m_PreInitEmitters.Contains(emitter));
	PopcornFX::CGuid	emitterId = m_PreInitEmitters.Insert(SEmitterRegister(emitter));
	PK_ASSERT(emitterId.Valid());
	if (emitterId.Valid())
		emitter->Scene_OnPreInitRegistered(this, emitterId);
	return emitterId.Valid();
}

//----------------------------------------------------------------------------

bool	CParticleScene::Emitter_PreInitUnregister(class UPopcornFXEmitterComponent *emitter)
{
	bool					ok = true;
	if (!m_PreInitEmitters.Empty())
	{
		const PopcornFX::CGuid	emitterId = emitter->Scene_PreInitEmitterId();
		ok = false;
		PK_ASSERT(emitterId.Valid());
		if (emitterId.Valid())
		{
			PK_ASSERT(emitterId < m_PreInitEmitters.Count());
			PK_ASSERT(emitter->IsPendingKillOrUnreachable() || m_PreInitEmitters.IndexOf(emitter) == emitterId);
			PK_ASSERT(m_PreInitEmitters[emitterId] == emitter);
			if (emitterId < m_PreInitEmitters.Count() && m_PreInitEmitters[emitterId] == emitter)
			{
				m_PreInitEmitters.Remove(emitterId);
				ok = true;
			}
		}
	}
	emitter->Scene_OnPreInitUnregistered(this);
	return ok;
}

//----------------------------------------------------------------------------

bool	CParticleScene::Emitter_IsRegistered(UPopcornFXEmitterComponent *emitter)
{
	PK_SCOPEDLOCK(m_EmittersLock);
	return m_Emitters.Contains(emitter);
}

//----------------------------------------------------------------------------

bool	CParticleScene::Emitter_Register(UPopcornFXEmitterComponent *emitter)
{
	PK_SCOPEDLOCK(m_EmittersLock);
	PK_ASSERT(!m_Emitters.Contains(emitter));
	PopcornFX::CGuid	emitterId = m_Emitters.Insert(SEmitterRegister(emitter));
	PK_ASSERT(emitterId.Valid());
	if (emitterId.Valid())
		emitter->Scene_OnRegistered(this, emitterId);
	return emitterId.Valid();
}

//----------------------------------------------------------------------------

bool	CParticleScene::Emitter_Unregister(UPopcornFXEmitterComponent *emitter)
{
	const PopcornFX::CGuid	emitterId = emitter->Scene_EmitterId();
	bool					ok = false;
	PK_ASSERT(emitterId.Valid());
	if (emitterId.Valid())
	{
		PK_SCOPEDLOCK(m_EmittersLock);
		if (PK_VERIFY(emitterId < m_Emitters.Count()) &&
			PK_VERIFY(m_Emitters[emitterId] == emitter))
		{
			m_Emitters.Remove(emitterId);
			ok = true;
		}
	}
	// always unregister !
	emitter->Scene_OnUnregistered(this);
	return ok;
}

//----------------------------------------------------------------------------

bool	CParticleScene::Effect_Install(PopcornFX::PCParticleEffect &effect)
{
	if (!PK_VERIFY(effect != null) ||
		!PK_VERIFY(m_ParticleMediumCollection != null))
		return false;
	return effect->Install(m_ParticleMediumCollection);
}

//----------------------------------------------------------------------------

void	CParticleScene::_PreUpdate_Emitters(float dt, enum ELevelTick tickType)
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::_PreUpdate_Emitters", POPCORNFX_UE_PROFILER_COLOR);

	SCOPE_CYCLE_COUNTER(STAT_PopcornFX_UpdateEmittersTime);

	for (uint32 iPreInitEmitter = 0; iPreInitEmitter < m_PreInitEmitters.Count(); ++iPreInitEmitter)
	{
		SEmitterRegister	&emitter = m_PreInitEmitters[iPreInitEmitter];
		if (emitter.Valid())
			emitter.m_Emitter->Scene_InitForUpdate(this);
		else if (!PK_VERIFY(!emitter.ValidLowLevel()))
		{
			UE_LOG(LogPopcornFXScene, Warning, TEXT("CParticleScene: safety unregister PreInitEmitter, it was not unregistered yet but is about to be destroyed"));
			emitter.m_Emitter.GetEvenIfUnreachable()->Scene_OnPreInitUnregistered(this);
			m_PreInitEmitters.Remove(iPreInitEmitter);
		}
	}

	if (m_Emitters.UsedCount() == 0)
		return;
	PK_SCOPEDLOCK(m_EmittersLock);
	INC_DWORD_STAT_BY(STAT_PopcornFX_EmitterUpdateCount, m_Emitters.UsedCount());
	for (uint32 emitteri = 0; emitteri < m_Emitters.Count(); ++emitteri)
	{
		SEmitterRegister		&emitter = m_Emitters[emitteri];
		if (emitter.Valid())
			emitter.m_Emitter->Scene_PreUpdate(this, dt, tickType);
	}
}

//----------------------------------------------------------------------------

void	CParticleScene::_PostUpdate_Emitters(float dt, enum ELevelTick tickType)
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::_PostUpdate_Emitters", POPCORNFX_UE_PROFILER_COLOR);

	PK_SCOPEDLOCK(m_EmittersLock);
	if (m_Emitters.UsedCount() == 0)
		return;
	INC_DWORD_STAT_BY(STAT_PopcornFX_EmitterUpdateCount, m_Emitters.UsedCount());
	for (uint32 emitteri = 0; emitteri < m_Emitters.Count(); ++emitteri)
	{
		SEmitterRegister		&emitter = m_Emitters[emitteri];
		if (emitter.Valid())
			emitter.m_Emitter->Scene_PostUpdate(this, dt, tickType);
	}
}


//----------------------------------------------------------------------------

void	CParticleScene::_Clear_Emitters()
{
	PK_SCOPEDLOCK(m_EmittersLock);
	if (m_PreInitEmitters.UsedCount() > 0)
	{
		for (uint32 iPreInitEmitter = 0; iPreInitEmitter < m_PreInitEmitters.Count(); ++iPreInitEmitter)
		{
			SEmitterRegister	&emitter = m_PreInitEmitters[iPreInitEmitter];
			if (emitter.ValidLowLevel())
				emitter.m_Emitter.GetEvenIfUnreachable()->Scene_OnPreInitUnregistered(this);
		}
		m_PreInitEmitters.Clear();
	}
	if (m_Emitters.UsedCount() > 0)
	{
		for (uint32 emitteri = 0; emitteri < m_Emitters.Count(); ++emitteri)
		{
			SEmitterRegister		&emitter = m_Emitters[emitteri];
			if (emitter.ValidLowLevel())
				emitter.m_Emitter.GetEvenIfUnreachable()->Scene_OnUnregistered(this);
		}
		m_Emitters.Clear();
	}
}

//----------------------------------------------------------------------------

namespace
{
	void	_PatchProjectionMatrix(FMatrix &projMatrix)
	{
		const float			n = GNearClippingPlane * FPopcornFXPlugin::GlobalScaleRcp();
		static const float	f = 100.0f; // Fixed near/far..
		const float			fmn = f - n;
		const float			z = (f + n) / fmn;
		const float			w = (2 * n * f) / fmn;

		projMatrix.M[2][3] = -1.0f;
		// Patch near/far
		projMatrix.M[2][2] = z;
		projMatrix.M[3][2] = w;
	}
}

//----------------------------------------------------------------------------

void	CParticleScene::_PreUpdate_Views()
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::_PreUpdate_Views", POPCORNFX_UE_PROFILER_COLOR);

	// This method is auto detect views for each local player (or editor viewport)

	PK_ASSERT(GEngine != null);
	const USceneComponent	*sceneComponent = SceneComponent();
	if (!PK_VERIFY(sceneComponent != null))
		return;
	PK_ASSERT(sceneComponent->GetWorld() != null);

	const float		scaleUEToPk = FPopcornFXPlugin::GlobalScaleRcp();
	UWorld			*world = sceneComponent->GetWorld();

	m_ParticleMediumCollection->ClearAllViews();

	const EWorldType::Type	worldType = world->WorldType;
	if (worldType == EWorldType::Inactive ||
		worldType == EWorldType::None)
		return;

	if (worldType == EWorldType::Game ||
		worldType == EWorldType::PIE)
	{
		CFloat4x4	toZUp;
		PopcornFX::CCoordinateFrame::BuildTransitionFrame(PopcornFX::ECoordinateFrame::Frame_LeftHand_Y_Up, PopcornFX::ECoordinateFrame::Frame_LeftHand_Z_Up, toZUp);

		auto	playerIt = GEngine->GetLocalPlayerIterator(world);
		while (playerIt)
		{
			const ULocalPlayer		*localPlayer = *playerIt;
			if (localPlayer != null && localPlayer->ViewportClient != null)
			{
				const PopcornFX::CGuid		playerViewId = m_ParticleMediumCollection->RegisterView();
				if (!PK_VERIFY(playerViewId.Valid()))
					return;

				FViewport	*viewport = localPlayer->ViewportClient->Viewport;
				if (!PK_VERIFY(viewport != null))
					return;
				const CInt2	viewportSize = ToPk(viewport->GetSizeXY());
				if (viewportSize.x() > 0 && viewportSize.y() > 0) // Ignore this viewport (Happens when PIE with Number of Players set to 2)
				{
					PK_NAMEDSCOPEDPROFILE_C("CParticleScene::_PreUpdate_Views BuildProjectionMatrix", POPCORNFX_UE_PROFILER_COLOR);

					FSceneViewProjectionData	projectionData;
					if (localPlayer->GetProjectionData(viewport, eSSP_FULL, projectionData)) // Returns false if viewport or player is invalid
					{
						const CFloat4x4	worldToView = ToPk(FTranslationMatrix(-projectionData.ViewOrigin * scaleUEToPk) * projectionData.ViewRotationMatrix);

						_PatchProjectionMatrix(projectionData.ProjectionMatrix);

						m_ParticleMediumCollection->UpdateView(playerViewId,
							worldToView * toZUp,
							worldToView * ToPk(projectionData.ProjectionMatrix),
							viewportSize);
					}
				}
			}
			++playerIt;
		}
	}
#if WITH_EDITOR
	else if (worldType == EWorldType::EditorPreview ||
			 worldType == EWorldType::GamePreview ||
			 worldType == EWorldType::Editor)
	{
		FViewport				*viewport = null;
		FEditorViewportClient	*viewportClient = null;

		// First, check if we have a FLevelEditorViewportClient (special case)
		bool											levelEditorViewport = false;
		TArray<FEditorViewportClient*>					allClientsCopy = GEditor->GetAllViewportClients();
		const TArray<class FLevelEditorViewportClient*>	&levelViewportClients = GEditor->GetLevelViewportClients();

		const u32		levelViewportCount = levelViewportClients.Num();
		if (levelViewportCount == 0)
			return;

		CFloat4x4	toZUp;
		PopcornFX::CCoordinateFrame::BuildTransitionFrame(PopcornFX::ECoordinateFrame::Frame_LeftHand_Y_Up, PopcornFX::ECoordinateFrame::Frame_LeftHand_Z_Up, toZUp);
		for (u32 iViewport = 0; iViewport < levelViewportCount; ++iViewport)
		{
			FLevelEditorViewportClient	*client = levelViewportClients[iViewport];
			if (client == null)
				continue;
			allClientsCopy.Remove(client); // Make sure we don't re-iterate over level editor viewport clients next loop
			if (client->GetWorld() == world && client == GCurrentLevelEditingViewportClient)
			{
				viewportClient = client;
				viewport = viewportClient->Viewport;
				levelEditorViewport = true;
				break;
			}
		}

		// If not, can be any kind of viewport
		if (!levelEditorViewport)
		{
			const u32	viewportCount = allClientsCopy.Num();
			for (u32 iViewport = 0; iViewport < viewportCount; ++iViewport)
			{
				FEditorViewportClient	*client = allClientsCopy[iViewport];
				if (client == null)
					continue;
				if (client->GetWorld() == world)
				{
					viewportClient = client;
					viewport = viewportClient->Viewport;
					break;
				}
			}
		}

		// We should at least have one valid viewport
		if (!PK_VERIFY(viewport != null && viewportClient != null))
			return;

		const PopcornFX::CGuid		viewId = m_ParticleMediumCollection->RegisterView();
		if (!PK_VERIFY(viewId.Valid()))
			return;

		FSceneViewProjectionData	projectionData;
		FMinimalViewInfo			viewInfo;
		CInt2						viewportSize = ToPk(viewport->GetSizeXY());

		// Can happen when editing a blueprint and 3D viewport is not in focus (ie. when focusing the graph)
		// If we do not early out, projection matrix generated by UE4 will contain nans
		if (viewportSize.x() == 0 || viewportSize.y() == 0)
			return;

		UCameraComponent	*camera = null;
		if (levelEditorViewport)
			camera = ((FLevelEditorViewportClient*)viewportClient)->GetCameraComponentForView();
		if (PK_PREDICT_LIKELY(camera == null))
		{
			viewInfo.Location = viewportClient->GetViewLocation();
			viewInfo.Rotation = viewportClient->GetViewRotation();
			viewInfo.FOV = viewportClient->ViewFOV;
		}
		else // Viewport locked to a camera actor
		{
			FMinimalViewInfo	view;
			camera->GetCameraView(world->GetDeltaSeconds(), view);

			viewInfo.Location = camera->GetComponentTransform().GetLocation();
			viewInfo.Rotation = camera->GetComponentTransform().Rotator();
			viewInfo.FOV = view.FOV;
		}

		projectionData.ViewOrigin = viewInfo.Location;
		projectionData.SetViewRectangle(FIntRect(0, 0, viewportSize.x(), viewportSize.y()));
		projectionData.ViewRotationMatrix = FInverseRotationMatrix(viewInfo.Rotation) * FMatrix(
			FPlane(0, 0, 1, 0),
			FPlane(1, 0, 0, 0),
			FPlane(0, 1, 0, 0),
			FPlane(0, 0, 0, 1));

		const EAspectRatioAxisConstraint	aspectRatioAxisConstraint = GetDefault<ULevelEditorViewportSettings>()->AspectRatioAxisConstraint;
		FMinimalViewInfo::CalculateProjectionMatrixGivenView(viewInfo, aspectRatioAxisConstraint, viewport, projectionData);

		const CFloat4x4	worldToView = ToPk(FTranslationMatrix(-projectionData.ViewOrigin * scaleUEToPk) * projectionData.ViewRotationMatrix);

		_PatchProjectionMatrix(projectionData.ProjectionMatrix);

		m_ParticleMediumCollection->UpdateView(0,
			worldToView * toZUp,
			worldToView * ToPk(projectionData.ProjectionMatrix),
			viewportSize);
	}
#endif // WITH_EDITOR
	else
	{
		PK_ASSERT_NOT_REACHED();
	}
}

//----------------------------------------------------------------------------
//
//
//
// Collisions
//
//
//
//----------------------------------------------------------------------------

DECLARE_CYCLE_STAT_EXTERN(TEXT("RayTracePacket"), STAT_PopcornFX_RayTracePacketTime, STATGROUP_PopcornFX, );
DEFINE_STAT(STAT_PopcornFX_RayTracePacketTime);

//----------------------------------------------------------------------------
#if PK_WITH_PHYSX
//----------------------------------------------------------------------------

namespace
{
	using namespace physx;

	PxQueryHitType::Enum	_PopcornFXRaycastPreFilter_Simple(PxFilterData filterData, PxFilterData shapeData)
	{
		const ECollisionChannel		shapeChannel = GetCollisionChannel(shapeData.word3);
		const uint32				shapeChannelBit = ECC_TO_BITFIELD(shapeChannel);
		const uint32				filterChannelMask = filterData.word1;

		// Cull overlap requests immediately
		//if (shapeChannel == ECollisionChannel::ECC_OverlapAll_Deprecated)
		//	return PxQueryHitType::eNONE;

		if (!(filterChannelMask & shapeChannelBit))
			return PxQueryHitType::eNONE;

		const uint32		shapeFlags = shapeData.word3 & 0xFFFFFF;
		const uint32		filterFlags = filterData.word3 & 0xFFFFFF;
		const uint32		commonFlags = shapeFlags & filterFlags;

		if (!(commonFlags & (EPDF_SimpleCollision | EPDF_ComplexCollision)))
			return PxQueryHitType::eNONE;

		// ObjectQuery only
		// we should not need to check other flags
		// (see FPxQueryFilterCallback::CalcQueryHitType)

		return PxQueryHitType::eBLOCK;
	}

	PxQueryHitType::Enum		_PopcornFXRaycastPreFilter(PxFilterData filterData, PxFilterData shapeData, const void *constantBlock, PxU32 constantBlockSize, PxHitFlags &hitFlags)
	{
		return _PopcornFXRaycastPreFilter_Simple(filterData, shapeData);
	}

	// see Runtime/Engine/Private/Collision/PhysXCollision.h 120
	// see Runtime/Engine/Private/Collision/PhysXCollision.cpp 609
	class FPopcornFXPxQueryFilterCallback : public PxSceneQueryFilterCallback
	{
	public:
		FPopcornFXPxQueryFilterCallback() {}
		virtual PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxSceneQueryFlags& queryFlags) override
		{
			if (!shape) // can happen since 4.13
			{
				//UE_LOG(LogCollision, Warning, TEXT("Invalid shape encountered in FPxQueryFilterCallback::preFilter, actor: %p, filterData: %x %x %x %x"), actor, filterData.word0, filterData.word1, filterData.word2, filterData.word3);
				return PxQueryHitType::eNONE;
			}
			const PxFilterData		shapeData = shape->getQueryFilterData();
			return _PopcornFXRaycastPreFilter_Simple(filterData, shapeData);
		}
		virtual PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxSceneQueryHit& hit) override
		{
			// Currently not used
			return PxQueryHitType::eBLOCK;
		}
	};

	UPrimitiveComponent			*_PhysXExtractHitComponent(const PxLocationHit &hit)
	{
		// see Runtime/Engine/Private/Collision/CollisionConversions.cpp
		// SetHitResultFromShapeAndFaceIndex

		const FBodyInstance	*bodyInstance = FPhysxUserData::Get<FBodyInstance>(hit.actor->userData);
		if (bodyInstance != null)
		{
			bodyInstance = FPhysicsInterface_PhysX::ShapeToOriginalBodyInstance(bodyInstance, hit.shape);
			PK_ASSERT(bodyInstance != null);
			return bodyInstance->OwnerComponent.Get();
		}
		const FCustomPhysXPayload	*customPayload = FPhysxUserData::Get<FCustomPhysXPayload>(hit.shape->userData);
		if (customPayload != null)
			return customPayload->GetOwningComponent().Get();
		// Invalid user datas
		return null;
	}

	UPhysicalMaterial			*_PhysXExtractPhysicalMaterial(const PxLocationHit &hit)
	{
		UPhysicalMaterial	*umat = null;
		// Query the physical material
		const PxMaterial	*pMat = hit.shape->getMaterialFromInternalFaceIndex(hit.faceIndex);
		if (pMat != null)
			umat = FPhysxUserData::Get<UPhysicalMaterial>(pMat->userData);
		return umat;
	}


} // namespace

#endif // PK_WITH_PHYSX
//----------------------------------------------------------------------------

//#define RAYTRACE_PROFILE

#ifdef	RAYTRACE_PROFILE
#	define RAYTRACE_PROFILE_DECLARE				PK_PROFILESTATS_DECLARE
#	define RAYTRACE_PROFILE_CAPTURE				PK_PROFILESTATS_CAPTURE
#	define RAYTRACE_PROFILE_CAPTURE_N			PK_PROFILESTATS_CAPTURE_N
#	define RAYTRACE_PROFILE_CAPTURE_CYCLES		PK_PROFILESTATS_CAPTURE_CYCLES
#	define RAYTRACE_PROFILE_CAPTURE_CYCLES_N	PK_PROFILESTATS_CAPTURE_CYCLES_N
#else
#	define RAYTRACE_PROFILE_DECLARE(...)
#	define RAYTRACE_PROFILE_CAPTURE(...)
#	define RAYTRACE_PROFILE_CAPTURE_N(...)
#	define RAYTRACE_PROFILE_CAPTURE_CYCLES(...)
#	define RAYTRACE_PROFILE_CAPTURE_CYCLES_N(...)
#endif

#if PK_WITH_CHAOS
class FBlockQueryCallbackChaos : public ICollisionQueryFilterCallbackBase
{
public:
	virtual ~FBlockQueryCallbackChaos() {}
	virtual ECollisionQueryHitType PostFilter(const FCollisionFilterData &filterData, const ChaosInterface::FQueryHit &hit) override { return ECollisionQueryHitType::Block; }
	virtual ECollisionQueryHitType PreFilter(const FCollisionFilterData &filterData, const Chaos::FPerShapeData &shape, const Chaos::TGeometryParticle<float, 3> &actor) override
	{
		const FCollisionFilterData	&shapeData = shape.GetQueryData();
		const ECollisionChannel		shapeChannel = GetCollisionChannel(shapeData.Word3);
		const uint32				shapeChannelBit = ECC_TO_BITFIELD(shapeChannel);
		const uint32				filterChannelMask = filterData.Word1;

		if (!(filterChannelMask & shapeChannelBit))
			return ECollisionQueryHitType::None;

		const uint32		shapeFlags = shapeData.Word3 & 0xFFFFFF;
		const uint32		filterFlags = filterData.Word3 & 0xFFFFFF;
		const uint32		commonFlags = shapeFlags & filterFlags;

		if (!(commonFlags & (EPDF_SimpleCollision | EPDF_ComplexCollision)))
			return ECollisionQueryHitType::None;

		return ECollisionQueryHitType::Block;
	}

#if PHYSICS_INTERFACE_PHYSX
	virtual ECollisionQueryHitType	PostFilter(const FCollisionFilterData& FilterData, const physx::PxQueryHit& Hit) { PK_ASSERT_NOT_REACHED(); return ECollisionQueryHitType::None; }
	virtual ECollisionQueryHitType	PreFilter(const FCollisionFilterData& FilterData, const physx::PxShape& Shape, physx::PxRigidActor& Actor) { PK_ASSERT_NOT_REACHED(); return ECollisionQueryHitType::None; }
	virtual PxQueryHitType::Enum	preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) { PK_ASSERT_NOT_REACHED(); return PxQueryHitType::eNONE; }
	virtual PxQueryHitType::Enum	postFilter(const PxFilterData& filterData, const PxQueryHit& hit) { PK_ASSERT_NOT_REACHED(); return PxQueryHitType::eNONE; }
#endif
};
#endif

void	CParticleScene::RayTracePacket(
	const PopcornFX::Colliders::STraceFilter &traceFilter,
	const PopcornFX::Colliders::SRayPacket &packet,
	const PopcornFX::Colliders::STracePacket &results)
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::RayTracePacket", POPCORNFX_UE_PROFILER_COLOR);

	RAYTRACE_PROFILE_DECLARE(
		//10000,
		50000,
		RayTrace,
		RayTrace_PX_Scene_Lock,
		RayTrace_PX_CreateBatch,
		RayTrace_PX_BuildQuery,
		RayTrace_PX_Exec,
		RayTrace_Results_Hit,
		RayTrace_Results_Hit_Comp,
		RayTrace_Results_Hit_Mat
		);

	const u32		resCount = results.Count();
	const bool		emptySphereSweeps = packet.m_RaySweepRadii_Aligned16.Empty();
	const bool		emptyMasks = packet.m_RayMasks_Aligned16.Empty();

	RAYTRACE_PROFILE_CAPTURE_CYCLES_N(RayTrace, resCount);

	SCOPE_CYCLE_COUNTER(STAT_PopcornFX_RayTracePacketTime);

	PK_ASSERT(packet.m_RayDirectionsAndLengths_Aligned16.Count() == resCount);
	PK_ASSERT(packet.m_RayOrigins_Aligned16.Count() == resCount);
	PK_ASSERT(packet.m_RaySweepRadii_Aligned16.Count() == resCount || emptySphereSweeps);
	PK_ASSERT(packet.m_RayMasks_Aligned16.Count() == resCount || emptyMasks);
	PK_ASSERT(results.m_HitTimes_Aligned16 != null);
	PK_ASSERT(results.m_ContactNormals_Aligned16 != null);

	PK_ASSERT_MESSAGE(results.m_ContactPoints_Aligned16 == null, "Not implemented");

	const bool		queryPhysicalMaterial = m_SceneComponent->ResolvedSimulationSettings().bEnablePhysicalMaterials;
	const float		scalePkToUE = FPopcornFXPlugin::GlobalScale();
	const float		scaleUEToPk = FPopcornFXPlugin::GlobalScaleRcp();

	// m_FilterFlags is a bitfield, matching UE4's object channels
	s32		objectTypesToQuery = traceFilter.m_FilterFlags;
	if (objectTypesToQuery == 0) // "None" : no collision preset is specified
	{
		// Consider the default to be tracing "WorldStatic" & "WorldDynamic" object channels.
		objectTypesToQuery = ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic);
	}

	// False by default for now, we should extract this from traceFilter.m_FilterFlags
	const bool			traceComplexGeometry = true;

#if PK_WITH_PHYSX

	using namespace physx;

//#	define IF_ASSERTS_PHYSX(...)	PK_ONLY_IF_ASSERTS(__VA_ARGS__)
#	define IF_ASSERTS_PHYSX(...)

	PxScene			* const physxScene = m_CurrentPhysxScene;
	if (!PK_VERIFY(physxScene != null))
		return;
	PxHitFlags					outFlags = IF_ASSERTS_PHYSX(PxHitFlag::ePOSITION | ) PxHitFlag::eDISTANCE | PxHitFlag::eNORMAL;

	PxFilterData				filterData;
	filterData.word0 = 0; // ECollisionQuery::ObjectQuery;
	filterData.word1 = objectTypesToQuery;
	filterData.word3 = EPDF_SimpleCollision;
	if (traceComplexGeometry)
		filterData.word3 |= EPDF_ComplexCollision;

#	define PxSceneQueryFilterFlag		PxQueryFlag
	PxSceneQueryFilterData				queryFilterData(filterData, PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::ePREFILTER);
	FPopcornFXPxQueryFilterCallback		queryCallback; // PreFilter

	u32			hitResultBufferCurrentIndex = 0;
	struct FRaycastBufferAndIndex
	{
		u32					m_RayIndex;
		PxRaycastBuffer		m_RayBuffer;
		PxSweepBuffer		m_SweepBuffer;
	};
	PK_STACKMEMORYVIEW(FRaycastBufferAndIndex, hitResultBuffers, resCount);

	{
		RAYTRACE_PROFILE_CAPTURE_CYCLES_N(RayTrace_PX_Scene_Lock, resCount);
		// !! LOCK physics scene
		physxScene->lockRead();
	}
	{
		for (u32 rayi = 0; rayi < resCount; ++rayi)
		{
			if (!emptyMasks && packet.m_RayMasks_Aligned16[rayi] == 0)
				continue;

			CFloat3		start;
			CFloat3		rayDir;
			float		rayLen;
			{
				RAYTRACE_PROFILE_CAPTURE_CYCLES(RayTrace_PX_BuildQuery);
				const CFloat4		&_rayDirAndLen = packet.m_RayDirectionsAndLengths_Aligned16[rayi];
				// PhysX collides even with rayLen == 0
				// PopcornFX input rayLen == 0 means ignore it
				if (_rayDirAndLen.w() <= 0)
					continue;
				start = packet.m_RayOrigins_Aligned16[rayi].xyz() * scalePkToUE;
				rayDir = _rayDirAndLen.xyz();
				rayLen = _rayDirAndLen.w() * scalePkToUE;
			}

			FRaycastBufferAndIndex		&resultBufferAndIndex = hitResultBuffers[hitResultBufferCurrentIndex];
			{
				RAYTRACE_PROFILE_CAPTURE_CYCLES(RayTrace_PX_Exec);
				PxLocationHit	resultBlock;
				bool			hasBlock;
				if (emptySphereSweeps || packet.m_RaySweepRadii_Aligned16[rayi] == 0.0f)
				{
					PxRaycastBuffer		&outResultBuffer = resultBufferAndIndex.m_RayBuffer;

					physxScene->raycast(_Reinterpret<PxVec3>(start), _Reinterpret<PxVec3>(rayDir), rayLen, outResultBuffer, outFlags, queryFilterData, &queryCallback);

					resultBlock = outResultBuffer.block;
					hasBlock = outResultBuffer.hasBlock;
				}
				else
				{
					PxSweepBuffer		&outResultBuffer = resultBufferAndIndex.m_SweepBuffer;
					PxSphereGeometry	sphere(packet.m_RaySweepRadii_Aligned16[rayi] * scalePkToUE);
					PxTransform			sphereStart(_Reinterpret<PxVec3>(start));

					physxScene->sweep(sphere, sphereStart, _Reinterpret<PxVec3>(rayDir), rayLen, outResultBuffer, outFlags, queryFilterData, &queryCallback);

					resultBlock = outResultBuffer.block;
					hasBlock = outResultBuffer.hasBlock;
				}

				// negative rays do happen, maybe because of physx float precision issues ?
				const float			kMinLenEpsilon = -1e-03f; // 10 micrometer still acceptable ?!
				PK_ASSERT(resultBlock.distance >= kMinLenEpsilon);
				resultBlock.distance = PopcornFX::PKMax(resultBlock.distance, 0.f);

				if (PK_PREDICT_LIKELY(!hasBlock))
					continue;
			}
			resultBufferAndIndex.m_RayIndex = rayi;
			++hitResultBufferCurrentIndex;
		}
	}

	void			** contactObjects = results.m_ContactObjects_Aligned16;
	void			** contactSurfaces = queryPhysicalMaterial ? results.m_ContactSurfaces_Aligned16 : null;

	const u32		hitResultCount = hitResultBufferCurrentIndex;
	for (u32 hiti = 0; hiti < hitResultCount; ++hiti)
	{
		RAYTRACE_PROFILE_CAPTURE_CYCLES(RayTrace_Results_Hit);

		const FRaycastBufferAndIndex	&resultBufferAndIndex = hitResultBuffers[hiti];
		const u32						rayi = resultBufferAndIndex.m_RayIndex;
		PK_ASSERT(rayi < resCount);

		const bool						isSweep = !packet.m_RaySweepRadii_Aligned16.Empty() && packet.m_RaySweepRadii_Aligned16[rayi] != 0.0f;
		PxLocationHit					hit;
		if (isSweep)
		{
			hit = resultBufferAndIndex.m_SweepBuffer.block;
			PK_ASSERT(resultBufferAndIndex.m_SweepBuffer.hasBlock);
		}
		else
		{
			hit = resultBufferAndIndex.m_RayBuffer.block;
			PK_ASSERT(resultBufferAndIndex.m_RayBuffer.hasBlock);
		}

		const CFloat4		&_rayDirAndLen = packet.m_RayDirectionsAndLengths_Aligned16[rayi];
		const float			rayLen = _rayDirAndLen.w();
		const CFloat3		rayDir = _rayDirAndLen.xyz();

		const float			hitTime = hit.distance * scaleUEToPk;
		// PhysX sometimes collides farther than asked !!
		IF_ASSERTS_PHYSX(PK_ASSERT(hitTime < rayLen);)
		// hitTime = PopcornFX::PKMin(hitTime, rayLen);

		results.m_HitTimes_Aligned16[rayi] = hitTime;

		IF_ASSERTS_PHYSX(
			const CFloat3	hitPos = _Reinterpret<CFloat3>(hit.position) * scaleUEToPk;
			const float		computedTime = (hitPos - packet.m_RayOrigins_Aligned16[rayi].xyz()).Length();
			PK_ASSERT(PopcornFX::PKAbs(computedTime - hitTime) < 1.0e-3f);
		)

		// Patch PhysX invalid normal !!!!
		CFloat3			normal = _Reinterpret<CFloat3>(hit.normal);
		IF_ASSERTS_PHYSX(PK_ASSERT(normal.IsNormalized(10e-3f)));
		{
			static const float		kThreshold = 1.0e-3f * 1.0e-3f;
			const float				lenSq = normal.LengthSquared();
			if (PopcornFX::PKAbs(lenSq - 1.0f) >= kThreshold)
			{
				if (lenSq <= kThreshold || !PopcornFX::TNumericTraits<float>::IsFinite(lenSq))
					normal = - rayDir;
				else
					normal /= PopcornFX::PKSqrt(lenSq);
			}
			PK_ASSERT(normal.IsNormalized(10e-3f));
		}
		results.m_ContactNormals_Aligned16[rayi].xyz() = normal;

		if (contactObjects != null)
		{
			RAYTRACE_PROFILE_CAPTURE_CYCLES_N(RayTrace_Results_Hit_Comp, 1);
			contactObjects[rayi] = _PhysXExtractHitComponent(hit);
		}
		if (contactSurfaces != null)
		{
			RAYTRACE_PROFILE_CAPTURE_CYCLES_N(RayTrace_Results_Hit_Mat, 1);
			contactSurfaces[rayi] = _PhysXExtractPhysicalMaterial(hit);
		}
	}

	// !! UNLOCK physics scene
	physxScene->unlockRead();

#elif PK_WITH_CHAOS

	u32			hitResultBufferCurrentIndex = 0;
	struct FRaycastBufferAndIndex
	{
		FHitLocation	m_HitLocation;
		u32				m_RayIndex;
	};
	PK_STACKMEMORYVIEW(FRaycastBufferAndIndex, hitResultBuffers, resCount);

	EHitFlags						outFlags = PK_ONLY_IF_ASSERTS(EHitFlags::Position | ) EHitFlags::Distance | EHitFlags::Normal;
	FBlockQueryCallbackChaos		callBack;
	FCollisionFilterData			filterData = FCollisionFilterData();
	filterData.Word0 = 0; // ECollisionQuery::ObjectQuery;
	filterData.Word1 = objectTypesToQuery;
	filterData.Word3 = EPDF_SimpleCollision;
	if (traceComplexGeometry)
		filterData.Word3 |= EPDF_ComplexCollision;

	EQueryFlags						queryFlags = EQueryFlags::AnyHit | EQueryFlags::PreFilter;
	FQueryFilterData				queryFilterData = ChaosInterface::MakeQueryFilterData(filterData, queryFlags, FCollisionQueryParams());
	FQueryDebugParams				debugParams;
	FCollisionShape					sphere;

	// Note: There doesn't seem to be a lock necessary for the Chaos accel struct
	{
		if (const auto &solverAccelerationStructure = m_CurrentChaosScene->GetSpacialAcceleration())
		{
			FChaosSQAccelerator sqAccelerator(*solverAccelerationStructure);
			{
				for (u32 rayi = 0; rayi < resCount; ++rayi)
				{
					if (!emptyMasks && packet.m_RayMasks_Aligned16[rayi] == 0)
						continue;

					CFloat3		start;
					CFloat3		rayDir;
					float		rayLen;
					{
						RAYTRACE_PROFILE_CAPTURE_CYCLES(RayTrace_CHAOS_BuildQuery);
						const CFloat4	&_rayDirAndLen = packet.m_RayDirectionsAndLengths_Aligned16[rayi];

						if (_rayDirAndLen.w() <= 0)
							continue;
						start = packet.m_RayOrigins_Aligned16[rayi].xyz() * scalePkToUE;
						rayDir = _rayDirAndLen.xyz();
						rayLen = _rayDirAndLen.w() * scalePkToUE;
					}

					bool					hasHit = false;
					FRaycastBufferAndIndex	&resultBufferAndIndex = hitResultBuffers[hitResultBufferCurrentIndex];
					{
						RAYTRACE_PROFILE_CAPTURE_CYCLES(RayTrace_CHAOS_Exec);

						if (emptySphereSweeps || packet.m_RaySweepRadii_Aligned16[rayi] == 0.0f)
						{
							FSingleHitBuffer<FHitRaycast>	hitBuffer;
							sqAccelerator.Raycast(_Reinterpret<FVector>(start), _Reinterpret<FVector>(rayDir), rayLen, hitBuffer, outFlags, queryFilterData, callBack, debugParams);
							if (hitBuffer.HasBlockingHit())
							{
								resultBufferAndIndex.m_HitLocation = *hitBuffer.GetBlock();
								hasHit = resultBufferAndIndex.m_HitLocation.Distance > 0.0f;
							}
						}
						else
						{
							const FTransform StartTM = FTransform(_Reinterpret<FVector>(start));

							FSingleHitBuffer<FHitSweep>		hitBuffer;
							sqAccelerator.Sweep(Chaos::TSphere<float, 3>(Chaos::FVec3::ZeroVector, packet.m_RaySweepRadii_Aligned16[rayi] * scalePkToUE), StartTM, _Reinterpret<FVector>(rayDir), rayLen, hitBuffer, outFlags, queryFilterData, callBack, debugParams);
							if (hitBuffer.HasBlockingHit())
							{
								resultBufferAndIndex.m_HitLocation = *hitBuffer.GetBlock();
								hasHit = resultBufferAndIndex.m_HitLocation.Distance > 0.0f;
							}
						}

						if (PK_PREDICT_LIKELY(!hasHit))
							continue;
					}
					resultBufferAndIndex.m_RayIndex = rayi;
					++hitResultBufferCurrentIndex;
				}
			}
		}
	}

	for (u32 hiti = 0; hiti < hitResultBufferCurrentIndex; ++hiti)
	{
		RAYTRACE_PROFILE_CAPTURE_CYCLES(RayTrace_CHAOS_Results_Hit);

		u32					rayi = hitResultBuffers[hiti].m_RayIndex;
		const FHitLocation	&hit = hitResultBuffers[hiti].m_HitLocation;

		const float		hitTime = hit.Distance * scaleUEToPk;

		results.m_HitTimes_Aligned16[rayi] = hitTime;

		PK_ONLY_IF_ASSERTS(
			const CFloat3	hitPos = _Reinterpret<CFloat3>(hit.WorldPosition) * scaleUEToPk;
			const float		computedTime = (hitPos - packet.m_RayOrigins_Aligned16[rayi].xyz()).Length();
			if (emptySphereSweeps)
				PK_ASSERT(PopcornFX::PKAbs(computedTime - hitTime) < 1.0e-3f);
			else
			{
				const float		radius = packet.m_RaySweepRadii_Aligned16[rayi];
				PK_ASSERT(PopcornFX::PKAbs(computedTime - hitTime) < radius);
			}
			)
		CFloat3			normal = _Reinterpret<CFloat3>(hit.WorldNormal);
		results.m_ContactNormals_Aligned16[rayi].xyz() = normal;
	}
#endif
}

//----------------------------------------------------------------------------

void	CParticleScene::RayTracePacketTemporal(
	const PopcornFX::Colliders::STraceFilter &traceFilter,
	const PopcornFX::Colliders::SRayPacket &packet,
	const PopcornFX::Colliders::STracePacket &results)
{
	RayTracePacket(traceFilter, packet, results);
}

//----------------------------------------------------------------------------

void	CParticleScene::_PreUpdate_Collisions()
{
#if PK_WITH_PHYSX
	m_CurrentPhysxScene = null;
	if (SceneComponent() != null)
	{
		const UWorld	*world = m_SceneComponent->GetWorld();
		if (PK_VERIFY(world != null))
		{
			FPhysScene		*worldPhysScene = world->GetPhysicsScene();
			if (worldPhysScene != null)
				m_CurrentPhysxScene = worldPhysScene->GetPxScene();
		}
	}
#elif PK_WITH_CHAOS
	 m_CurrentChaosScene = null;
	if (SceneComponent() != null)
	{
		const UWorld	*world = m_SceneComponent->GetWorld();
		if (PK_VERIFY(world != null))
		{
			m_CurrentChaosScene = world->GetPhysicsScene();
		}
	}
#endif
}

//----------------------------------------------------------------------------

#if PK_WITH_PHYSX
namespace
{
	using namespace physx;

	PK_FORCEINLINE PopcornFX::CFloat3		ToPk(const PxVec3 &v)
	{
		return _Reinterpret<PopcornFX::CFloat3>(v);
	}
}
#endif

//----------------------------------------------------------------------------

void	CParticleScene::SetAudioInterface(IPopcornFXAudio *audioInterface)
{
	m_AudioInterface = audioInterface;
}

//----------------------------------------------------------------------------

TMemoryView<const float * const>	CParticleScene::GetAudioSpectrum(PopcornFX::CStringId channelGroup, u32 &outBaseCount) const
{
	if (channelGroup.Empty() || m_FillAudioBuffers == null)
		return TMemoryView<const float * const>();

	const FName			channelName(ANSI_TO_TCHAR(channelGroup.ToString().Data()));
	const float * const	*rawSpectrumData = m_FillAudioBuffers->AsyncGetAudioSpectrum(channelName, outBaseCount);
	if (rawSpectrumData == null || outBaseCount == 0)
		return TMemoryView<const float * const>();

	const u32		pyramidSize = PopcornFX::IntegerTools::Log2(outBaseCount) + 1;
	return TMemoryView<const float * const>(rawSpectrumData, pyramidSize);
}

//----------------------------------------------------------------------------

TMemoryView<const float * const>	CParticleScene::GetAudioWaveform(PopcornFX::CStringId channelGroup, u32 &outBaseCount) const
{
	if (channelGroup.Empty() || m_FillAudioBuffers == null)
		return TMemoryView<const float * const>();

	const FName			channelName(ANSI_TO_TCHAR(channelGroup.ToString().Data()));
	const float * const	*rawWaveformData = m_FillAudioBuffers->AsyncGetAudioWaveform(channelName, outBaseCount);
	if (rawWaveformData == null || outBaseCount == 0)
		return TMemoryView<const float * const>();

	const u32		pyramidSize = PopcornFX::IntegerTools::Log2(outBaseCount) + 1;
	return TMemoryView<const float * const>(rawWaveformData, pyramidSize);
}

//----------------------------------------------------------------------------
//
//
//
// GPU stuff
//
//
//
//----------------------------------------------------------------------------

#if (PK_HAS_GPU == 1)

DECLARE_CYCLE_STAT_EXTERN(TEXT("GPU PreUpdate FlushRenderingCommands"), STAT_PopcornFX_GPU_PreUpdate_Flush, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("GPU PreRender Execute"), STAT_PopcornFX_GPU_PreRender_Exec, STATGROUP_PopcornFX, );

DEFINE_STAT(STAT_PopcornFX_GPU_PreUpdate_Flush);
DEFINE_STAT(STAT_PopcornFX_GPU_PreRender_Exec);

//----------------------------------------------------------------------------

bool	CParticleScene::GPU_InitIFN()
{
	bool	ok = false;
#if (PK_GPU_D3D11 == 1)
	if (m_EnableD3D11)
		ok = D3D11_InitIFN();
#endif
#if (PK_GPU_D3D12 == 1)
	if (!ok && m_EnableD3D12)
		ok = D3D12_InitIFN();
#endif
	return ok;
}

//----------------------------------------------------------------------------

void	CParticleScene::GPU_Destroy()
{
#if (PK_GPU_D3D11 == 1)
	if (m_EnableD3D11)
		D3D11_Destroy();
#endif
#if (PK_GPU_D3D12 == 1)
	if (m_EnableD3D12)
		D3D12_Destroy();
#endif
}

//----------------------------------------------------------------------------

void	CParticleScene::GPU_PreRender()
{
#if (PK_GPU_D3D11 == 1)
	if (D3D11Ready())
		D3D11_PreRender();
#endif
#if (PK_GPU_D3D12 == 1)
	if (D3D12Ready())
		D3D12_PreRender();
#endif
}

//----------------------------------------------------------------------------

void	CParticleScene::GPU_PreUpdate()
{
#if (PK_GPU_D3D11 == 1)
	if (D3D11Ready())
		D3D11_PreUpdate();
#endif
#if (PK_GPU_D3D12 == 1)
	if (D3D12Ready())
		D3D12_PreUpdate();
#endif
}

//----------------------------------------------------------------------------

void	CParticleScene::GPU_PreUpdateFence()
{
#if (PK_GPU_D3D11 == 1)
	if (D3D11Ready())
		D3D11_PreUpdateFence();
#endif
#if (PK_GPU_D3D12 == 1)
	if (D3D12Ready())
		D3D12_PreUpdateFence();
#endif
}

//----------------------------------------------------------------------------

void	CParticleScene::GPU_PostUpdate()
{
#if (PK_GPU_D3D11 == 1)
	if (D3D11Ready())
		D3D11_PostUpdate();
#endif
#if (PK_GPU_D3D12 == 1)
	if (D3D12Ready())
		D3D12_PostUpdate();
#endif
}

//----------------------------------------------------------------------------

#endif // (PK_HAS_GPU == 1)

//----------------------------------------------------------------------------
//
// D3D11
//
//----------------------------------------------------------------------------
#if (PK_GPU_D3D11 == 1)

namespace
{

	u32	D3D11_GetRefCount(IUnknown *o)
	{
		const u32			r = o->AddRef();
		PK_ASSERT(r > 0);
		o->Release();
		return r - 1;
	}

	PopcornFX::CParticleUpdateManager_D3D11		*GetUpdateManagerD3D11_IFP(PopcornFX::CParticleUpdateManager *abstractUpdateManager)
	{
		if (abstractUpdateManager->UpdateClass() == PopcornFX::CParticleUpdateManager_D3D11::DefaultUpdateClass())
		{
			return PopcornFX::checked_cast<PopcornFX::CParticleUpdateManager_D3D11*>(abstractUpdateManager);
		}
		else if (abstractUpdateManager->UpdateClass() == PopcornFX::CParticleUpdateManager_Auto::DefaultUpdateClass())
		{
			PopcornFX::CParticleUpdateManager_Auto	*autoUpdateManager = static_cast<PopcornFX::CParticleUpdateManager_Auto*>(abstractUpdateManager);
			return autoUpdateManager->GetUpdateManager_GPU<PopcornFX::CParticleUpdateManager_D3D11>();
		}
		return null;
	}

	struct SFetchD3D11Context
	{
		bool							volatile m_Finished = false;
		struct ID3D11Device				* volatile m_Device = null;
		struct ID3D11DeviceContext		* volatile m_DeferedContext = null;

		void		Fetch()
		{
			m_Device = (ID3D11Device*)RHIGetNativeDevice();
			if (!PK_VERIFY(m_Device != null))
			{
				m_Finished = true;
				return;
			}
			ID3D11DeviceContext		*deferedContext = null;
			HRESULT hr;
			hr = m_Device->CreateDeferredContext(0, &deferedContext);
			if (!PK_VERIFY(!FAILED(hr)))
			{
				m_Finished = true;
				return;
			}
			PK_ASSERT(deferedContext != null);
			m_DeferedContext = deferedContext;
			m_Finished = true;
		}
	};
} // namespace

//----------------------------------------------------------------------------

bool	CParticleScene::D3D11_InitIFN()
{
	PK_ASSERT(m_EnableD3D11);

	if (D3D11Ready())
		return true;

	PK_ASSERT(m_D3D11_Device == null && m_D3D11_DeferedContext == null);

	PopcornFX::CParticleUpdateManager_D3D11		*updateManager_D3D11 = GetUpdateManagerD3D11_IFP(m_ParticleMediumCollection->UpdateManager());
	if (updateManager_D3D11 == null)
	{
		m_EnableD3D11 = false;
		return false;
	}
	PK_ASSERT(updateManager_D3D11->GPU_InternalDeferredContext() == null);

	SFetchD3D11Context		*fetch = new SFetchD3D11Context;

	ENQUEUE_RENDER_COMMAND(PopcornFXFetchDeferredContext)(
		[fetch](FRHICommandListImmediate& RHICmdList)
		{
			fetch->Fetch();
		});

	FlushRenderingCommands();

	if (!PK_RELEASE_VERIFY(fetch->m_Finished))
	{
		m_EnableD3D11 = false;
		delete fetch;
		return false;
	}

	if (!PK_VERIFY(fetch->m_Device != null && fetch->m_DeferedContext != null))
	{
		m_EnableD3D11 = false;
		delete fetch;
		return false;
	}

	m_D3D11_Device = fetch->m_Device;
	m_D3D11_DeferedContext = fetch->m_DeferedContext;

	delete fetch;

	PK_ASSERT(D3D11_GetRefCount(m_D3D11_DeferedContext) == 1);

	updateManager_D3D11->BindD3D11(
		m_D3D11_Device,
		PopcornFX::CParticleUpdateManager_D3D11::CbEnqueueImmediateTask(this, &CParticleScene::D3D11_EnqueueImmediateTask)
	);

	updateManager_D3D11->BindD3D11DeferredContext(m_D3D11_DeferedContext);

	return true;
}

//----------------------------------------------------------------------------

PopcornFX::TAtomic<u32>				g_D3D11_PendingTasks = 0;

//----------------------------------------------------------------------------

static void		_D3D11_ExecuteImmTasksArray(CParticleScene *self)
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::_D3D11_ExecuteImmTasksArray", POPCORNFX_UE_PROFILER_COLOR);

	auto			&m_D3D11_Tasks_Lock = self->m_D3D11_Tasks_Lock;
	auto			&m_Exec_D3D11_Tasks = self->m_Exec_D3D11_Tasks;
	auto			&m_UpdateLock = self->m_UpdateLock;

	ID3D11Device			*device = (ID3D11Device*)RHIGetNativeDevice();
	if (!PK_RELEASE_VERIFY(device != null))
		return;
	ID3D11DeviceContext		*immContext = null;
	device->GetImmediateContext(&immContext);
	if (!PK_RELEASE_VERIFY(immContext != null))
		return;

	{
		PK_SCOPEDLOCK(m_UpdateLock);
		PK_SCOPEDLOCK(m_D3D11_Tasks_Lock);

		// Fake a UAV transition so UE4 is happy, and disables UAV overlap (undefined behavior, generates random artefacts in the D3D11 GPU sim)
		// This is ugly, but they removed the implementations of FD3D11DynamicRHI::RHIBeginUAVOverlap/RHIEndUAVOverlap in 4.26.
		FRHICommandListImmediate	&RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
		RHICmdList.Transition(FRHITransitionInfo((FRHIUnorderedAccessView*)0x1234, ERHIAccess::UAVGraphics, ERHIAccess::UAVCompute));
		RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);

		for (u32 i = 0; i < m_Exec_D3D11_Tasks.Count(); ++i)
		{
			m_Exec_D3D11_Tasks[i]->Execute(immContext);
		}
		m_Exec_D3D11_Tasks.Clear();
	}

	SAFE_RELEASE(immContext); // GetImmediateContext AddRefs
}

//----------------------------------------------------------------------------

void	CParticleScene::D3D11_EnqueueImmediateTask(const PopcornFX::PParticleUpdaterImmediateTaskD3D11 &task)
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::D3D11_EnqueueImmediateTask", POPCORNFX_UE_PROFILER_COLOR);

	PK_ASSERT(m_EnableD3D11);
	if (!PK_RELEASE_VERIFY(D3D11Ready()))
		return;

	PK_RELEASE_ASSERT(task != null);

	m_CurrentUpdate_D3D11_Tasks.PushBack(task);
}

//----------------------------------------------------------------------------

void	CParticleScene::D3D11_Destroy() // GPU_Destroy()
{
	if (m_ParticleMediumCollection != null)
	{
		PopcornFX::CParticleUpdateManager_D3D11		*updateManager_D3D11 = GetUpdateManagerD3D11_IFP(m_ParticleMediumCollection->UpdateManager());
		if (updateManager_D3D11 != null &&
			updateManager_D3D11->GPU_InternalDeferredContext() != null)
		{
			updateManager_D3D11->BindD3D11(null, null);
		}
	}
	m_D3D11_DeferedContext = null;
	m_D3D11_Device = null;

	PK_SCOPEDLOCK(m_D3D11_Tasks_Lock);
	// @FIXME: if this is not for MediumCollection Clear or Destroy, there will be bugs
	m_Exec_D3D11_Tasks.Clear();
	m_CurrentUpdate_D3D11_Tasks.Clear();
}

//----------------------------------------------------------------------------

void	CParticleScene::D3D11_PreUpdate() // GPU_PreUpdate()
{
	PK_ASSERT(D3D11Ready());

	u32			pendingTasksCount = m_Exec_D3D11_Tasks.Count();
	if (pendingTasksCount > 0)
	{
		// make sure with a lock
		PK_SCOPEDLOCK(m_D3D11_Tasks_Lock);
		pendingTasksCount = m_Exec_D3D11_Tasks.Count();
	}

	// We NEED to wait tasks or the Runtime will FREEZE !

	if (pendingTasksCount > 0)
	{
		//UE_LOG(LogPopcornFXScene, Warning, TEXT("FlushRenderingCommands for last frame pending GPU immediate tasks (%d tasks) ... "), pendingTasksCount);

		{
			SCOPE_CYCLE_COUNTER(STAT_PopcornFX_GPU_PreUpdate_Flush);
			FlushRenderingCommands();
		}

		{
			PK_SCOPEDLOCK(m_D3D11_Tasks_Lock);
			PK_RELEASE_ASSERT(m_Exec_D3D11_Tasks.Empty());
		}
	}
}

//----------------------------------------------------------------------------

void	CParticleScene::D3D11_PreUpdateFence() // GPU_PreUpdateFence()
{
	PK_ASSERT(D3D11Ready());
}

//----------------------------------------------------------------------------

void	CParticleScene::D3D11_PostUpdate() // GPU_PostUpdate()
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::D3D11_PostUpdate", POPCORNFX_UE_PROFILER_COLOR);

	PK_ASSERT(D3D11Ready());

	{
		PK_SCOPEDLOCK(m_D3D11_Tasks_Lock);

		m_Exec_D3D11_Tasks.Reserve(m_Exec_D3D11_Tasks.Count() + m_CurrentUpdate_D3D11_Tasks.Count());

		for (u32 i = 0; i < m_CurrentUpdate_D3D11_Tasks.Count(); ++i)
			m_Exec_D3D11_Tasks.PushBack(m_CurrentUpdate_D3D11_Tasks[i]);

		m_CurrentUpdate_D3D11_Tasks.Clear();
	}

	CParticleScene	*self = this;
	ENQUEUE_RENDER_COMMAND(PopcornFX_D3D11_EnqueueImmediateTasksArray)(
		[self](FRHICommandListImmediate& RHICmdList)
		{
			_D3D11_ExecuteImmTasksArray(self);
		});

	//FlushRenderingCommands();
}

//----------------------------------------------------------------------------

void	CParticleScene::D3D11_PreRender() // GPU_PreRender()
{
	PK_ASSERT(D3D11Ready());

	u32		pendingTasksCount = m_Exec_D3D11_Tasks.Count();
	if (pendingTasksCount > 0)
	{
		// make sure with a lock
		PK_SCOPEDLOCK(m_D3D11_Tasks_Lock);
		pendingTasksCount = m_Exec_D3D11_Tasks.Count();
	}

	if (pendingTasksCount > 0)
	{
		SCOPE_CYCLE_COUNTER(STAT_PopcornFX_GPU_PreRender_Exec);
		_D3D11_ExecuteImmTasksArray(this);
	}
	return;
}

//----------------------------------------------------------------------------

#endif // (PK_GPU_D3D11 == 1)

//----------------------------------------------------------------------------
//
// D3D12
//
//----------------------------------------------------------------------------
#if (PK_GPU_D3D12 == 1)

namespace
{
	PopcornFX::CParticleUpdateManager_D3D12		*GetUpdateManagerD3D12_IFP(PopcornFX::CParticleUpdateManager *abstractUpdateManager)
	{
		if (abstractUpdateManager->UpdateClass() == PopcornFX::CParticleUpdateManager_D3D12::DefaultUpdateClass())
		{
			return PopcornFX::checked_cast<PopcornFX::CParticleUpdateManager_D3D12*>(abstractUpdateManager);
		}
		else if (abstractUpdateManager->UpdateClass() == PopcornFX::CParticleUpdateManager_Auto::DefaultUpdateClass())
		{
			PopcornFX::CParticleUpdateManager_Auto	*autoUpdateManager = static_cast<PopcornFX::CParticleUpdateManager_Auto*>(abstractUpdateManager);
			return autoUpdateManager->GetUpdateManager_GPU<PopcornFX::CParticleUpdateManager_D3D12>();
		}
		return null;
	}

	struct SFetchD3D12Context
	{
		bool							volatile m_Finished = false;
		struct ID3D12Device				* volatile m_Device = null;
		struct ID3D12CommandQueue		* volatile m_CopyCommandQueue = null;

		void		Fetch()
		{
			m_Device = (ID3D12Device*)RHIGetNativeDevice();

			PK_RELEASE_ASSERT(GDynamicRHI != null);
			FD3D12DynamicRHI	*dynamicRHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);

#if 0
			m_CopyCommandQueue = dynamicRHI->GetAdapter().GetDevice(0)->GetD3DCommandQueue(ED3D12CommandQueueType::Copy);
#endif
			if (!PK_VERIFY(m_Device != null))
			{
				m_Finished = true;
				return;
			}
			m_Finished = true;
		}
	};
}

//----------------------------------------------------------------------------

PopcornFX::TAtomic<u32>	g_D3D12_PendingTasks = 0;

static void		_D3D12_ExecuteTasksArray(CParticleScene *self)
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::_D3D12_ExecuteTasksArray", POPCORNFX_UE_PROFILER_COLOR);

	auto			&m_D3D12_Tasks_Lock = self->m_D3D12_Tasks_Lock;
	auto			&m_Exec_D3D12_Tasks = self->m_Exec_D3D12_Tasks;
	//auto			&m_UpdateLock = self->m_UpdateLock;

	PK_RELEASE_ASSERT(GDynamicRHI != null);
	FD3D12DynamicRHI	*dynamicRHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
	ID3D12CommandQueue	*commandQueue = dynamicRHI->RHIGetD3DCommandQueue(); // Returns the direct command queue
	// TODO: Investigate performance when using the compute command queue

	{
		//PK_SCOPEDLOCK(m_UpdateLock);
		PK_SCOPEDLOCK(m_D3D12_Tasks_Lock);

		for (u32 i = 0; i < m_Exec_D3D12_Tasks.Count(); ++i)
		{
			m_Exec_D3D12_Tasks[i]->Execute(commandQueue);
		}

		m_Exec_D3D12_Tasks.Clear();
	}
}

//----------------------------------------------------------------------------

void	CParticleScene::D3D12_SubmitSimCommandLists()
{
	_D3D12_ExecuteTasksArray(this);
}

//----------------------------------------------------------------------------

void	CParticleScene::D3D12_EnqueueTask(const PopcornFX::PParticleUpdaterTaskD3D12 &task)
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::D3D12_EnqueueTask", POPCORNFX_UE_PROFILER_COLOR);

	PK_ASSERT(m_EnableD3D12);
	if (!PK_RELEASE_VERIFY(D3D12Ready()))
		return;

	PK_RELEASE_ASSERT(task != null);

	m_CurrentUpdate_D3D12_Tasks.PushBack(task);
}

//----------------------------------------------------------------------------

bool	CParticleScene::D3D12_InitIFN()
{
	PK_ASSERT(m_EnableD3D12);

	if (D3D12Ready())
		return true;

	FCoreDelegates::OnBeginFrameRT.AddRaw(this, &CParticleScene::D3D12_SubmitSimCommandLists);

	PK_ASSERT(m_D3D12_Device == null);

	SFetchD3D12Context	*fetch = new SFetchD3D12Context;

	ENQUEUE_RENDER_COMMAND(PopcornFXFetchDeferredContext)(
		[fetch](FRHICommandListImmediate& RHICmdList)
		{
			fetch->Fetch();
		});

	FlushRenderingCommands();

	if (!PK_RELEASE_VERIFY(fetch->m_Finished) ||
		fetch->m_Device == null)
	{
		m_EnableD3D12 = false;
		delete fetch;
		return false;
	}

	PopcornFX::CParticleUpdateManager_D3D12	*updateManager_D3D12 = GetUpdateManagerD3D12_IFP(m_ParticleMediumCollection->UpdateManager());
	if (!PK_VERIFY(updateManager_D3D12 != null))
	{
		m_EnableD3D12 = false;
		delete fetch;
		return false;
	}

	m_D3D12_Device = fetch->m_Device;
	PopcornFX::SD3D12Context	context(m_D3D12_Device, fetch->m_CopyCommandQueue, &D3D12SerializeRootSignature, PopcornFX::CParticleUpdateManager_D3D12::CbEnqueueTask(this, &CParticleScene::D3D12_EnqueueTask));
	delete fetch;

	if (!PK_VERIFY(updateManager_D3D12->BindContext(context)))
	{
		m_EnableD3D12 = false;
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

void	CParticleScene::D3D12_Destroy() // GPU_Destroy()
{
	if (m_ParticleMediumCollection != null)
	{
		PopcornFX::CParticleUpdateManager_D3D12		*updateManager_D3D12 = GetUpdateManagerD3D12_IFP(m_ParticleMediumCollection->UpdateManager());
		if (updateManager_D3D12 != null)
			updateManager_D3D12->UnbindContext();
	}
	m_D3D12_Device = null;
	FCoreDelegates::OnBeginFrameRT.RemoveAll(this);
}

//----------------------------------------------------------------------------

void	CParticleScene::D3D12_PreUpdate() // GPU_PreUpdate()
{
	PK_ASSERT(D3D12Ready());

}

//----------------------------------------------------------------------------

void	CParticleScene::D3D12_PreUpdateFence() // GPU_PreUpdateFence()
{
	PK_ASSERT(D3D12Ready());
}

//----------------------------------------------------------------------------

void	CParticleScene::D3D12_PostUpdate() // GPU_PostUpdate()
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::D3D12_PostUpdate", POPCORNFX_UE_PROFILER_COLOR);

	PK_ASSERT(D3D12Ready());

	{
		PK_SCOPEDLOCK(m_D3D12_Tasks_Lock);

		if (!PK_VERIFY(m_Exec_D3D12_Tasks.Reserve(m_Exec_D3D12_Tasks.Count() + m_CurrentUpdate_D3D12_Tasks.Count())))
			return;

		for (u32 i = 0; i < m_CurrentUpdate_D3D12_Tasks.Count(); ++i)
			m_Exec_D3D12_Tasks.PushBack(m_CurrentUpdate_D3D12_Tasks[i]);

		m_CurrentUpdate_D3D12_Tasks.Clear();
	}
}

//----------------------------------------------------------------------------

void	CParticleScene::D3D12_PreRender() // GPU_PreRender()
{
	PK_ASSERT(D3D12Ready());
}

//----------------------------------------------------------------------------

#endif // (PK_GPU_D3D12 == 1)

//----------------------------------------------------------------------------
//
//
//
// Events
//
//
//
//----------------------------------------------------------------------------

DECLARE_CYCLE_STAT_EXTERN(TEXT("RaiseEvent"), STAT_PopcornFX_RaiseEventTime, STATGROUP_PopcornFX, );
DEFINE_STAT(STAT_PopcornFX_RaiseEventTime);

// static
CParticleScene::SPopcornFXPayloadView	CParticleScene::SPopcornFXPayloadView::Invalid = CParticleScene::SPopcornFXPayloadView();

//----------------------------------------------------------------------------

CParticleScene::SPopcornFXEventListener::SPopcornFXEventListener(const PopcornFX::CStringId &eventName)
:	m_EventName(eventName)
{
}

//----------------------------------------------------------------------------

CParticleScene::SPopcornFXPayload::SPopcornFXPayload(const PopcornFX::CStringId &payloadName, const EPopcornFXPayloadType::Type payloadType)
:	m_PayloadName(payloadName)
,	m_PayloadType(payloadType)
{
}

//----------------------------------------------------------------------------

CParticleScene::SPopcornFXPayloadView::SPopcornFXPayloadView()
:	m_PayloadMedium(null)
,	m_EventID(0)
,	m_CurrentParticle(0)
,	m_KillTimer(3.f)
{
}

//----------------------------------------------------------------------------

CParticleScene::SPopcornFXPayloadView::SPopcornFXPayloadView(const PopcornFX::CParticleMedium *medium, u32 eventID)
:	m_PayloadMedium(medium)
,	m_EventID(eventID)
,	m_CurrentParticle(0)
,	m_KillTimer(3.f)
{
}

//----------------------------------------------------------------------------

bool	operator==(const CParticleScene::SPopcornFXEventListener &other, const PopcornFX::CStringId &eventName)
{
	return eventName == other.m_EventName;
}

//----------------------------------------------------------------------------

bool	operator==(const CParticleScene::SPopcornFXPayload &other, const PopcornFX::CStringId &payloadName)
{
	return payloadName == other.m_PayloadName;
}

//----------------------------------------------------------------------------

void	CParticleScene::_Clear_Events()
{
	PK_SCOPEDLOCK(m_RaiseEventLock);

	m_PayloadViews.Clean();
	m_PendingEventAssocs.Clear();
	m_EventListeners.Clear();
}

//----------------------------------------------------------------------------

void	CParticleScene::ClearPendingEvents_NoLock()
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::ClearPendingEvents", POPCORNFX_UE_PROFILER_COLOR);
	const u32	payloadViewsCount = m_PayloadViews.Count();
	for (u32 iPayloadView = 0; iPayloadView < payloadViewsCount; ++iPayloadView)
	{
		SPopcornFXPayloadView	&payloadView = m_PayloadViews[iPayloadView];

		if (payloadView.Valid())
		{
			payloadView.m_PayloadMedium = null;
			payloadView.m_Payloads.Clear();
			payloadView.m_CurrentParticle = 0;
		}
	}
	m_PendingEventAssocs.Clear();
}

//----------------------------------------------------------------------------

void	CParticleScene::_PostUpdate_Events()
{
	INC_DWORD_STAT_BY(STAT_PopcornFX_BroadcastedEventsCount, m_PendingEventAssocs.Count());

	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::_PostUpdate_Events", POPCORNFX_UE_PROFILER_COLOR);

	const u32	elAssocCount = m_PendingEventAssocs.Count();
	for (u32 iAssoc = 0; iAssoc < elAssocCount; ++iAssoc)
	{
		const SPopcornFXEventListenerAssoc				&assoc = m_PendingEventAssocs[iAssoc];
		const PopcornFX::TArray<PopcornFX::CEffectID>	&effectIDs = assoc.m_EffectIDs;
		const u32										eventCount = effectIDs.Count();

		m_CurrentPayloadView = &m_PayloadViews[assoc.m_PayloadViewIndex];
		m_CurrentPayloadView->m_KillTimer = 3.f;

		PK_NAMEDSCOPEDPROFILE_C("CParticleScene::_PostUpdate_Events_TreatEventAssoc", POPCORNFX_UE_PROFILER_COLOR);
		for (u32 iEvent = 0; iEvent < eventCount; ++iEvent)
		{
			// execute callbacks that match with effect ID
			const u32	delegateCount = assoc.m_Event->m_Delegates.Count();

			PK_ASSERT(delegateCount == assoc.m_Event->m_EffectIDs.Count());
			for (u32 iDelegate = 0; iDelegate < delegateCount; ++iDelegate)
			{
				PK_NAMEDSCOPEDPROFILE_C("CParticleScene::_PostUpdate_Events_TreatSingleDelegate", POPCORNFX_UE_PROFILER_COLOR);

				if (assoc.m_Event->m_EffectIDs[iDelegate] == assoc.m_EffectIDs[iEvent])
					assoc.m_Event->m_Delegates[iDelegate].ExecuteIfBound();
			}

			++m_CurrentPayloadView->m_CurrentParticle;
		}
	}

	ClearPendingEvents_NoLock();

	const UWorld	*world = m_SceneComponent->GetWorld();

	const u32		payloadViewsCount = m_PayloadViews.Count();
	for (u32 iPayloadView = 0; iPayloadView < payloadViewsCount; ++iPayloadView)
	{
		SPopcornFXPayloadView	&payloadView = m_PayloadViews[iPayloadView];

		if (payloadView.Valid())
		{
			payloadView.m_KillTimer -= world->DeltaTimeSeconds;

			if (payloadView.m_KillTimer <= 0.f)
				m_PayloadViews.Remove(iPayloadView);
		}
	}
}

//----------------------------------------------------------------------------

void	CParticleScene::BroadcastEvent(
	PopcornFX::Threads::SThreadContext				*threadCtx,
	PopcornFX::CParticleMedium						*parentMedium,
	u32												eventID,
	PopcornFX::CStringId							eventName,
	u32												count,
	const PopcornFX::SUpdateTimeArgs				&timeArgs,
	const TMemoryView<const float>					&spawnDtToEnd,
	const TMemoryView<const PopcornFX::CEffectID>	&effectIDs,
	const PopcornFX::SPayloadView					&payloadView)
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::BroadcastEvent", POPCORNFX_UE_PROFILER_COLOR);
	SCOPE_CYCLE_COUNTER(STAT_PopcornFX_RaiseEventTime);

	if (!PK_VERIFY(m_SceneComponent != null))
		return;

	if (!m_SceneComponent->IsValidLowLevel())
		return;

	PK_ASSERT(m_SceneComponent->GetWorld() != null);
	if (!m_SceneComponent->GetWorld()->IsGameWorld())
		return;

	const PopcornFX::CGuid	eventListenerIndex = m_EventListeners.IndexOf(eventName);
	// Is an event listener was registered for this specific event, if not, return
	if (!eventListenerIndex.Valid())
		return;

	PK_SCOPEDLOCK(m_RaiseEventLock);

	// search for existing payload view
	PopcornFX::CGuid		payloadViewIndex = PopcornFX::CGuid::INVALID;

	const u32				payloadViewsCount = m_PayloadViews.Count();
	for (u32 iPayloadView = 0; iPayloadView < payloadViewsCount; ++iPayloadView)
	{
		const SPopcornFXPayloadView	&currPayloadView = m_PayloadViews[iPayloadView];
		if (currPayloadView.Valid() && currPayloadView.m_EventID == eventID && currPayloadView.m_PayloadMedium == parentMedium)
		{
			payloadViewIndex = iPayloadView;
			break;
		}
	}

	if (!payloadViewIndex.Valid()) // new payload view
	{
		payloadViewIndex = m_PayloadViews.Insert(SPopcornFXPayloadView(parentMedium, eventID));
		if (!PK_VERIFY(payloadViewIndex.Valid()))
			return;
	}

	RetrievePayloadElements(payloadView, m_PayloadViews[payloadViewIndex]);

	// create new event
	SPopcornFXEventListenerAssoc	newAssoc(&m_EventListeners[eventListenerIndex], effectIDs, payloadViewIndex);

	if (!PK_VERIFY(m_PendingEventAssocs.PushBack(newAssoc).Valid())) // add event listener to pending array
		return;
}

//----------------------------------------------------------------------------

template <typename _OutType>
void	CParticleScene::FillPayload(const PopcornFX::SPayloadElementView &srcPayloadElementData, SPopcornFXPayload &dstPayload)
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::FillPayload", POPCORNFX_UE_PROFILER_COLOR);

	const u32		valueOffset = dstPayload.m_Values.Count();
	const u32		particleCount = srcPayloadElementData.m_Data.m_Count;
	const u32		particleStride = srcPayloadElementData.m_Data.m_Stride;

	if (!PK_VERIFY(dstPayload.m_Values.Resize(valueOffset + particleCount)))
		return;

	for (u32 iParticle = 0; iParticle < particleCount; ++iParticle)
	{
		SPopcornFXPayloadValue	&value = dstPayload.m_Values[valueOffset + iParticle];

		FGenericPlatformMemory::Memcpy(value.m_ValueBool, srcPayloadElementData.m_Data.m_RawDataPtr + iParticle * particleStride, sizeof(_OutType));
	}
}

//----------------------------------------------------------------------------

void    CParticleScene::FillPayloadBool(const PopcornFX::SPayloadElementView &srcPayloadElementData, SPopcornFXPayload &dstPayload, u32 dim)
{
	const u32	valueOffset = dstPayload.m_Values.Count();
	const u32	particleCount = srcPayloadElementData.m_Data.m_Count;
	const u32	particleStride = srcPayloadElementData.m_Data.m_Stride;

	if (!PK_VERIFY(dstPayload.m_Values.Resize(valueOffset + particleCount)))
		return;

	for (u32 iParticle = 0; iParticle < particleCount; ++iParticle)
	{
		SPopcornFXPayloadValue	&value = dstPayload.m_Values[valueOffset + iParticle];
		const u32				*srcPtr = (u32*)&srcPayloadElementData.m_Data.m_RawDataPtr[iParticle * particleStride];

		for (u32 dimIdx = 0; dimIdx < dim; dimIdx++)
			value.m_ValueBool[dimIdx] = (srcPtr[dimIdx] == 0) ? false : true;
	}
}

//----------------------------------------------------------------------------

void	CParticleScene::RetrievePayloadElements(const PopcornFX::SPayloadView &srcPayloadView, SPopcornFXPayloadView &dstPayloadView)
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::RetrievePayloadElements", POPCORNFX_UE_PROFILER_COLOR);

	dstPayloadView.m_Payloads.Reserve(srcPayloadView.m_PayloadElements.Count());

	// loop on payloads
	const u32	payloadElementsCount = srcPayloadView.m_PayloadElements.Count();
	for (u32 iPayloadElement = 0; iPayloadElement < payloadElementsCount; ++iPayloadElement)
	{
		const PopcornFX::SParticleDeclaration::SEvent::SPayload	&srcPayload = srcPayloadView.m_EventDesc->m_Payload[iPayloadElement];

		// search for existing payload
		PopcornFX::CGuid					payloadIndex = dstPayloadView.m_Payloads.IndexOf(srcPayload.m_NameGUID);

		const EPopcornFXPayloadType::Type	payloadElementType = EPopcornFXPayloadType::FromPopcornFXBaseTypeID(srcPayload.m_Type);

		if (!payloadIndex.Valid()) // new payload
		{
			payloadIndex = dstPayloadView.m_Payloads.PushBack(SPopcornFXPayload(srcPayload.m_NameGUID, payloadElementType));
			if (!PK_VERIFY(payloadIndex.Valid()))
				continue;
		}

		// fill payload element data into m_PayloadViews
		switch (payloadElementType)
		{
			case	EPopcornFXPayloadType::Type::Bool:
				FillPayloadBool(srcPayloadView.m_PayloadElements[iPayloadElement], dstPayloadView.m_Payloads[payloadIndex], 1);
				break;
			case	EPopcornFXPayloadType::Type::Bool2:
				FillPayloadBool(srcPayloadView.m_PayloadElements[iPayloadElement], dstPayloadView.m_Payloads[payloadIndex], 2);
				break;
			case	EPopcornFXPayloadType::Type::Bool3:
				FillPayloadBool(srcPayloadView.m_PayloadElements[iPayloadElement], dstPayloadView.m_Payloads[payloadIndex], 3);
				break;
			case	EPopcornFXPayloadType::Type::Bool4:
				FillPayloadBool(srcPayloadView.m_PayloadElements[iPayloadElement], dstPayloadView.m_Payloads[payloadIndex], 4);
				break;
			case	EPopcornFXPayloadType::Type::Float:
				FillPayload<float>(srcPayloadView.m_PayloadElements[iPayloadElement], dstPayloadView.m_Payloads[payloadIndex]);
				break;
			case	EPopcornFXPayloadType::Type::Float2:
				FillPayload<CFloat2>(srcPayloadView.m_PayloadElements[iPayloadElement], dstPayloadView.m_Payloads[payloadIndex]);
				break;
			case	EPopcornFXPayloadType::Type::Float3:
				FillPayload<CFloat3>(srcPayloadView.m_PayloadElements[iPayloadElement], dstPayloadView.m_Payloads[payloadIndex]);
				break;
			case	EPopcornFXPayloadType::Type::Float4:
			case	EPopcornFXPayloadType::Type::Quaternion:
				FillPayload<CFloat4>(srcPayloadView.m_PayloadElements[iPayloadElement], dstPayloadView.m_Payloads[payloadIndex]);
				break;
			case	EPopcornFXPayloadType::Type::Int:
				FillPayload<u32>(srcPayloadView.m_PayloadElements[iPayloadElement], dstPayloadView.m_Payloads[payloadIndex]);
				break;
			case	EPopcornFXPayloadType::Type::Int2:
				FillPayload<CInt2>(srcPayloadView.m_PayloadElements[iPayloadElement], dstPayloadView.m_Payloads[payloadIndex]);
				break;
			case	EPopcornFXPayloadType::Type::Int3:
				FillPayload<CInt3>(srcPayloadView.m_PayloadElements[iPayloadElement], dstPayloadView.m_Payloads[payloadIndex]);
				break;
			case	EPopcornFXPayloadType::Type::Int4:
				FillPayload<CInt4>(srcPayloadView.m_PayloadElements[iPayloadElement], dstPayloadView.m_Payloads[payloadIndex]);
				break;
			default:
				break;
		}
	}

	// Check that all SPopcornFXPayload::m_Values.Count() are the same for all SPopcornFXPayload
	PK_ONLY_IF_ASSERTS(
		const u32	payloadViewsCount = m_PayloadViews.Count();
		for (u32 iPayloadView = 0; iPayloadView < payloadViewsCount; ++iPayloadView)
		{
			const PopcornFX::TArray<SPopcornFXPayload>	&payloads = m_PayloadViews[iPayloadView].m_Payloads;

			if (payloads.Count() > 0)
			{
				const u32	firstPayloadValuesCount = payloads[0].m_Values.Count();

				const u32	payloadsCount = payloads.Count();
				for (u32 iPayloadIndex = 1; iPayloadIndex < payloadsCount; ++iPayloadIndex)
					PK_ASSERT(firstPayloadValuesCount == payloads[iPayloadIndex].m_Values.Count());
			}
		}
	);
}

//----------------------------------------------------------------------------

bool	CParticleScene::RegisterEventListener(UPopcornFXEffect* particleEffect, PopcornFX::CEffectID effectID, const PopcornFX::CStringId &eventNameID, FPopcornFXRaiseEventSignature &callback)
{
	PK_ASSERT(IsInGameThread());

	if (!PK_VERIFY(m_SceneComponent != null))
		return false;

	PK_ASSERT(m_SceneComponent->GetWorld() != null);
	if (!m_SceneComponent->GetWorld()->IsGameWorld())
		return false;

	const PopcornFX::FastDelegate<PopcornFX::CParticleEffect::EventCallback>	broadcastCallback(this, &CParticleScene::BroadcastEvent);

	if (!particleEffect->Effect()->RegisterEventCallback(broadcastCallback, eventNameID))
	{
		UE_LOG(LogPopcornFXScene, Warning, TEXT("Register Event Listener: Couldn't register callback to event '%s' on effect '%s'"), eventNameID.ToString().Data(), *particleEffect->GetName());
		return false;
	}

	PopcornFX::CGuid	eventListenerIndex = m_EventListeners.IndexOf(eventNameID);
	if (!eventListenerIndex.Valid())
	{
		// First listener registered for this event, register it.
		eventListenerIndex = m_EventListeners.PushBack(eventNameID);
	}

	if (PK_VERIFY(eventListenerIndex.Valid()))
	{
		SPopcornFXEventListener	&eventListener = m_EventListeners[eventListenerIndex];

		if (!PK_VERIFY(eventListener.m_Delegates.PushBack(callback).Valid()) ||
			!PK_VERIFY(eventListener.m_EffectIDs.PushBack(effectID).Valid()))
			return false;

		PK_ASSERT(eventListener.m_Delegates.Count() == eventListener.m_EffectIDs.Count());
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------

void	CParticleScene::UnregisterEventListener(UPopcornFXEffect* particleEffect, PopcornFX::CEffectID effectID, const PopcornFX::CStringId &eventNameID, FPopcornFXRaiseEventSignature &callback)
{
	PK_ASSERT(IsInGameThread());

	if (m_EventListeners.Empty())
		return;

	const PopcornFX::FastDelegate<PopcornFX::CParticleEffect::EventCallback>	broadcastCallback(this, &CParticleScene::BroadcastEvent);

	particleEffect->Effect()->UnregisterEventCallback(broadcastCallback, eventNameID);

	const PopcornFX::CGuid	eventListenerIndex = m_EventListeners.IndexOf(eventNameID);
	if (eventListenerIndex.Valid())
	{
		SPopcornFXEventListener	&eventListener = m_EventListeners[eventListenerIndex];
		const u32				delegateCount = eventListener.m_Delegates.Count();

		PK_ASSERT(eventListener.m_Delegates.Count() == eventListener.m_EffectIDs.Count());
		for (u32 iDelegate = 0; iDelegate < delegateCount; ++iDelegate)
		{
			if (eventListener.m_Delegates[iDelegate] == callback)
			{
				PK_ASSERT(effectID == eventListener.m_EffectIDs[iDelegate]); // Should not happen
				eventListener.m_Delegates.Remove(iDelegate);
				eventListener.m_EffectIDs.Remove(iDelegate);
				ClearPendingEvents_NoLock();
				break;
			}
		}
	}
}

//----------------------------------------------------------------------------

void	CParticleScene::UnregisterAllEventsListeners(UPopcornFXEffect* particleEffect, PopcornFX::CEffectID effectID)
{
	PK_ASSERT(IsInGameThread());

	if (m_EventListeners.Empty())
		return;

	const PopcornFX::FastDelegate<PopcornFX::CParticleEffect::EventCallback>	broadcastCallback(this, &CParticleScene::BroadcastEvent);

	const u32	eventListenersCount = m_EventListeners.Count();

	for (u32 iEventListener = 0; iEventListener < eventListenersCount; ++iEventListener)
	{
		SPopcornFXEventListener	&eventListener = m_EventListeners[iEventListener];

		bool callbackUnregistered = false;

		PK_ASSERT(eventListener.m_Delegates.Count() == eventListener.m_EffectIDs.Count());
		for (u32 iEffectID = 0; iEffectID < eventListener.m_EffectIDs.Count(); ++iEffectID)
		{
			if (eventListener.m_EffectIDs[iEffectID] == effectID)
			{
				if (!callbackUnregistered)
				{
					callbackUnregistered = true;

					particleEffect->Effect()->UnregisterEventCallback(broadcastCallback, eventListener.m_EventName);
				}

				eventListener.m_Delegates.Remove(iEffectID);
				eventListener.m_EffectIDs.Remove(iEffectID);

				--iEffectID;
			}
		}
	}
	ClearPendingEvents_NoLock();
}

//----------------------------------------------------------------------------

namespace	EPopcornFXPayloadType
{
	inline u32	Dimension(Type type) { return (u32(type) % 4) + 1; }
}

//----------------------------------------------------------------------------

bool	CParticleScene::GetPayloadValue(const FName &payloadName, EPopcornFXPayloadType::Type expectedPayloadType, void *outValue) const
{
	PK_ASSERT(IsInGameThread());
	PK_ASSERT(!payloadName.IsNone() && payloadName.IsValid());
	PK_ASSERT(outValue != null);

	PK_NAMEDSCOPEDPROFILE_C("CParticleScene::GetPayloadValue", POPCORNFX_UE_PROFILER_COLOR);

	if (!m_CurrentPayloadView->Valid())
	{
		UE_LOG(LogPopcornFXScene, Warning, TEXT("Get Payload Value: Payload's data not setup. Are you trying to call this function in a BeginPlay?"));
		return false;
	}

	const PopcornFX::CStringId		payloadNameID(TCHAR_TO_ANSI(*payloadName.ToString())); // Expensive
	const PopcornFX::CGuid			payloadIndex = m_CurrentPayloadView->m_Payloads.IndexOf(payloadNameID);

	if (!payloadIndex.Valid())
	{
		UE_LOG(LogPopcornFXScene, Warning, TEXT("Get Payload Value: Cannot retrieve event payload '%s', no event payload registered."), *payloadName.ToString());
		return false;
	}

	const SPopcornFXPayload		&payload = m_CurrentPayloadView->m_Payloads[payloadIndex];

	if (payload.m_PayloadType != expectedPayloadType)
	{
		UE_LOG(LogPopcornFXScene, Warning, TEXT("Get Payload Value: mismatching types for event payload '%s'"), *payloadName.ToString());
		return false;
	}

	if (!PK_VERIFY(m_CurrentPayloadView->m_CurrentParticle < payload.m_Values.Count()))
	{
		UE_LOG(LogPopcornFXScene, Warning, TEXT("Get Payload Value: Couldn't find event payload '%s' (payload doesn't exist)"), *payloadName.ToString());
		return false;
	}

	const SPopcornFXPayloadValue	&value = payload.m_Values[m_CurrentPayloadView->m_CurrentParticle];

	const uint32					payloadValueDim = EPopcornFXPayloadType::Dimension(expectedPayloadType);

	FGenericPlatformMemory::Memcpy(outValue, value.m_ValueBool, sizeof(int32) * payloadValueDim);
	return true;
}

//----------------------------------------------------------------------------
