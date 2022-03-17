
//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "Render/RendererSubView.h"
#include "Render/PopcornFXBuffer.h"
#include "PopcornFXSettings.h"
#include "PopcornFXTypes.h"
#include "PopcornFXAudio.h"

#include "PrimitiveSceneProxy.h"
#include "Engine/EngineTypes.h"
#include "Math/BoxSphereBounds.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Assets/PopcornFXEffect.h"
#include "PopcornFXEmitterComponent.h" // TWeakObjectPtr

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_effect.h>
#include <pk_particles/include/ps_mediums.h>
#include <pk_particles/include/ps_scene.h>
#include <pk_particles/include/ps_system.h>
#include <pk_particles/include/ps_bounds.h>
#include <pk_particles/include/ps_stats.h>
#include <pk_kernel/include/kr_timers.h>

class	UPopcornFXSceneComponent;
class	FPopcornFXSceneProxy;

#define PK_WITH_PHYSX    PHYSICS_INTERFACE_PHYSX && WITH_PHYSX
#define PK_WITH_CHAOS    !(PK_WITH_PHYSX) && WITH_CHAOS

// The plugin doesn't support both being active at the same time.
#if PK_WITH_PHYSX && PK_WITH_CHAOS
#	error
#endif

#if PK_WITH_PHYSX
namespace	physx
{
	struct	PxRaycastHit;
	class	PxScene;
}
#endif // PK_WITH_PHYSX

FWD_PK_API_BEGIN
class	CParticleMediumCollection;
class	CParticleDescriptor;
struct	SSimDispatchHint;
namespace Drawers
{
	class	CScene;
}
PK_FORWARD_DECLARE(ParticleUpdaterImmediateTaskD3D11);
PK_FORWARD_DECLARE(ParticleUpdaterTaskD3D12);
FWD_PK_API_END
PK_FORWARD_DECLARE(RenderBatchManager);
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

class	CParticleScene : public PopcornFX::IParticleScene
{
public:
	typedef PopcornFX::IParticleScene	Super;

	static CParticleScene	*CreateNew(const UPopcornFXSceneComponent *sceneComp);
	static void				SafeDelete(CParticleScene *&scene);

private:
	CParticleScene();
	~CParticleScene();

public:
	void					Clear();

	const UPopcornFXSceneComponent		*SceneComponent() const;

	void					StartUpdate(float dt, enum ELevelTick tickType);
	void					ApplyWorldOffset(const FVector &inOffset);
	void					SendRenderDynamicData_Concurrent();
	bool					PostUpdate_ShouldMarkRenderStateDirty() const;
	void					GetUsedMaterials(TArray<UMaterialInterface*> &outMaterials, bool bGetDebugMaterials);

	void					GatherSimpleLights(const FSceneViewFamily& ViewFamily, FSimpleLightArray& OutParticleLights) const;

#if RHI_RAYTRACING
	void					GetDynamicRayTracingInstances(const FPopcornFXSceneProxy *sceneProxy, FRayTracingMaterialGatheringContext &context, TArray<FRayTracingInstance> &outRayTracingInstances);
#endif // RHI_RAYTRACING

	void					GetDynamicMeshElements(const FPopcornFXSceneProxy *sceneProxy, const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector);

	PopcornFX::CParticleMediumCollection		*Unsafe_ParticleMediumCollection() { return m_ParticleMediumCollection; }
	const PopcornFX::CParticleMediumCollection	*Unsafe_ParticleMediumCollection() const { return m_ParticleMediumCollection; }

	//const PopcornFX::CAABB	&PkSpaceParticleBounds() const { return m_CachedBounds.CachedBounds(); }
	const FBoxSphereBounds	&Bounds() const { return m_Bounds; }
//	uint32					LastUpdateFrameNumber() const { return m_LastUpdateFrameNumber; }

	u32						LastUpdatedParticleCount() const { if (m_LastTotalParticleCount < 0) return 0; return u32(m_LastTotalParticleCount); }

private:
	bool										InternalSetup(const UPopcornFXSceneComponent *sceneComp);

	static bool									_SimDispatchMask(const PopcornFX::CParticleDescriptor *descriptor, PopcornFX::SSimDispatchHint &outHint);
	u32											_InstanceCount(const PopcornFX::CParticleEffect *effect);

	//----------------------------------------------------------------------------

private:
	PopcornFX::CParticleMediumCollection		*m_ParticleMediumCollection;
	PopcornFX::CRendererSubView					m_SubView;

public:
	PopcornFX::Threads::CCriticalSection		m_UpdateLock;

private:
	PopcornFX::CSmartCachedBounds				m_CachedBounds;
	FBoxSphereBounds							m_Bounds;
	s32											m_LastTotalParticleCount = 0;

	PopcornFX::CGuid							m_ParticleMediumCollectionID;
	PopcornFX::CGuid							m_SpawnTransformsID;
	PopcornFX::CGuid							m_PositionsID;
	PopcornFX::CGuid							m_TransformsID;

//	uint32										m_LastUpdateFrameNumber;

	const UPopcornFXSceneComponent				*m_SceneComponent;

#if	(PK_PARTICLES_HAS_STATS != 0)
public:
	struct	SPopcornFXEffectTimings
	{
		const PopcornFX::CParticleEffect	*m_Effect = null; // Use the effect pointer as a compare value only
		PopcornFX::CString					m_EffectPath;
		PopcornFX::SEvolveStatsReport		m_TotalStatsReport;
		u32									m_TotalParticleCount_CPU = 0;
		u32									m_TotalParticleCount_GPU = 0;
		u32									m_InstanceCount = 0;

		float			TotalTime() const { return m_TotalStatsReport.m_PipelineStages[PopcornFX::SEvolveStatsReport::PipelineStage_Total].m_Time; }
		u32				TotalParticleCount_CPU() const { return m_TotalParticleCount_CPU; }
		u32				TotalParticleCount_GPU() const { return m_TotalParticleCount_GPU; }
		u32				TotalParticleCount() const { return m_TotalParticleCount_CPU + m_TotalParticleCount_GPU; }
		u32				TotalInstanceCount() const { return m_InstanceCount; }
		void			Merge(SPopcornFXEffectTimings &other)
		{
			m_TotalStatsReport += other.m_TotalStatsReport;
			m_TotalParticleCount_CPU += other.m_TotalParticleCount_CPU;
			m_TotalParticleCount_GPU += other.m_TotalParticleCount_GPU;
			m_InstanceCount += other.m_InstanceCount;
		}
	};

	const PopcornFX::TArray<SPopcornFXEffectTimings>	&EffectTimings() const { return m_EffectTimings_Sum; }
	float												MediumCollectionUpdateTime() const { return m_MediumCollectionUpdateTime_Average; }
	u32													MediumCollectionParticleCount_CPU() const { return m_MediumCollectionParticleCount_CPU_Average; }
	u32													MediumCollectionParticleCount_GPU() const { return m_MediumCollectionParticleCount_GPU_Average; }
	u32													MediumCollectionInstanceCount() const { return m_MediumCollectionInstanceCount_Average; }

private:
	PopcornFX::TArray<SPopcornFXEffectTimings>	m_EffectTimings;
	PopcornFX::TArray<SPopcornFXEffectTimings>	m_EffectTimings_Sum;
	u32											m_MediumCollectionUpdateTime_FrameCount = 0;
	double										m_MediumCollectionUpdateTime_Sum = 0.0;
	float										m_MediumCollectionUpdateTime_Average = 0.0f;
	u32											m_MediumCollectionParticleCount_CPU_Sum = 0;
	u32											m_MediumCollectionParticleCount_GPU_Sum = 0;
	u32											m_MediumCollectionInstanceCount_Sum = 0;
	u32											m_MediumCollectionParticleCount_CPU_Average = 0;
	u32											m_MediumCollectionParticleCount_GPU_Average = 0;
	u32											m_MediumCollectionInstanceCount_Average = 0;
#endif //	(PK_PARTICLES_HAS_STATS != 0)

private:
	//----------------------------------------------------------------------------

	void				_PreUpdate_Views();

	//----------------------------------------------------------------------------

	PopcornFX::CRendererSubView		m_RenderSubView;
	PopcornFX::CRendererSubView		m_UpdateSubView;

	uint64							m_LastUpdate = 0;

#if	POPCORNFX_RENDER_DEBUG
	bool							m_IsFreezedBillboardingMatrix = false;
	PopcornFX::CFloat4x4			m_FreezedBillboardingMatrix;
#endif

	PRenderBatchManager				m_RenderBatchManager;

	//----------------------------------------------------------------------------

	void		_PreUpdate(float dt, enum ELevelTick tickType);
	void		_Clear();

	//----------------------------------------------------------------------------
	//
	// Internal emitters management
	//
	//----------------------------------------------------------------------------
public:
	struct SEmitterRegister
	{
		TWeakObjectPtr<class UPopcornFXEmitterComponent>	m_Emitter;
		SEmitterRegister(class UPopcornFXEmitterComponent *emitter) : m_Emitter(emitter) { PK_ASSERT(Valid()); }
		SEmitterRegister() : m_Emitter(null) { }
		PK_FORCEINLINE bool						operator == (class UPopcornFXEmitterComponent *other) const { return m_Emitter == other; }
		PK_FORCEINLINE bool						Valid() const { return m_Emitter.IsValid(); }
		PK_FORCEINLINE bool						ValidLowLevel() const { return m_Emitter.IsValid(true, true); } // Emitter is about to be destroyed, but we still want to unregister it from the scene
		static SEmitterRegister					Invalid; // needed by TChunkedSlotArray
	};
	bool				Emitter_PreInitRegister(class UPopcornFXEmitterComponent *emitter);
	bool				Emitter_PreInitUnregister(class UPopcornFXEmitterComponent *emitter);
	bool				Emitter_IsRegistered(class UPopcornFXEmitterComponent *emitter);
	bool				Emitter_Register(class UPopcornFXEmitterComponent *emitter);
	bool				Emitter_Unregister(class UPopcornFXEmitterComponent *emitter);

	bool				Effect_Install(PopcornFX::PCParticleEffect &effect);

private:
	void				_PreUpdate_Emitters(float dt, enum ELevelTick tickType);
	void				_PostUpdate_Emitters(float dt, enum ELevelTick tickType);
	void				_Clear_Emitters();

	PopcornFX::TChunkedSlotArray<SEmitterRegister>	m_PreInitEmitters;
	PopcornFX::Threads::CCriticalSection			m_EmittersLock;
	PopcornFX::TChunkedSlotArray<SEmitterRegister>	m_Emitters;

private:

	//----------------------------------------------------------------------------
	//
	// Collisions
	//
	//----------------------------------------------------------------------------

	virtual void			RayTracePacket(
		const PopcornFX::Colliders::STraceFilter &traceFilter,
		const PopcornFX::Colliders::SRayPacket &packet,
		const PopcornFX::Colliders::STracePacket &results) override;

	virtual void			RayTracePacketTemporal(
		const PopcornFX::Colliders::STraceFilter &traceFilter,
		const PopcornFX::Colliders::SRayPacket &packet,
		const PopcornFX::Colliders::STracePacket &results) override;

	virtual void			ResolveContactMaterials(const PopcornFX::TMemoryView<void * const>									&contactObjects,
													const PopcornFX::TMemoryView<void * const>									&contactSurfaces,
													const PopcornFX::TMemoryView<PopcornFX::Colliders::SSurfaceProperties>		&outSurfaceProperties) const override;

	PopcornFX::Threads::CCriticalSection		m_RaytraceLock;

	void					_PreUpdate_Collisions();
#if PK_WITH_PHYSX
	physx::PxScene		*m_CurrentPhysxScene = null;
#else
	FChaosScene			*m_CurrentChaosScene = null;
#endif

	virtual	PopcornFX::TMemoryView<const float * const>	GetAudioSpectrum(PopcornFX::CStringId channelGroup, u32 &outBaseCount) const override;
	virtual	PopcornFX::TMemoryView<const float * const>	GetAudioWaveform(PopcornFX::CStringId channelGroup, u32 &outBaseCount) const override;

public:
	void								SetAudioInterface(class IPopcornFXAudio *audioInterface);

	class IPopcornFXFillAudioBuffers	*m_FillAudioBuffers;
	class IPopcornFXAudio				*m_AudioInterface; // Unused for now (v2)

#ifdef USE_DEFAULT_AUDIO_INTERFACE
	class FPopcornFXAudioDefault		*m_AudioInterfaceDefault;
#endif // USE_DEFAULT_AUDIO_INTERFACE
private:

#if (PK_HAS_GPU != 0)
	//----------------------------------------------------------------------------
	//
	// GPU Stuff
	//
	//----------------------------------------------------------------------------

private:
	bool			GPU_InitIFN();
	void			GPU_Destroy();
	void			GPU_PreRender();
	void			GPU_PreUpdate();
	void			GPU_PreUpdateFence();
	void			GPU_PostUpdate();

#if (PK_GPU_D3D11 == 1)
	//----------------------------------------------------------------------------
	// D3D11
	//----------------------------------------------------------------------------

public:
	bool							D3D11Ready() const { return m_EnableD3D11 && m_D3D11_DeferedContext != null; }
	struct ID3D11Device				*D3D11_Device() const { PK_ASSERT(D3D11Ready()); return m_D3D11_Device; }
	struct ID3D11DeviceContext		*D3D11_DeferedContext() const { PK_ASSERT(D3D11Ready()); return m_D3D11_DeferedContext; }

private:
	bool			D3D11_InitIFN();
	void			D3D11_Destroy();
	void			D3D11_PreRender();
	void			D3D11_PreUpdate();
	void			D3D11_PreUpdateFence();
	void			D3D11_PostUpdate();

	void			D3D11_EnqueueImmediateTask(const PopcornFX::PParticleUpdaterImmediateTaskD3D11 &task);

private:
	bool							m_EnableD3D11;
	struct ID3D11Device				*m_D3D11_Device = null;
	struct ID3D11DeviceContext		*m_D3D11_DeferedContext = null;

public:
	PopcornFX::Threads::CCriticalSection									m_D3D11_Tasks_Lock;
	PopcornFX::TArray<PopcornFX::PParticleUpdaterImmediateTaskD3D11>		m_CurrentUpdate_D3D11_Tasks;
	PopcornFX::TArray<PopcornFX::PParticleUpdaterImmediateTaskD3D11>		m_Exec_D3D11_Tasks;

#endif //(PK_GPU_D3D11 == 1)
#if (PK_GPU_D3D12 == 1)
	//----------------------------------------------------------------------------
	// D3D12
	//----------------------------------------------------------------------------

public:
	bool							D3D12Ready() const { return m_EnableD3D12 && m_D3D12_Device != null; }
	struct ID3D12Device				*D3D12_Device() const { PK_ASSERT(D3D12Ready()); return m_D3D12_Device; }

private:
	bool			D3D12_InitIFN();
	void			D3D12_Destroy();
	void			D3D12_PreRender();
	void			D3D12_PreUpdate();
	void			D3D12_PreUpdateFence();
	void			D3D12_PostUpdate();

	void			D3D12_SubmitSimCommandLists();
	void			D3D12_EnqueueTask(const PopcornFX::PParticleUpdaterTaskD3D12 &task);

private:
	bool							m_EnableD3D12;
	struct ID3D12Device				*m_D3D12_Device = null;

public:
	PopcornFX::Threads::CCriticalSection						m_D3D12_Tasks_Lock;
	PopcornFX::TArray<PopcornFX::PParticleUpdaterTaskD3D12>		m_CurrentUpdate_D3D12_Tasks;
	PopcornFX::TArray<PopcornFX::PParticleUpdaterTaskD3D12>		m_Exec_D3D12_Tasks;

#endif //(PK_GPU_D3D12 == 1)

#endif // (PK_HAS_GPU != 0)

	//----------------------------------------------------------------------------
	//
	// Events
	//
	//----------------------------------------------------------------------------
public:
	struct	SPopcornFXEventListener
	{
		typedef class FPopcornFXRaiseEventSignature	RaiseEventDelegate;

		PopcornFX::CStringId					m_EventName;
		PopcornFX::TArray<RaiseEventDelegate>	m_Delegates; // List of unique delegates
		PopcornFX::TArray<PopcornFX::CEffectID>	m_EffectIDs; // Can contain same effect ids, but bound to different delegates

		SPopcornFXEventListener(const PopcornFX::CStringId &eventName);
	};

	struct	SPopcornFXEventListenerAssoc
	{
		SPopcornFXEventListener					*m_Event;
		PopcornFX::TArray<PopcornFX::CEffectID>	m_EffectIDs;
		PopcornFX::CGuid						m_PayloadViewIndex;

		SPopcornFXEventListenerAssoc(SPopcornFXEventListener *eventListener)
		:	m_Event(eventListener)
		,	m_EffectIDs()
		,	m_PayloadViewIndex(PopcornFX::CGuid::INVALID) { }

		SPopcornFXEventListenerAssoc(SPopcornFXEventListener *eventListener, const PopcornFX::TArray<PopcornFX::CEffectID> &effectIDs, const PopcornFX::CGuid &payloadViewIndex)
		:	m_Event(eventListener)
		,	m_EffectIDs(effectIDs)
		,	m_PayloadViewIndex(payloadViewIndex) { }
	};

	struct	SPopcornFXPayloadValue
	{
		union
		{
			bool	m_ValueBool[4];
			uint32	m_ValueInt[4];
			float	m_ValueFloat[4];
		};
	};

	struct	SPopcornFXPayload
	{
		PopcornFX::CStringId						m_PayloadName;
		EPopcornFXPayloadType::Type					m_PayloadType;

		PopcornFX::TArray<SPopcornFXPayloadValue>	m_Values;

		SPopcornFXPayload(const PopcornFX::CStringId &payloadName, const EPopcornFXPayloadType::Type payloadType);
	};

	struct	SPopcornFXPayloadView
	{
		const PopcornFX::CParticleMedium		*m_PayloadMedium;
		u32										m_EventID;

		PopcornFX::TArray<SPopcornFXPayload>	m_Payloads;

		u32										m_CurrentParticle;
		float									m_KillTimer;

		SPopcornFXPayloadView();
		SPopcornFXPayloadView(const PopcornFX::CParticleMedium *medium, u32 eventID);

		PK_FORCEINLINE bool						Valid() const { return m_PayloadMedium != null; }
		static SPopcornFXPayloadView			Invalid; // needed by TChunkedSlotArray
	};

	bool	RegisterEventListener(
		UPopcornFXEffect* particleEffect,
		PopcornFX::CEffectID effectID,
		const PopcornFX::CStringId &eventNameID,
		class FPopcornFXRaiseEventSignature &callback);

	void	UnregisterEventListener(
		UPopcornFXEffect* particleEffect,
		PopcornFX::CEffectID effectID,
		const PopcornFX::CStringId &eventNameID,
		class FPopcornFXRaiseEventSignature &callback);

	void	UnregisterAllEventsListeners(
		UPopcornFXEffect* particleEffect,
		PopcornFX::CEffectID effectID);

	bool	GetPayloadValue(
		const FName &payloadName,
		EPopcornFXPayloadType::Type expectedPayloadType,
		void *outValue) const;

public:
	void	BroadcastEvent(
		PopcornFX::Threads::SThreadContext							*threadCtx,
		PopcornFX::CParticleMedium									*parentMedium,
		u32															eventID,
		PopcornFX::CStringId										eventName,
		u32															count,
		const PopcornFX::SUpdateTimeArgs							&timeArgs,
		const PopcornFX::TMemoryView<const float>					&spawnDtToEnd,
		const PopcornFX::TMemoryView<const PopcornFX::CEffectID>	&effectIDs,
		const PopcornFX::SPayloadView								&payloadView);

private:
	void	ClearPendingEvents_NoLock();
	void	_Clear_Events();
	void	_PostUpdate_Events();

private:
	template <typename _OutType>
	void	FillPayload(const PopcornFX::SPayloadElementView &srcPayloadElementData, SPopcornFXPayload &dstPayload);
	void	FillPayloadBool(const PopcornFX::SPayloadElementView &srcPayloadElementData, SPopcornFXPayload &dstPayload, u32 dim);

	void	RetrievePayloadElements(const PopcornFX::SPayloadView &srcPayloadView, SPopcornFXPayloadView &dstPayloadView);

private:
	SPopcornFXPayloadView									*m_CurrentPayloadView;
	PopcornFX::Threads::CCriticalSection					m_RaiseEventLock;
	PopcornFX::TArray<SPopcornFXEventListener>				m_EventListeners;
	PopcornFX::TChunkedSlotArray<SPopcornFXPayloadView>		m_PayloadViews;
	PopcornFX::TArray<SPopcornFXEventListenerAssoc>			m_PendingEventAssocs;
};
