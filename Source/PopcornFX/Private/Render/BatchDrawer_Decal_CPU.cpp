//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "BatchDrawer_Decal_CPU.h"
#include "RenderBatchManager.h"
#include "MaterialDesc.h"
#include "PopcornFXPlugin.h"
#include "PopcornFXStats.h"
#include "PopcornFXSceneActor.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "Internal/ParticleScene.h"

#include "SceneInterface.h"
#include "Engine/World.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialDomain.h"
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 6)
#include "SceneProxies/DeferredDecalProxy.h"
#else
#include "Components/SceneComponent.h"
#endif

#include <pk_particles/include/Storage/MainMemory/storage_ram.h>
#include <pk_render_helpers/include/render_features/rh_features_basic.h>

//----------------------------------------------------------------------------

CBatchDrawer_Decal_CPUBB::CBatchDrawer_Decal_CPUBB()
{
}

//----------------------------------------------------------------------------

CBatchDrawer_Decal_CPUBB::~CBatchDrawer_Decal_CPUBB()
{
	_ReleaseAllDecals();
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Decal_CPUBB::Setup(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleRenderMedium *owner, const PopcornFX::CFrameCollector *fc, const PopcornFX::CStringId &storageClass)
{
	if (!Super::Setup(renderer, owner, fc, storageClass))
		return false;

	CRendererCache	*matCache = static_cast<CRendererCache*>(renderer->m_RendererCache.Get());
	if (!PK_VERIFY(matCache != null))
		return false;

	const CMaterialDesc_GameThread	&matDesc = matCache->GameThread_Desc();
	UMaterialInstanceConstant		*materialInstance = matDesc.m_RendererMaterial.Get()->GetInstance(0, false);
	if (!PK_VERIFY(IsValid(materialInstance)))
		return false;
	m_WeakMaterial = materialInstance;

	owner->m_OnRenderMediumActiveStateChanged	+= PopcornFX::FastDelegate<void(PopcornFX::CParticleRenderMedium*, bool)>(this, &CBatchDrawer_Decal_CPUBB::_OnRenderMediumActiveStateChanged);
	owner->m_OnRenderMediumDestroyed			+= PopcornFX::FastDelegate<void(PopcornFX::CParticleRenderMedium*)>(this, &CBatchDrawer_Decal_CPUBB::_OnRenderMediumDestroyed);

	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Decal_CPUBB::AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const
{
	if (!Super::AreRenderersCompatible(rendererA, rendererB))
		return false;

	const CRendererCache	*matCacheA = static_cast<const CRendererCache*>(rendererA->m_RendererCache.Get());
	const CRendererCache	*matCacheB = static_cast<const CRendererCache*>(rendererB->m_RendererCache.Get());
	if (matCacheA == null || matCacheB == null)
		return false;

	const CMaterialDesc_GameThread	&gDescA = matCacheA->GameThread_Desc();
	const CMaterialDesc_GameThread	&gDescB = matCacheB->GameThread_Desc();
	PK_ASSERT(gDescA.m_RendererClass == PopcornFX::Renderer_Decal);
	if (gDescA.m_RendererClass != gDescB.m_RendererClass)
		return false;
	if (gDescA.m_RendererMaterial == null || gDescB.m_RendererMaterial == null)
		return false;
	if (gDescA.m_RendererMaterial != gDescB.m_RendererMaterial)
		return false;

	const FPopcornFXSubRendererMaterial	*matA = gDescA.m_RendererMaterial->GetSubMaterial(0);
	const FPopcornFXSubRendererMaterial	*matB = gDescB.m_RendererMaterial->GetSubMaterial(0);
	if (matA == null || matB == null)
		return false;
	if (matA == matB ||
		matA->RenderThread_SameMaterial_Decal(*matB))
		return true;

	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Decal_CPUBB::IsCompatible(UMaterialInterface *material)
{
	UMaterial	*mat = material->GetMaterial();
	if (mat != null)
		return mat->MaterialDomain == MD_DeferredDecal;
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Decal_CPUBB::CanRender(PopcornFX::SRenderContext &ctx) const
{
	PK_ASSERT(DrawPass().m_RendererCaches.First() != null);

	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);
	PK_ASSERT(renderContext.m_RendererSubView != null);

	return renderContext.m_RendererSubView->Pass() == CRendererSubView::UpdatePass_PostUpdate;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Decal_CPUBB::BeginFrame(PopcornFX::SRenderContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Decal_CPUBB::BeginFrame");

	Super::BeginFrame(ctx);

	INC_DWORD_STAT_BY(STAT_PopcornFX_BatchesCount, 1);
}

//----------------------------------------------------------------------------

template<typename T>
void	 CBatchDrawer_Decal_CPUBB::_GetStreamForReading(TStridedMemoryView<const T> &outStream, const PopcornFX::CParticlePageToRender_MainMemory *page, PopcornFX::CGuid streamId, u32 pcount)
{
	outStream = page->StreamForReading<T>(streamId);
	PK_ASSERT(outStream.Count() == pcount);
}

//----------------------------------------------------------------------------

void CBatchDrawer_Decal_CPUBB::_GetDecalStreams(SDecalStreams &outStreams, const PopcornFX::Drawers::SDecal_BillboardingRequest &bbRequest, 
												const PopcornFX::CParticlePageToRender_MainMemory *page, u32 pcount)
{
	// Get enabled particles stream
	const u8	enabledTrue = u8(-1);
	outStreams.enableds = (bbRequest.m_EnabledStreamId.Valid()) ? page->StreamForReading<bool>(bbRequest.m_EnabledStreamId) : TStridedMemoryView<const u8>(&enabledTrue, pcount, 0);

	// Get default streams
	_GetStreamForReading(outStreams.positions, page, bbRequest.m_PositionStreamId, pcount);
	_GetStreamForReading(outStreams.orientations, page, bbRequest.m_OrientationStreamId, pcount);
	if (outStreams.isScaleFloat3)
		_GetStreamForReading(outStreams.scalesF3, page, bbRequest.m_ScaleStreamId, pcount);
	else
		_GetStreamForReading(outStreams.scalesF1, page, bbRequest.m_ScaleStreamId, pcount);

	// Get additional streams
	for (const PopcornFX::Drawers::SDecal_BillboardingRequest::SAdditionalInputInfo &additionnalInput : bbRequest.m_AdditionalInputs)
	{
		const PopcornFX::CStringId	inputName = additionnalInput.m_Name;

		switch (additionnalInput.m_Type)
		{
		case PopcornFX::EBaseTypeID::BaseType_Float4:
			if (inputName == PopcornFX::BasicRendererProperties::SID_Diffuse_Color() || 
				inputName == PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseColor())
				_GetStreamForReading(outStreams.colorsDiffuse, page, additionnalInput.m_StreamId, pcount);

			else if (inputName == PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveColor())
				_GetStreamForReading(outStreams.colorsEmissive, page, additionnalInput.m_StreamId, pcount);
			break;

		case PopcornFX::EBaseTypeID::BaseType_Float3:
			if (inputName == PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveColor())
				_GetStreamForReading(outStreams.colorsEmissiveLegacy, page, additionnalInput.m_StreamId, pcount);
			break;

		case PopcornFX::EBaseTypeID::BaseType_Float:
			if (inputName == PopcornFX::BasicRendererProperties::SID_Atlas_TextureID())
				_GetStreamForReading(outStreams.atlasTextureIDs, page, additionnalInput.m_StreamId, pcount);

			else if (inputName == PopcornFX::BasicRendererProperties::SID_AlphaRemap_Cursor())
				_GetStreamForReading(outStreams.alphaRemapCursors, page, additionnalInput.m_StreamId, pcount);
			break;
		}
	}
}

//----------------------------------------------------------------------------

void CBatchDrawer_Decal_CPUBB::_BuildDecalUpdates(	TArray<FDeferredDecalUpdateParams> &decalUpdates, const SDecalStreams &ds, 
													const UPopcornFXSceneComponent *sceneComp, u32 &activeDecalProxiesCounter, u32 pcount, float globalScale)
{
	for (u32 parti = 0; parti < pcount; ++parti)
	{
		if (!ds.enableds[parti])
			continue;

		// Create Update Parameters
		FDeferredDecalUpdateParams	&updateParams = decalUpdates.AddDefaulted_GetRef();
		if (activeDecalProxiesCounter < m_ActiveDecalProxies.Count())
		{
			updateParams.OperationType = FDeferredDecalUpdateParams::EOperationType::Update;
			updateParams.DecalProxy = m_ActiveDecalProxies[activeDecalProxiesCounter];
		}
		else
		{
			updateParams.OperationType = FDeferredDecalUpdateParams::EOperationType::AddToSceneAndUpdate;
			updateParams.DecalProxy = new FDeferredDecalProxy(sceneComp, m_WeakMaterial.Get());
			m_ActiveDecalProxies.PushBack(updateParams.DecalProxy);
		}
		++activeDecalProxiesCounter;

		// Rotation has offset in Y because decals face up in PK and right in UE by default
		const FQuat		rotOffset	= FQuat::MakeFromEuler({ 0, -90, 0 });
		const FQuat		rotation	= FQuat(ToUE(ds.orientations[parti])) * rotOffset;
		const FVector	position	= FVector(ToUE(ds.positions[parti] * globalScale));
		const FVector	scale		= rotOffset.Inverse() * (ds.isScaleFloat3 ? FVector(ToUE(ds.scalesF3[parti])) : FVector(ds.scalesF1[parti])) * globalScale;
		const CFloat4	diffuse		= !ds.colorsDiffuse.Empty() ? ds.colorsDiffuse[parti] : CFloat4::ZERO;
		const CFloat3	emissive	= !ds.colorsEmissive.Empty() ? ds.colorsEmissive[parti].xyz() * ds.colorsEmissive[parti].w() // Bake emissive alpha into RGB values
									: (!ds.colorsEmissiveLegacy.Empty() ? ds.colorsEmissiveLegacy[parti] : CFloat3::ZERO);
		const float		atlasTextureID		= !ds.atlasTextureIDs.Empty() ? ds.atlasTextureIDs[parti] : 0;
		const float		alphaRemapCursor	= !ds.alphaRemapCursors.Empty() ? ds.alphaRemapCursors[parti] : 0;

		// Pack emissive and alpha remap into half floats
		const FVector2DHalf	emissiveRG				= FVector2DHalf(emissive.x(), emissive.y());
		const FVector2DHalf	emissiveB_AlphaRemap	= FVector2DHalf(emissive.z(), alphaRemapCursor);

		// Pack all additionnal inputs in uints 32
		const u32	packedDiffuse	= ((u8)(255 * diffuse.x()))
									+ ((u8)(255 * diffuse.y()) << 8)
									+ ((u8)(255 * diffuse.z()) << 16)
									+ ((u8)(255 * diffuse.w()) << 24);
		const u32	packedEmissiveRG			= (emissiveRG.X.Encoded & 0xFFFFFFFF) | (emissiveRG.Y.Encoded << 16);
		const u32	packedEmissiveB_AlphaRemap	= (emissiveB_AlphaRemap.X.Encoded & 0xFFFFFFFF) | (emissiveB_AlphaRemap.Y.Encoded << 16);

		// Store all additionnal inputs in an UE color
		FLinearColor colorStored = FLinearColor(FGenericPlatformMath::AsFloat(packedDiffuse),
												FGenericPlatformMath::AsFloat(packedEmissiveRG),
												FGenericPlatformMath::AsFloat(packedEmissiveB_AlphaRemap),
												atlasTextureID);

		// Set decal parameters, and send all additional inputs through DecalColor
		updateParams.Transform = FTransform(rotation, position, scale);
		updateParams.Bounds = FBoxSphereBounds(FSphere(position, scale.GetAbsMax() * 2.0));
		updateParams.AbsSpawnTime = 0;
		updateParams.FadeStartDelay = 0;
		updateParams.FadeDuration = 0;
		updateParams.FadeScreenSize = 0.01f;
		updateParams.DecalColor = colorStored;
		updateParams.SortOrder = 0;
	}
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Decal_CPUBB::_IssueDrawCall_Decal(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Decal");
	
	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);
	const float	globalScale = view->GlobalScale();

	PK_ASSERT(!desc.m_DrawRequests.Empty());
	PK_ASSERT(desc.m_TotalParticleCount > 0);
	const u32	totalParticleCount = desc.m_TotalParticleCount;

	const UWorld	*world = renderContext.GetWorld();
	if (!PK_VERIFY(IsValid(world)))
		return;
	if (m_WeakWorld == null)
		m_WeakWorld = world; // Used to release decals in destructor

	const UPopcornFXSceneComponent	*sceneComp = renderContext.m_RenderBatchManager->ParticleScene().SceneComponent();
	if (!PK_VERIFY(IsValid(sceneComp)))
		return;

	// Generate list of decals to update
	u32	activeDecalProxiesCounter = 0;
	TArray<FDeferredDecalUpdateParams>	decalUpdates;
	decalUpdates.Reserve(totalParticleCount);

	const u32	drCount = desc.m_DrawRequests.Count();
	for (u32 iDr = 0; iDr < drCount; ++iDr)
	{
		const PopcornFX::Drawers::SDecal_DrawRequest			&drawRequest = *static_cast<const PopcornFX::Drawers::SDecal_DrawRequest*>(desc.m_DrawRequests[iDr]);
		const PopcornFX::Drawers::SDecal_BillboardingRequest	&bbRequest = static_cast<const PopcornFX::Drawers::SDecal_BillboardingRequest&>(drawRequest.BaseBillboardingRequest());
		if (drawRequest.StorageClass() != PopcornFX::CParticleStorageManager_MainMemory::DefaultStorageClass())
		{
			PK_ASSERT_NOT_REACHED();
			return;
		}

		const PopcornFX::CParticleStreamToRender_MainMemory	*lockedStream = drawRequest.StreamToRender_MainMemory();
		if (!PK_VERIFY(lockedStream != null)) // Decal particles shouldn't handle GPU streams for now
			return;

		const u32	pageCount = lockedStream->PageCount();
		for (u32 pagei = 0; pagei < pageCount; ++pagei)
		{
			const PopcornFX::CParticlePageToRender_MainMemory	*page = lockedStream->Page(pagei);
			PK_ASSERT(page != null);

			const u32	pcount = (page == null ? 0 : page->InputParticleCount());
			if (pcount == 0)
				continue;

			// Get streams for the current page and create decal updates with per-particle values
			SDecalStreams decalStreams;
			_GetDecalStreams(decalStreams, bbRequest, page, pcount);
			_BuildDecalUpdates(decalUpdates, decalStreams, sceneComp, activeDecalProxiesCounter, pcount, globalScale);
		}
	}

	// Remove any unused decals
	while (m_ActiveDecalProxies.Count() > activeDecalProxiesCounter)
	{
		FDeferredDecalUpdateParams	&UpdateParams = decalUpdates.AddDefaulted_GetRef();
		UpdateParams.OperationType = FDeferredDecalUpdateParams::EOperationType::RemoveFromSceneAndDelete;
		UpdateParams.DecalProxy = m_ActiveDecalProxies.Pop();
	}

	// Send updates to RT
	if (decalUpdates.Num() > 0)
	 	world->Scene->BatchUpdateDecals(MoveTemp(decalUpdates));

	INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsDecalCount, totalParticleCount);
}

//----------------------------------------------------------------------------
#include "Engine/Engine.h"
void CBatchDrawer_Decal_CPUBB::_OnRenderMediumActiveStateChanged(PopcornFX::CParticleRenderMedium *renderMedium, bool active)
{
	if (active)
		return;

	// Sometimes this callback might be executed on a worker thread
	// In that case we request the execution of the method on the game thread
	if (!IsInGameThread())
	{
		CBatchDrawer_Decal_CPUBB	*self = this;
		ExecuteOnGameThread(TEXT("CBatchDrawer_Decal_CPUBB::_ReleaseAllDecals"), [self](){ self->_ReleaseAllDecals(); });
	}

	_ReleaseAllDecals();
}

//----------------------------------------------------------------------------

void CBatchDrawer_Decal_CPUBB::_OnRenderMediumDestroyed(PopcornFX::CParticleRenderMedium *renderMedium)
{
	renderMedium->m_OnRenderMediumActiveStateChanged	-= PopcornFX::FastDelegate<void(PopcornFX::CParticleRenderMedium*, bool)>(this, &CBatchDrawer_Decal_CPUBB::_OnRenderMediumActiveStateChanged);
	renderMedium->m_OnRenderMediumDestroyed				-= PopcornFX::FastDelegate<void(PopcornFX::CParticleRenderMedium*)>(this, &CBatchDrawer_Decal_CPUBB::_OnRenderMediumDestroyed);
}

//----------------------------------------------------------------------------

void CBatchDrawer_Decal_CPUBB::_ReleaseAllDecals()
{
	const u32	activeDecalProxiesCount = m_ActiveDecalProxies.Count();
	if (activeDecalProxiesCount > 0 && m_WeakWorld.IsValid())
	{
		TArray<FDeferredDecalUpdateParams>	decalUpdates;
		decalUpdates.AddDefaulted(activeDecalProxiesCount);
		for (u32 i = 0; i < activeDecalProxiesCount; ++i)
		{
			decalUpdates[i].OperationType = FDeferredDecalUpdateParams::EOperationType::RemoveFromSceneAndDelete;
			decalUpdates[i].DecalProxy = m_ActiveDecalProxies[i];
		}

		m_WeakWorld.Get()->Scene->BatchUpdateDecals(MoveTemp(decalUpdates));
		m_ActiveDecalProxies.Clear();
	}
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Decal_CPUBB::EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SDrawCallDesc &toEmit)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Decal_CPUBB::EmitDrawCall");
	PK_ASSERT(toEmit.m_DrawRequests.First() != null);

	// !Resolve material proxy and interface for first compatible material
	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);
	CRendererCache				*matCache = static_cast<CRendererCache*>(toEmit.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return true;

	_IssueDrawCall_Decal(renderContext, toEmit);
	return true;
}

//----------------------------------------------------------------------------
