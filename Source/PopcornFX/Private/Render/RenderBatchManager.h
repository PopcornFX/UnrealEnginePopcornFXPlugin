//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXSDK.h"
#include "RenderPolicies.h"
#include "RenderTypesPolicies.h"
#include "PopcornFXSettings.h"
#include "Materials/MaterialInterface.h"

#include <pk_particles/include/ps_mediums.h>
#include <pk_render_helpers/include/frame_collector/rh_frame_collector.h>

#if POPCORNFX_RENDER_DEBUG
	namespace EHeavyDebugModeMask
	{
		enum Type
		{
			Particles	= (1U << 0),
			Mediums		= (1U << 1),
			Pages		= (1U << 2),
		};
	}
#endif // POPCORNFX_RENDER_DEBUG

//#define	POPCORNFX_ENABLE_POOL_STATS

class	CParticleScene;
class	UMaterialInterface;

//----------------------------------------------------------------------------

class	CFrameCollector : public PopcornFX::TFrameCollector<CPopcornFXRenderTypes>
{
public:
	CFrameCollector() : m_Views(null) { }

	void			ReleaseRenderedFrameIFP();

public:
	PopcornFX::CRendererSubView	*m_Views = null;
	bool						m_DisableShadowCulling = false;
	u32							m_LastFrameDrawCalledCount = 0;

private:
	virtual bool	LateCull(const PopcornFX::CAABB &bbox) const override;
};

//----------------------------------------------------------------------------

class	CRenderBatchManager : public PopcornFX::CRefCountedObject
{
public:
	CRenderBatchManager();
	~CRenderBatchManager();

	void	Setup(const CParticleScene *scene, PopcornFX::CParticleMediumCollection *collection
#if (PK_HAS_GPU != 0)
	, uint32 API
#endif // (PK_HAS_GPU != 0)
	);

	void	Clear();
	void	Clean();

#if POPCORNFX_RENDER_DEBUG
	void	GameThread_SetDebugMode(uint32 debugMode) { m_DebugDrawMode = debugMode; }
	void	DrawHeavyDebug(const FPopcornFXSceneProxy *sceneProxy, FPrimitiveDrawInterface *PDI, const FSceneView *view, uint32 debugModeMask);
#endif // POPCORNFX_RENDER_DEBUG

	void	GameThread_PreUpdate(const FPopcornFXRenderSettings &renderSettings);
	void	GameThread_EndUpdate(PopcornFX::CRendererSubView &updateView, UWorld *world);
	void	ConcurrentThread_SendRenderDynamicData();
	void	RenderThread_DrawCalls(PopcornFX::CRendererSubView &view);

	void										AddCollectedUsedMaterial(UMaterialInterface *materialInstance);
	bool										ShouldMarkRenderStateDirty() const;
	u32											LastUsedMaterialCount() const { return m_LastCollectedUsedMaterials.Num(); }
	UMaterialInterface							*GetLastUsedMaterial(u32 matIndex) const;
#if RHI_RAYTRACING
	const SCollectedDrawCalls					&CollectedDrawCalls() const { return m_CollectedDrawCalls; }
#endif // RHI_RAYTRACING

#if WITH_EDITOR
	bool										RenderThread_CollectedForRendering(const UMaterialInterface *material) const { return m_RenderThread_CollectedUsedMaterials.Contains(material); }
#endif // WITH_EDITOR
	PopcornFX::Drawers::EBillboardingLocation	RenderThread_BillboardingLocation() const { return m_RenderThread_BillboardingLocation; }

	void										GatherSimpleLights(const FSceneViewFamily &viewFamily, FSimpleLightArray &outParticleLights) const;

	ERHIFeatureLevel::Type						GetFeatureLevel() const { return m_CurrentFeatureLevel; }
public:
	bool						ForceLightsTranslucent() { return m_ForceLightsTranslucent; }

	CVertexBufferPool			&VBPool() { return *m_VertexBufferPool; }
	CIndexBufferPool			&IBPool() { return *m_IndexBufferPool; }
	const CVertexBufferPool		&VBPool() const { return *m_VertexBufferPool; }
	const CIndexBufferPool		&IBPool() const { return *m_IndexBufferPool; }

	CVertexBufferPool			&VBPool_VertexBB() { return *m_VertexBufferPool_VertexBB; }
	const CVertexBufferPool		&VBPool_VertexBB() const { return *m_VertexBufferPool_VertexBB; }

	CVertexBufferPool			&VBPool_GPU() { return *m_VertexBufferPool_GPU; }
	CIndexBufferPool			&IBPool_GPU() { return *m_IndexBufferPool_GPU; }
	const CVertexBufferPool		&VBPool_GPU() const { return *m_VertexBufferPool_GPU; }
	const CIndexBufferPool		&IBPool_GPU() const { return *m_IndexBufferPool_GPU; }

private:
	void	GarbageBufferPools();

private:
	CFrameCollector							m_FrameCollector_Render;
	CFrameCollector							m_FrameCollector_Update;
	CRenderDataFactory						m_RenderBatchFactory;
	SRenderContext							m_RenderThreadRenderContext;
	SRenderContext							m_UpdateThreadRenderContext;
	SCollectedDrawCalls						m_CollectedDrawCalls;

	ERHIFeatureLevel::Type					m_CurrentFeatureLevel;

	// Keep around
	const CParticleScene					*m_ParticleScene;
	PopcornFX::CParticleMediumCollection	*m_ParticleMediumCollection;

private:
	bool											m_LastCollectedMaterialsChanged;
	u32												m_CollectedUsedMaterialCount;
	::TArray<TWeakObjectPtr<UMaterialInterface> >	m_LastCollectedUsedMaterials;
#if WITH_EDITOR
	::TArray<TWeakObjectPtr<UMaterialInterface> >	m_RenderThread_CollectedUsedMaterials; // For safety lookup
#endif // WITH_EDITOR

#if POPCORNFX_RENDER_DEBUG
	uint32		m_DebugDrawMode;
	float		m_DebugBoundsLinesThickness;
	float		m_DebugParticlePointSize;
#endif // POPCORNFX_RENDER_DEBUG
	bool		m_ForceLightsTranslucent;

private:
	float										m_LastBufferGCTime;

	PopcornFX::EDrawCallSortMethod				m_DCSortMethod;

	bool										m_StatelessCollect;

	PopcornFX::Drawers::EBillboardingLocation	m_BillboardingLocation;
	PopcornFX::Drawers::EBillboardingLocation	m_RenderThread_BillboardingLocation;

	PopcornFX::CTimer							m_RenderTimer;
	CVertexBufferPool							*m_VertexBufferPool;
	CVertexBufferPool							*m_VertexBufferPool_VertexBB;
	CVertexBufferPool							*m_VertexBufferPool_GPU;
	CIndexBufferPool							*m_IndexBufferPool;
	CIndexBufferPool							*m_IndexBufferPool_GPU;

#ifdef	POPCORNFX_ENABLE_POOL_STATS
	PopcornFX::SBufferPool_Stats				*m_VertexBufferPoolStats;
#endif // POPCORNFX_ENABLE_POOL_STATS
};
PK_DECLARE_REFPTRCLASS(RenderBatchManager);
