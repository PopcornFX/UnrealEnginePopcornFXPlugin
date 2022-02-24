//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "RenderBatchManager.h"
#include "RenderingThread.h"
#include "PopcornFXStats.h"
#include "PopcornFXPlugin.h"

#include "Materials/MaterialInterface.h"
#include "Render/RendererSubView.h" // Might change

#if POPCORNFX_RENDER_DEBUG
#	include "PopcornFXSceneComponent.h"
#	include "World/PopcornFXSceneProxy.h"
#	include "SceneManagement.h"
#	include "PopcornFXPlugin.h"
#endif // POPCORNFX_RENDER_DEBUG

DEFINE_LOG_CATEGORY_STATIC(LogRenderBatchManager, Log, All);
//----------------------------------------------------------------------------

bool	CFrameCollector::LateCull(const PopcornFX::CAABB &bbox) const
{
	PK_NAMEDSCOPEDPROFILE("CFrameCollector::Cull Page (RenderThread)");

	if (m_Views == null ||
		!bbox.IsFinite() ||
		!bbox.Valid())
		return false;

#if RHI_RAYTRACING
	// TODO: Nothing is culled here, do we really need to submit the entire particle scene? How to cull properly?
	if (m_Views->RenderPass() == CRendererSubView::RenderPass_RT_AccelStructs)
		return false;
#endif

	PK_ASSERT(m_Views->RenderPass() == CRendererSubView::RenderPass_Main ||
		m_Views->RenderPass() == CRendererSubView::RenderPass_Shadow);

	const bool	mainPass = m_Views->RenderPass() == CRendererSubView::RenderPass_Main;
	if (!mainPass)
	{
		// If we are here, it means this cull test is for a shadow casting renderer
		if (m_DisableShadowCulling)
			return false;
	}

	const CFloat3		origin = bbox.Center();
	const CFloat3		extent = bbox.Extent();
	const float			scale = FPopcornFXPlugin::GlobalScale();
	const FVector		ueOrigin = _Reinterpret<FVector>(origin) * scale;
	const FVector		ueExtent = _Reinterpret<FVector>(extent) * scale;
	const auto			&sceneViews = m_Views->SceneViews();
	for (u32 viewi = 0; viewi < sceneViews.Count(); ++viewi)
	{
		if (!sceneViews[viewi].m_ToRender)
			continue;
		const FSceneView		*view = sceneViews[viewi].m_SceneView;
		bool					fullyContained;

		if (mainPass)
		{
			if (view->ViewFrustum.IntersectBox(ueOrigin, ueExtent, fullyContained))
				return false;
		}
		else
		{
			PK_ASSERT(view->GetDynamicMeshElementsShadowCullFrustum());
			if (view->GetDynamicMeshElementsShadowCullFrustum()->IntersectBox(ueOrigin + view->GetPreShadowTranslation(), ueExtent, fullyContained))
				return false;
		}
	}

	INC_DWORD_STAT_BY(STAT_PopcornFX_CulledDrawReqCount, 1);
	return true;
}

//----------------------------------------------------------------------------

void	CFrameCollector::ReleaseRenderedFrameIFP()
{
	PK_NAMEDSCOPEDPROFILE_C("CFrameCollector::ReleaseRenderedFrameIFP", POPCORNFX_UE_PROFILER_COLOR);
	// Optim:
	// Try and Release the RenderThread's collected frame
	// only if we think the RenderThread wont use it anymore

	PopcornFX::SParticleCollectedFrameToRender	*toRelease = null;
	{
		PK_NAMEDSCOPEDPROFILE_C("CFrameCollector::ReleaseRenderedFrameIFP - Try capture frame", POPCORNFX_UE_PROFILER_COLOR);
		if (m_CollectedLock.TryLock())
		{
			const u32	expectedDrawCalledCount = PopcornFX::PKMax(1U, m_LastFrameDrawCalledCount);
			if (m_Collected != null &&
				m_Collected->m_RenderedCount >= expectedDrawCalledCount)
			{
				if (m_Collected->m_Built) // if actually rendered
					m_LastFrameDrawCalledCount = m_Collected->m_RenderedCount;
				toRelease = m_Collected;
				m_Collected = null;
			}
			m_CollectedLock.Unlock();
		}
		else
		{
			// render thread is using it, dont touch it
		}
	}

	{
		PK_NAMEDSCOPEDPROFILE_C("CFrameCollector::ReleaseRenderedFrameIFP - Release frame", POPCORNFX_UE_PROFILER_COLOR);
		if (toRelease != null)
			m_FramePool->SafeReleaseFrame(toRelease);
	}

	// Updating this stat at pre-update
	// Because this is where if there is more than one, they will most probably be copied
	// INC_DWORD_STAT_BY(STAT_PopcornFX_ActiveCollectedFrame, m_FramePool->m_StatActiveCollectedFrame);
}

//----------------------------------------------------------------------------

CRenderBatchManager::CRenderBatchManager()
:	m_CurrentFeatureLevel(GMaxRHIFeatureLevel)
,	m_ParticleScene(null)
,	m_ParticleMediumCollection(null)
#if POPCORNFX_RENDER_DEBUG
,	m_DebugDrawMode(0)
#endif // POPCORNFX_RENDER_DEBUG
,	m_DCSortMethod(PopcornFX::EDrawCallSortMethod::Sort_DrawCalls)
,	m_StatelessCollect(false)
,	m_BillboardingLocation(PopcornFX::Drawers::BillboardingLocation_CPU)
,	m_RenderThread_BillboardingLocation(PopcornFX::Drawers::BillboardingLocation_CPU)
{
	m_RenderTimer.Start();
	m_VertexBufferPool = new CVertexBufferPool();
	m_IndexBufferPool = new CIndexBufferPool();

	m_VertexBufferPool_GPU = new CVertexBufferPool();
	m_VertexBufferPool_GPU->m_BuffersUsedAsUAV = true;
	m_VertexBufferPool_GPU->m_BuffersUsedAsSRV = true;
	m_VertexBufferPool_GPU->m_ByteAddressBuffers = true;
	m_IndexBufferPool_GPU = new CIndexBufferPool();
	m_IndexBufferPool_GPU->m_BuffersUsedAsUAV = true;
	m_IndexBufferPool_GPU->m_BuffersUsedAsSRV = true;
	m_IndexBufferPool_GPU->m_ByteAddressBuffers = true;

	m_VertexBufferPool_VertexBB = new CVertexBufferPool();
	m_VertexBufferPool_VertexBB->m_BuffersUsedAsSRV = true;

#ifdef	POPCORNFX_ENABLE_POOL_STATS
	m_VertexBufferPoolStats = new PopcornFX::SBufferPool_Stats();
#endif // POPCORNFX_ENABLE_POOL_STATS

	m_RenderBatchFactory.m_RenderBatchManager = this;
}

//----------------------------------------------------------------------------

CRenderBatchManager::~CRenderBatchManager()
{
	PK_ASSERT(m_ParticleScene == null);
	PK_ASSERT(m_ParticleMediumCollection == null);

	// Release buffers needs to be in Rendering Thread
	ENQUEUE_RENDER_COMMAND(ReleaseCommand)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			delete m_VertexBufferPool;
			delete m_IndexBufferPool;
			delete m_VertexBufferPool_VertexBB;
			delete m_VertexBufferPool_GPU;
			delete m_IndexBufferPool_GPU;
		});
	m_VertexBufferPool = null;
	m_IndexBufferPool = null;
	m_VertexBufferPool_VertexBB = null;
	m_VertexBufferPool_GPU = null;
	m_IndexBufferPool_GPU = null;

	FlushRenderingCommands(); // so we can safely release frames

#ifdef	POPCORNFX_ENABLE_POOL_STATS
	delete m_VertexBufferPoolStats;
	m_VertexBufferPoolStats = null;
#endif // POPCORNFX_ENABLE_POOL_STATS
}

//----------------------------------------------------------------------------

void	CRenderBatchManager::Clean()
{
	// The render thread frame collector cannot unmap/clear VBuffers on the update thread
	ENQUEUE_RENDER_COMMAND(DestroyBatchesCommand)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			m_FrameCollector_Render.DestroyBillboardingBatches();
		});

	FlushRenderingCommands(); // so we can safely release frames

	if (m_ParticleMediumCollection != null)
	{
		m_FrameCollector_Render.UpdateThread_UninstallFromMediumCollection(m_ParticleMediumCollection);
		m_FrameCollector_Update.UpdateThread_UninstallFromMediumCollection(m_ParticleMediumCollection);

		m_FrameCollector_Render.UpdateThread_Destroy();

		m_FrameCollector_Update.DestroyBillboardingBatches();
		m_FrameCollector_Update.UpdateThread_Destroy();
	}

	m_ParticleScene = null;
	m_ParticleMediumCollection = null;
}

//----------------------------------------------------------------------------

void	CRenderBatchManager::Clear()
{
	PK_ASSERT(m_ParticleScene != null);
	PK_ASSERT(m_ParticleMediumCollection != null);
}

//----------------------------------------------------------------------------

void	CRenderBatchManager::Setup(const CParticleScene *scene, PopcornFX::CParticleMediumCollection *collection
#if (PK_HAS_GPU != 0)
, uint32 API
#endif // (PK_HAS_GPU != 0)
)
{
	PK_ASSERT(collection != null);
	PK_ASSERT(m_ParticleMediumCollection == null);

	const u32	enabledRenderers = (1U << PopcornFX::ERendererClass::Renderer_Billboard) |
								   (1U << PopcornFX::ERendererClass::Renderer_Ribbon) |
								   (1U << PopcornFX::ERendererClass::Renderer_Mesh) |
								   (1U << PopcornFX::ERendererClass::Renderer_Triangle) |
								   (1U << PopcornFX::ERendererClass::Renderer_Light);
	CFrameCollector::SFrameCollectorInit	init_RenderThread(&m_RenderBatchFactory, enabledRenderers, false /*releaseOnCull*/, false /*StatelessCollecting*/);
	m_FrameCollector_Render.UpdateThread_Initialize(init_RenderThread);
	m_FrameCollector_Render.UpdateThread_InstallToMediumCollection(collection);

	CFrameCollector::SFrameCollectorInit	init_GameThread(&m_RenderBatchFactory, (1U << PopcornFX::ERendererClass::Renderer_Sound), true /*releaseOnCull*/, false /*StatelessCollecting*/);
	m_FrameCollector_Update.UpdateThread_Initialize(init_GameThread);
	m_FrameCollector_Update.UpdateThread_InstallToMediumCollection(collection);

	m_RenderThreadRenderContext.m_RenderBatchManager = this;
	m_UpdateThreadRenderContext.m_RenderBatchManager = this;

#if (PK_HAS_GPU != 0)
	m_RenderThreadRenderContext.m_API = static_cast<SRenderContext::ERHIAPI>(API);
	m_UpdateThreadRenderContext.m_API = static_cast<SRenderContext::ERHIAPI>(API);
#endif // (PK_HAS_GPU != 0)

	m_ParticleScene = scene;
	m_ParticleMediumCollection = collection;
}

//----------------------------------------------------------------------------

UMaterialInterface	*CRenderBatchManager::GetLastUsedMaterial(u32 matIndex) const
{
	PK_ASSERT(matIndex < (u32)m_LastCollectedUsedMaterials.Num());

	return m_LastCollectedUsedMaterials[matIndex].Get();
}

//----------------------------------------------------------------------------

void	CRenderBatchManager::GatherSimpleLights(const FSceneViewFamily &viewFamily, FSimpleLightArray &outParticleLights) const
{
	const int32		posCount = m_CollectedDrawCalls.m_LightPositions.Count();
	const int32		dataCount = m_CollectedDrawCalls.m_LightDatas.Count();
	if (posCount == 0 || dataCount == 0)
		return;

	const int32		startPos = outParticleLights.PerViewData.Num();
	const int32		startData = outParticleLights.InstanceData.Num();
	outParticleLights.PerViewData.SetNumUninitialized(startPos + posCount);
	outParticleLights.InstanceData.SetNumUninitialized(startData + dataCount);

	if (PK_VERIFY(outParticleLights.PerViewData.Num() == startPos + posCount) &&
		PK_VERIFY(outParticleLights.InstanceData.Num() == startData + dataCount))
	{
		PopcornFX::Mem::CopyT(&(outParticleLights.PerViewData[startPos]), &(m_CollectedDrawCalls.m_LightPositions[0]), posCount);
		PopcornFX::Mem::CopyT(&(outParticleLights.InstanceData[startData]), &(m_CollectedDrawCalls.m_LightDatas[0]), dataCount);
	}
}

//----------------------------------------------------------------------------

bool	CRenderBatchManager::ShouldMarkRenderStateDirty() const
{
	return m_LastCollectedMaterialsChanged;
}

//----------------------------------------------------------------------------

void	CRenderBatchManager::AddCollectedUsedMaterial(UMaterialInterface *materialInstance)
{
	const u32	currentId = m_CollectedUsedMaterialCount++;
	bool			changedAndPushBack = false;
	if (m_LastCollectedMaterialsChanged)
		changedAndPushBack = true;
	else
	{
		if (currentId >= u32(m_LastCollectedUsedMaterials.Num()))
			changedAndPushBack = true;
		else if (m_LastCollectedUsedMaterials[currentId] != materialInstance)
		{
			m_LastCollectedUsedMaterials.SetNum(currentId);
			changedAndPushBack = true;
		}
	}
	if (changedAndPushBack)
	{
		m_LastCollectedMaterialsChanged = true;
		m_LastCollectedUsedMaterials.Push(materialInstance);
	}
}

//----------------------------------------------------------------------------

void	CRenderBatchManager::GameThread_PreUpdate(const FPopcornFXRenderSettings &renderSettings)
{
	PK_NAMEDSCOPEDPROFILE_C("CRenderBatchManager::GameThread_PreUpdate", POPCORNFX_UE_PROFILER_COLOR);

	m_LastCollectedMaterialsChanged = false;
	m_CollectedUsedMaterialCount = 0;

	if (renderSettings.bEnableEarlyFrameRelease)
		m_FrameCollector_Render.ReleaseRenderedFrameIFP();

#if WITH_EDITOR
	m_StatelessCollect = !renderSettings.bDisableStatelessCollecting;
	switch (renderSettings.DrawCallSortMethod)
	{
	case	EPopcornFXDrawCallSortMethod::None:
		m_DCSortMethod = PopcornFX::EDrawCallSortMethod::Sort_None;
		break;
	case	EPopcornFXDrawCallSortMethod::PerDrawCalls:
		m_DCSortMethod = PopcornFX::EDrawCallSortMethod::Sort_DrawCalls;
		break;
	case	EPopcornFXDrawCallSortMethod::PerSlicedDrawCalls:
		m_DCSortMethod = PopcornFX::EDrawCallSortMethod::Sort_Slices;
		break;
	case	EPopcornFXDrawCallSortMethod::PerPageDrawCalls:
	default:
		m_DCSortMethod = PopcornFX::EDrawCallSortMethod::Sort_DrawCalls;
		break;
	}
	switch (renderSettings.BillboardingLocation)
	{
	case	EPopcornFXBillboardingLocation::CPU:
		m_BillboardingLocation = PopcornFX::Drawers::BillboardingLocation_CPU;
		break;
	case	EPopcornFXBillboardingLocation::GPU:
		m_BillboardingLocation = PopcornFX::Drawers::BillboardingLocation_VertexShader;
		break;
	default:
		PK_ASSERT_NOT_REACHED();
		break;
	}

	m_FrameCollector_Update.SetDrawCallsSortMethod(m_DCSortMethod);
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------------

void	CRenderBatchManager::GameThread_EndUpdate(PopcornFX::CRendererSubView &updateView, UWorld *world)
{
	PK_NAMEDSCOPEDPROFILE_C("CRenderBatchManager::GameThread_EndUpdate", POPCORNFX_UE_PROFILER_COLOR);

	{
		// Billboards/Ribbons/Meshes/Lights
		// Lights could be gathered in the update thread pass
		PK_NAMEDSCOPEDPROFILE_C("CRenderBatchManager::GameThread_EndUpdate - Collect render thread frame", POPCORNFX_UE_PROFILER_COLOR);
		if (m_FrameCollector_Render.UpdateThread_BeginCollectFrame())
		{
			m_FrameCollector_Render.UpdateThread_CollectFrame(m_ParticleMediumCollection);

			// Note: used for VerifyUsedMaterials, so this cannot be done in SendRenderDynamicData because too late.
			if (m_LastCollectedUsedMaterials.Num() != m_CollectedUsedMaterialCount)
			{
				m_LastCollectedMaterialsChanged = true;
				m_LastCollectedUsedMaterials.SetNum(m_CollectedUsedMaterialCount, false);
			}
		}
	}
	{
		// Sounds
		PK_NAMEDSCOPEDPROFILE_C("CRenderBatchManager::GameThread_EndUpdate - Collect update thread frame", POPCORNFX_UE_PROFILER_COLOR);
		if (m_FrameCollector_Update.UpdateThread_BeginCollectFrame())
			m_FrameCollector_Update.UpdateThread_CollectFrame(m_ParticleMediumCollection);
	}

	PopcornFX::SParticleCollectedFrameToRender	*newToRender = m_FrameCollector_Update.UpdateThread_GetLastCollectedFrame();
	if (newToRender != null)
	{
		PK_NAMEDSCOPEDPROFILE_C("CRenderBatchManager::GameThread_EndUpdate - Update thread DCs", POPCORNFX_UE_PROFILER_COLOR);

		m_FrameCollector_Update.DestroyBillboardingBatches();
		m_FrameCollector_Update.BuildNewFrame(newToRender);

		m_UpdateThreadRenderContext.m_RendererSubView = &updateView;
		m_UpdateThreadRenderContext.SetWorld(world);

		m_CurrentFeatureLevel = world->FeatureLevel;

		PopcornFX::TStaticArray<PopcornFX::TSceneView<CBBView>, 1>	sceneViews;

		sceneViews[0].m_InvViewMatrix = CFloat4x4::IDENTITY;
		TMemoryView<PopcornFX::TSceneView<CBBView> >	views(sceneViews.Data(), sceneViews.Count());

		// TODO: Views (would be necessary for doppler), m_CollectedDrawCalls
		if (m_FrameCollector_Update.BeginCollectingDrawCalls(m_UpdateThreadRenderContext, views))
			m_FrameCollector_Update.EndCollectingDrawCalls(m_UpdateThreadRenderContext, m_CollectedDrawCalls, true);
	}
}

//----------------------------------------------------------------------------

void	CRenderBatchManager::ConcurrentThread_SendRenderDynamicData()
{
	PK_NAMEDSCOPEDPROFILE_C("CParticleRenderManager::ConcurrentThread_SendRenderDynamicData", POPCORNFX_UE_PROFILER_COLOR);

	PopcornFX::SParticleCollectedFrameToRender	*newToRender = m_FrameCollector_Render.UpdateThread_GetLastCollectedFrame();
#if WITH_EDITOR
	auto			collectedMaterials = m_LastCollectedUsedMaterials;
#endif // WITH_EDITOR

#if POPCORNFX_RENDER_DEBUG
	const float		debugParticlePointSize = FPopcornFXPlugin::Get().Settings()->DebugParticlePointSize;
	const float		debugBoundsLinesThickness = FPopcornFXPlugin::Get().Settings()->DebugBoundsLinesThickness;
#endif // POPCORNFX_RENDER_DEBUG
	const bool		forceLightsTranslucent = FPopcornFXPlugin::Get().Settings()->RenderSettings.bForceLightsLitTranslucent;

	// Those could/should be stripped from non-editor builds, having this modified at runtime is editor/debug only
	const PopcornFX::EDrawCallSortMethod				dcSortMethod = m_DCSortMethod;
	const PopcornFX::Drawers::EBillboardingLocation		bbLocation = m_BillboardingLocation;
	const bool											statelessCollect = m_StatelessCollect;

	// /!\ ConcurrentThread_SendRenderDynamicData cannot be called while UpdateThread_Endupdate() gets called
	if (newToRender != null)
	{
		// Always set to true right now
		ENQUEUE_RENDER_COMMAND(PopcornFXRenderBatchManager_SendRenderDynamicData)(
			[this, newToRender, dcSortMethod, bbLocation, statelessCollect
#if WITH_EDITOR
			, collectedMaterials
#endif // WITH_EDITOR
#if POPCORNFX_RENDER_DEBUG
			, debugParticlePointSize, debugBoundsLinesThickness
#endif // POPCORNFX_RENDER_DEBUG
			, forceLightsTranslucent
			](FRHICommandListImmediate &RHICmdList)
		{
			PK_NAMEDSCOPEDPROFILE_C("CParticleRenderManager::Pop Collected Frame", POPCORNFX_UE_PROFILER_COLOR);

			m_FrameCollector_Render.SetDrawCallsSortMethod(dcSortMethod);
			m_FrameCollector_Render.SetStatelessCollect(statelessCollect);

			m_RenderThread_BillboardingLocation = bbLocation;

#if WITH_EDITOR
			m_RenderThread_CollectedUsedMaterials = collectedMaterials;
#endif // WITH_EDITOR
#if POPCORNFX_RENDER_DEBUG
			m_DebugParticlePointSize = debugParticlePointSize;
			m_DebugBoundsLinesThickness = debugBoundsLinesThickness;
#endif // POPCORNFX_RENDER_DEBUG
			m_ForceLightsTranslucent = forceLightsTranslucent;

			PK_ASSERT(IsInRenderingThread());
			m_CollectedDrawCalls.Clear();

			{
				// Don't do this, find proper way to clean batches
				//m_FrameCollector_Render.DestroyBillboardingBatches();
			}

			PopcornFX::SParticleCollectedFrameToRender	*previousFrame = m_FrameCollector_Render.BuildNewFrame(newToRender, false);
			if (previousFrame != null)
			{
				m_FrameCollector_Render.m_LastFrameDrawCalledCount = previousFrame->m_RenderedCount;
				m_FrameCollector_Render.ReleaseFrame(previousFrame);
			}
		});
	}
}

//----------------------------------------------------------------------------

void	CRenderBatchManager::GarbageBufferPools()
{
	PK_NAMEDSCOPEDPROFILE_C("CRenderBatchManager::GarbageBufferPools", POPCORNFX_UE_PROFILER_COLOR);

	float			currentTime = m_RenderTimer.Read();
	float			gcElapsedTime = currentTime - m_LastBufferGCTime;

	PK_ASSERT(m_VertexBufferPool != null);
	PK_ASSERT(m_IndexBufferPool != null);
	{
		m_VertexBufferPool->FrameTick();
		m_IndexBufferPool->FrameTick();
		m_VertexBufferPool_VertexBB->FrameTick();
		m_VertexBufferPool_GPU->FrameTick();
		m_IndexBufferPool_GPU->FrameTick();
	}

#ifdef	POPCORNFX_ENABLE_POOL_STATS
	{
		m_VertexBufferPool->AccumStats(m_VertexBufferPoolStats);
	}
#endif

	if (gcElapsedTime > 2.f)
	{
		m_LastBufferGCTime = currentTime;

		SCOPE_CYCLE_COUNTER(STAT_PopcornFX_GCPoolsTime);

		m_VertexBufferPool->GarbageCollect();
		m_IndexBufferPool->GarbageCollect();
		m_VertexBufferPool_VertexBB->GarbageCollect();
		m_VertexBufferPool_GPU->GarbageCollect();
		m_IndexBufferPool_GPU->GarbageCollect();

#ifdef	POPCORNFX_ENABLE_POOL_STATS
		m_VertexBufferPoolStats->LogPrettyStats("VB");
		m_VertexBufferPoolStats->Clear();
#endif
	} // GC pools
}

//----------------------------------------------------------------------------

void	CRenderBatchManager::RenderThread_DrawCalls(PopcornFX::CRendererSubView &view)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchManager::RenderThread_DrawCalls");

	PK_ASSERT(IsInRenderingThread());

	GarbageBufferPools();

	const auto	&bbViews = view.BBViews();
	if (bbViews.Count() == 0)
		return;

	PopcornFX::TArray<PopcornFX::TSceneView<CBBView> >	sceneViews;
	const u32											viewCount = bbViews.Count();
	for (u32 iView = 0; iView < viewCount; ++iView)
	{
		PopcornFX::CGuid	sceneViewId = sceneViews.PushBack();
		if (!PK_VERIFY(sceneViewId != PopcornFX::CGuid::INVALID))
			return;
		PopcornFX::TSceneView<CBBView>	&sceneView = sceneViews[sceneViewId];

		//sceneView.m_UserData.
		sceneView.m_InvViewMatrix = bbViews[iView].m_BillboardingMatrix;
		sceneView.m_NeedsSortedIndices = view.RenderPass() != CRendererSubView::RenderPass_Shadow;
	}

	m_RenderThreadRenderContext.m_RendererSubView = &view;

	TMemoryView<PopcornFX::TSceneView<CBBView> >	views(sceneViews.RawDataPointer(), sceneViews.Count());

	// If any medium has been registered, build their renderer cache on the render thread
	m_RenderBatchFactory.RenderThread_BuildPendingCaches();

	m_FrameCollector_Render.m_Views = &view;
	m_FrameCollector_Render.m_DisableShadowCulling = false; // TODO


#if POPCORNFX_RENDER_DEBUG
	const u32	endCollectingDrawCallsMask = PopcornFX::kEndCollectingDrawCallsMask & ~PopcornFX::EGenerateDrawCallsMask::Mask_UnlockCollectedFrame; // We'll unlock after debug draw
	const u32	postDebugDrawMask = PopcornFX::EGenerateDrawCallsMask::Mask_UnlockCollectedFrame;
	bool		unlockFrame = false;
#else
	const u32	endCollectingDrawCallsMask = PopcornFX::kEndCollectingDrawCallsMask;
#endif

 	if (m_FrameCollector_Render.BeginCollectingDrawCalls(m_RenderThreadRenderContext, views))
	{
#if POPCORNFX_RENDER_DEBUG
		unlockFrame = true;
#endif
		m_FrameCollector_Render.GenerateDrawCalls(&m_RenderThreadRenderContext, null, &m_CollectedDrawCalls, false, endCollectingDrawCallsMask);
	}
	m_FrameCollector_Render.m_Views = null;

#if POPCORNFX_RENDER_DEBUG
	if (view.RenderPass() == CRendererSubView::RenderPass_Main &&
		m_DebugDrawMode != EPopcornFXHeavyDebugMode::None)
	{
		PK_NAMEDSCOPEDPROFILE("DrawHeavyDebug");

		uint32									debugModeMask = 0;
		const EPopcornFXHeavyDebugMode::Type	debugMode = static_cast<EPopcornFXHeavyDebugMode::Type>(m_DebugDrawMode);
		switch (debugMode)
		{
		case EPopcornFXHeavyDebugMode::None: break;
		case EPopcornFXHeavyDebugMode::ParticlePoints: debugModeMask |= EHeavyDebugModeMask::Particles; break;
		case EPopcornFXHeavyDebugMode::MediumsBBox: debugModeMask |= EHeavyDebugModeMask::Mediums; break;
		case EPopcornFXHeavyDebugMode::PagesBBox: debugModeMask |= EHeavyDebugModeMask::Pages; break;
		default:
			PK_ASSERT_NOT_REACHED();
		}
		const FPopcornFXSceneProxy		*sceneProxy = view.SceneProxy();
		for (u32 iView = 0; iView < view.SceneViews().Count(); ++iView)
		{
			if (view.SceneViews()[iView].m_ToRender)
				DrawHeavyDebug(sceneProxy, view.Collector()->GetPDI(iView), view.SceneViews()[iView].m_SceneView, debugModeMask);
		}
	}
	if (unlockFrame)
		m_FrameCollector_Render.GenerateDrawCalls(&m_RenderThreadRenderContext, null, &m_CollectedDrawCalls, false, postDebugDrawMask);
#endif
}

//----------------------------------------------------------------------------

#if POPCORNFX_RENDER_DEBUG

namespace
{
	inline FLinearColor		_MediumColorFromIndex(uint32 index)
	{
		return FLinearColor(FGenericPlatformMath::Frac(float(index) * 0.07777f) * 360.f, 0.9f, 0.9f, 0.8f).HSVToLinearRGB();
	}

	void	_RenderBoundsColor(FPrimitiveDrawInterface* PDI, const FBoxSphereBounds& Bounds, const FLinearColor &color, float boundsLinesThickness)
	{
		const ESceneDepthPriorityGroup	DepthPriority = SDPG_Foreground;
		const FColor					Color = color.ToFColor(false);
		const FBox						Box = Bounds.GetBox();

		FVector	B[2], P, Q;
		int32 i, j;
		B[0] = Box.Min;
		B[1] = Box.Max;
		for (i = 0; i<2; i++) for (j = 0; j<2; j++)
		{
			P.X = B[i].X; Q.X = B[i].X;
			P.Y = B[j].Y; Q.Y = B[j].Y;
			P.Z = B[0].Z; Q.Z = B[1].Z;
			PDI->DrawLine(P, Q, Color, DepthPriority, boundsLinesThickness);

			P.Y = B[i].Y; Q.Y = B[i].Y;
			P.Z = B[j].Z; Q.Z = B[j].Z;
			P.X = B[0].X; Q.X = B[1].X;
			PDI->DrawLine(P, Q, Color, DepthPriority, boundsLinesThickness);

			P.Z = B[i].Z; Q.Z = B[i].Z;
			P.X = B[j].X; Q.X = B[j].X;
			P.Y = B[0].Y; Q.Y = B[1].Y;
			PDI->DrawLine(P, Q, Color, DepthPriority, boundsLinesThickness);
		}
	}

	void	_DrawHeavyDebug(
		const FPopcornFXSceneProxy *sceneProxy,
		FPrimitiveDrawInterface *PDI,
		const FSceneView *view,
		uint32 debugModeMask,
		uint32 globalIndex,
		const PopcornFX::Drawers::SBase_DrawRequest &dr,
		float particlePointsize,
		float boundsLinesThickness)
	{
		if (dr.Empty())
			return;
		const FLinearColor							color = _MediumColorFromIndex(globalIndex);
		const PopcornFX::CParticleStreamToRender	*baseStream = &dr.StreamToRender();

		if (debugModeMask & EHeavyDebugModeMask::Mediums)
			_RenderBoundsColor(PDI, ToUE(baseStream->BBox() * FPopcornFXPlugin::GlobalScale()), color, boundsLinesThickness);

		if ((debugModeMask & (EHeavyDebugModeMask::Particles | EHeavyDebugModeMask::Pages)) == 0)
			return;

		// Draw particles as points for MainMemory streams
		const PopcornFX::CParticleStreamToRender_MainMemory		*stream = dr.StreamToRender_MainMemory();
		if (stream != null)
		{
			const PopcornFX::Drawers::SBase_BillboardingRequest	&br = dr.BaseBillboardingRequest();
			const PopcornFX::CGuid								positionStreamId = br.m_PositionStreamId;
			if (!positionStreamId.Valid())
				return;

			const float					scale = FPopcornFXPlugin::GlobalScale();
			static volatile uint8		depthPrio = SDPG_World;

			for (uint32 pagei = 0; pagei < stream->PageCount(); pagei++)
			{
				const PopcornFX::CParticlePageToRender_MainMemory	*page = stream->Page(pagei);
				PK_ASSERT(page != null);
				if (page->Culled() || page->Empty())
					continue;

				if (debugModeMask & EHeavyDebugModeMask::Pages)
					_RenderBoundsColor(PDI, ToUE(page->BBox() * FPopcornFXPlugin::GlobalScale()), color, boundsLinesThickness);

				if (debugModeMask & EHeavyDebugModeMask::Particles)
				{
					TStridedMemoryView<const CFloat3>	positions = page->StreamForReading<CFloat3>(positionStreamId);
					for (uint32 parti = 0; parti < positions.Count(); ++parti)
						PDI->DrawPoint(ToUE(positions[parti] * scale), color, particlePointsize, depthPrio);
				}
			}
		}
	}
}

void	CRenderBatchManager::DrawHeavyDebug(const FPopcornFXSceneProxy *sceneProxy, FPrimitiveDrawInterface *PDI, const FSceneView *view, uint32 debugModeMask)
{
	const PopcornFX::SParticleCollectedFrameToRender	*frame = m_FrameCollector_Render.RenderedFrame();
	if (frame == null)
		return;

	uint32	globalIndex = 0;
	for (uint32 dri = 0; dri < frame->m_BillboardDrawRequests.Count(); ++dri)
		_DrawHeavyDebug(sceneProxy, PDI, view, debugModeMask, globalIndex++, frame->m_BillboardDrawRequests[dri], m_DebugParticlePointSize, m_DebugBoundsLinesThickness);
	for (uint32 dri = 0; dri < frame->m_RibbonDrawRequests.Count(); ++dri)
		_DrawHeavyDebug(sceneProxy, PDI, view, debugModeMask, globalIndex++, frame->m_RibbonDrawRequests[dri], m_DebugParticlePointSize, m_DebugBoundsLinesThickness);
	for (uint32 dri = 0; dri < frame->m_MeshDrawRequests.Count(); ++dri)
		_DrawHeavyDebug(sceneProxy, PDI, view, debugModeMask, globalIndex++, frame->m_MeshDrawRequests[dri], m_DebugParticlePointSize, m_DebugBoundsLinesThickness);
	for (uint32 dri = 0; dri < frame->m_LightDrawRequests.Count(); ++dri)
		_DrawHeavyDebug(sceneProxy, PDI, view, debugModeMask, globalIndex++, frame->m_LightDrawRequests[dri], m_DebugParticlePointSize, m_DebugBoundsLinesThickness);

	// global bounds
	sceneProxy->RenderBounds(PDI, view->Family->EngineShowFlags, sceneProxy->GetBounds(), true);
}

#endif // POPCORNFX_RENDER_DEBUG
