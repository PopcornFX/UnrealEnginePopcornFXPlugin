//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "Render/RenderPolicies.h"
#include "Render/MaterialDesc.h"
#include "Render/RenderBatchManager.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "PopcornFXStats.h"

#if WITH_EDITOR
#	include "Editor.h"
#endif

#include "Render/PopcornFXGPUVertexFactory.h"

#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"

// HEADER containing default TParticleBatchInterface implementations
#include <pk_render_helpers/include/batches/rh_billboard_batch.h>
#include <pk_render_helpers/include/batches/rh_ribbon_batch.h>
#include <pk_render_helpers/include/batches/rh_triangle_batch.h>
#include <pk_render_helpers/include/batches/rh_mesh_batch.h>
#include <pk_render_helpers/include/batches/rh_light_batch.h>
#include <pk_render_helpers/include/batches/rh_sound_batch.h>

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXRenderDataFactory, Log, All);

//----------------------------------------------------------------------------

CRenderDataFactory::CRenderDataFactory()
:	m_RenderBatchManager(null)
{
}

//----------------------------------------------------------------------------

PopcornFX::PRendererCacheBase	CRenderDataFactory::UpdateThread_CreateRendererCache(const PopcornFX::PRendererDataBase &renderer, const PopcornFX::CParticleDescriptor *particleDesc)
{
	PK_NAMEDSCOPEDPROFILE("CRenderDataFactory::UpdateThread_CreateRendererCache");

	// We create the renderer cache:
	PRendererCache	rCache = PK_NEW(CRendererCache);
	if (!PK_VERIFY(rCache != null))
		return null;

	if (!rCache->GameThread_ResolveRenderer(renderer, particleDesc))
		return null;
	if (!rCache->GameThread_Setup())
	{
		UE_LOG(LogPopcornFXRenderDataFactory, Warning, TEXT("Couldn't setup renderer cache for rendering")); // TODO Log name
		return null;
	}
	//if (rCache->GameThread_Desc().HasMaterial()) // we need to setup the material for light renderers
	{
		PK_SCOPEDLOCK(m_PendingCachesLock);

		if (!PK_VERIFY(m_PendingCaches.PushBack(rCache).Valid()))
			return null;
	}
	return rCache;
}

//----------------------------------------------------------------------------

void	CRenderDataFactory::RenderThread_BuildPendingCaches()
{
	PK_NAMEDSCOPEDPROFILE("CRenderDataFactory::RenderThread_BuildPendingCaches");
	PK_SCOPEDLOCK(m_PendingCachesLock);

	const u32	matCount = m_PendingCaches.Count();
	for (u32 iMat = 0; iMat < matCount; ++iMat)
	{
		CRendererCache	*rCache = static_cast<CRendererCache*>(m_PendingCaches[iMat].Get());

		if (!rCache->RenderThread_Setup())
		{
			UE_LOG(LogPopcornFXRenderDataFactory, Warning, TEXT("Couldn't build renderer cache")); // TODO Log name
		}
	}
	m_PendingCaches.Clear();
}

//----------------------------------------------------------------------------

void	CRenderDataFactory::UpdateThread_CollectedForRendering(const PopcornFX::PRendererCacheBase &rendererCache)
{
	PK_NAMEDSCOPEDPROFILE("CRenderDataFactory::UpdateThread_CollectedForRendering");

	PK_ASSERT(m_RenderBatchManager != null);

	CRendererCache	*rCache = static_cast<CRendererCache*>(rendererCache.Get());
	if (!PK_VERIFY(rCache != null))
		return;
	if (!rCache->GameThread_Desc().HasMaterial()) // audio, light
		return;

#if WITH_EDITOR
	{
		PK_SCOPEDLOCK(m_PendingCachesLock);

		// Stateless renderer caches, too much editor only actions
		// Auto/manual reimport, resource remove, ..
		// We need to catch that here
		if (!rCache->GameThread_Desc().GameThread_Setup())
			return;
		if (!PK_VERIFY(m_PendingCaches.PushBack(rCache).Valid()))
			return;
	}
#endif

	PK_ASSERT(rCache->GameThread_Desc().MaterialIsValid());
	m_RenderBatchManager->AddCollectedUsedMaterial(rCache->GameThread_Desc().m_RendererMaterial->GetInstance(0, false));
//	m_RenderBatchManager->AddCollectedUsedMaterial();
}

//----------------------------------------------------------------------------

CRenderDataFactory::CBillboardingBatchInterface	*CRenderDataFactory::CreateBillboardingBatch(PopcornFX::ERendererClass rendererType, const PopcornFX::PRendererCacheBase &rendererCache, bool gpuStorage)
{
	INC_DWORD_STAT_BY(STAT_PopcornFX_BatchesCount, 1);

	PopcornFX::Drawers::EBillboardingLocation			bbLocation = m_RenderBatchManager->RenderThread_BillboardingLocation();
	const EShaderPlatform								platform = GetFeatureLevelShaderPlatform(m_RenderBatchManager->GetFeatureLevel());

	bool			hasVolumeMaterial = false;
	bool			hasValidMaterial = true;
	CRendererCache	*matCache = static_cast<CRendererCache*>(rendererCache.Get());
	if (PK_VERIFY(matCache != null))
	{
		CMaterialDesc_GameThread	&matDesc = matCache->GameThread_Desc();
		if (matDesc.HasMaterial() && PK_VERIFY(matDesc.MaterialIsValid()))
		{
			const FPopcornFXSubRendererMaterial	*rendererSubMat = matDesc.m_RendererMaterial->GetSubMaterial(0);
			if (!PK_VERIFY(rendererSubMat != null) ||
				!PK_VERIFY(rendererSubMat->Material != null))
				return null;

			UMaterial	*material = rendererSubMat->Material->GetMaterial();
			if (!PK_VERIFY(material != null))
			{
				// Avoid creating a render batch for no reason
				UE_LOG(LogPopcornFXRenderDataFactory, Warning, TEXT("Couldn't create compatible render policy for material: invalid material."));
				return null;
			}
			hasVolumeMaterial = rendererSubMat->MaterialType == EPopcornFXMaterialType::Billboard_SixWayLightmap ||
								material->MaterialDomain == MD_Volume;

			switch (rendererType)
			{
			case	PopcornFX::Renderer_Billboard:
				hasValidMaterial = hasVolumeMaterial || material->MaterialDomain == MD_Surface;
				break;
			case	PopcornFX::Renderer_Ribbon:
			case	PopcornFX::Renderer_Mesh:
			case	PopcornFX::Renderer_Triangle:
				hasValidMaterial = material->MaterialDomain == MD_Surface;
				break;
			default:
				PK_ASSERT_NOT_REACHED();
				break;
			}
		}
	}

	const bool		vertexBBSupported = FPopcornFXGPUVertexFactory::PlatformIsSupported(platform);

	if (!hasValidMaterial)
	{
		UE_LOG(LogPopcornFXRenderDataFactory, Warning, TEXT("Couldn't create compatible render policy for material: invalid material domain."));
		return null;
	}
	if (hasVolumeMaterial && !vertexBBSupported)
	{
		UE_LOG(LogPopcornFXRenderDataFactory, Warning, TEXT("Couldn't create compatible render policy for volume texture: can't fallback on vertex shader billboarding policy."));
		return null;
	}
	if (hasVolumeMaterial)
		bbLocation = PopcornFX::Drawers::BillboardingLocation_VertexShader; // Force billboarding location

	if (gpuStorage)
	{
#if 1 // Force GPU billboard particles to be drawn using vertex shader billboarding, the compute shader path is kept for debugging purposes only
		if (vertexBBSupported && rendererType == PopcornFX::Renderer_Billboard)
			bbLocation = PopcornFX::Drawers::BillboardingLocation_VertexShader; // Force billboarding location
#endif

		if (bbLocation == PopcornFX::Drawers::BillboardingLocation_CPU ||
			rendererType != PopcornFX::Renderer_Billboard ||
			!vertexBBSupported)
		{
			switch (rendererType)
			{
				case	PopcornFX::Renderer_Billboard:
				{
					auto	bbBatch = PK_NEW((PopcornFX::TBillboardBatch<CPopcornFXRenderTypes, CRenderBatchPolicy>));
					if (!PK_VERIFY(bbBatch != null))
						return null;
					bbBatch->SetBillboardingLocation(PopcornFX::Drawers::BillboardingLocation_ComputeShader);
					return bbBatch;
				}
				case	PopcornFX::Renderer_Mesh:
					return PK_NEW((PopcornFX::TMeshBatch<CPopcornFXRenderTypes, CRenderBatchPolicy>));
				default:
					PK_ASSERT_NOT_REACHED();
					break;
			}
		}
		else
		{
			switch (rendererType)
			{
				case	PopcornFX::Renderer_Billboard:
				{
					auto	bbBatch = PK_NEW((PopcornFX::TBillboardBatch<CPopcornFXRenderTypes, CVertexBillboardingRenderBatchPolicy>));
					if (!PK_VERIFY(bbBatch != null))
						return null;
					bbBatch->SetBillboardingLocation(PopcornFX::Drawers::BillboardingLocation_VertexShader);
					return bbBatch;
				}
				default:
					PK_ASSERT_NOT_REACHED();
					break;
			}
		}
	}
	else
	{
		if (bbLocation == PopcornFX::Drawers::BillboardingLocation_CPU ||
			rendererType != PopcornFX::Renderer_Billboard ||
			!vertexBBSupported)
		{
			switch (rendererType)
			{
				case	PopcornFX::Renderer_Billboard:
					return PK_NEW((PopcornFX::TBillboardBatch<CPopcornFXRenderTypes, CRenderBatchPolicy>));
				case	PopcornFX::Renderer_Ribbon:
					return PK_NEW((PopcornFX::TRibbonBatch<CPopcornFXRenderTypes, CRenderBatchPolicy>));
				case	PopcornFX::Renderer_Triangle:
					return PK_NEW((PopcornFX::TTriangleBatch<CPopcornFXRenderTypes, CRenderBatchPolicy>));
				case	PopcornFX::Renderer_Light:
					return PK_NEW((PopcornFX::TLightBatch<CPopcornFXRenderTypes, CRenderBatchPolicy>));
				case	PopcornFX::Renderer_Mesh:
					return PK_NEW((PopcornFX::TMeshBatch<CPopcornFXRenderTypes, CRenderBatchPolicy>));
				case	PopcornFX::Renderer_Sound:
					return PK_NEW((PopcornFX::TSoundBatch<CPopcornFXRenderTypes, CRenderBatchPolicy>));
				default:
					PK_ASSERT_NOT_REACHED();
					break;
			}
		}
		else if (bbLocation == PopcornFX::Drawers::BillboardingLocation_VertexShader)
		{
			PK_ASSERT(GSupportsResourceView);
			switch (rendererType)
			{
				case	PopcornFX::Renderer_Billboard:
				{
					auto	bbBatch = PK_NEW((PopcornFX::TBillboardBatch<CPopcornFXRenderTypes, CVertexBillboardingRenderBatchPolicy>));
					if (!PK_VERIFY(bbBatch != null))
						return null;
					bbBatch->SetBillboardingLocation(bbLocation);
					return bbBatch;
				}
				default:
					PK_ASSERT_NOT_REACHED();
					break;
			}
		}
		else
		{
			PK_ASSERT_NOT_REACHED();
		}
	}
	return null;
}

//----------------------------------------------------------------------------
