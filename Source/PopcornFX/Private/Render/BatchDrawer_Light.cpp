//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "BatchDrawer_Light.h"
#include "RenderBatchManager.h"
#include "MaterialDesc.h"

#include "Assets/PopcornFXRendererMaterial.h"
#include "PopcornFXStats.h"

#include <pk_render_helpers/include/basic_renderer_properties/rh_basic_renderer_properties.h>
#include <pk_render_helpers/include/basic_renderer_properties/rh_vertex_animation_renderer_properties.h>

//----------------------------------------------------------------------------

CBatchDrawer_Light::CBatchDrawer_Light()
{
}

//----------------------------------------------------------------------------

CBatchDrawer_Light::~CBatchDrawer_Light()
{
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Light::AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const
{
	if (!Super::AreRenderersCompatible(rendererA, rendererB))
		return false;
	const CRendererCache	*firstMatCache = static_cast<const CRendererCache*>(rendererA->m_RendererCache.Get());
	const CRendererCache	*secondMatCache = static_cast<const CRendererCache*>(rendererB->m_RendererCache.Get());

	if (firstMatCache == null || secondMatCache == null)
		return false;

	const CMaterialDesc_RenderThread	&rDesc0 = firstMatCache->RenderThread_Desc();
	const CMaterialDesc_RenderThread	&rDesc1 = secondMatCache->RenderThread_Desc();
	if (rDesc0.m_RendererMaterial == null || rDesc1.m_RendererMaterial == null)
		return false;

	if (rDesc0.m_RendererClass != rDesc1.m_RendererClass) // We could try batching ribbons with billboards
		return false;
	return true; // Single batch for all lights
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Light::CanRender(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) const
{
	PK_ASSERT(drawPass.m_RendererCaches.First() != null);

	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);
	PK_ASSERT(renderContext.m_RendererSubView != null);

	return renderContext.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Main;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Light::BeginFrame(PopcornFX::SRenderContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Light::BeginFrame");

	Super::BeginFrame(ctx);

	INC_DWORD_STAT_BY(STAT_PopcornFX_BatchesCount, 1);
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Light::_IssueDrawCall_Light(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Light");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);
	const float			globalScale = view->GlobalScale();

	PK_ASSERT(!desc.m_DrawRequests.Empty());
	PK_ASSERT(desc.m_TotalParticleCount > 0);
	const u32		totalParticleCount = desc.m_TotalParticleCount;

	PopcornFX::TArray<FSimpleLightPerViewEntry>		&lightPositions = renderContext.m_CollectedDrawCalls->m_LightPositions;
	PopcornFX::TArray<FSimpleLightEntry>			&lightDatas = renderContext.m_CollectedDrawCalls->m_LightDatas;
	PK_ASSERT(lightPositions.Count() == lightDatas.Count());

	if (!lightPositions.Reserve(lightPositions.Count() + totalParticleCount) ||
		!lightDatas.Reserve(lightDatas.Count() + totalParticleCount))
	{
		PK_ASSERT_NOT_REACHED();
		return;
	}

	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return;
	CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();

	const bool		forceLitTranslucentGeometry = renderContext.m_RenderBatchManager->ForceLightsTranslucent();

	const u32	drCount = desc.m_DrawRequests.Count();
	for (u32 iDr = 0; iDr < drCount; ++iDr)
	{
		const PopcornFX::Drawers::SLight_DrawRequest			&drawRequest = *static_cast<const PopcornFX::Drawers::SLight_DrawRequest*>(desc.m_DrawRequests[iDr]);
		const PopcornFX::Drawers::SLight_BillboardingRequest	&bbRequest = static_cast<const PopcornFX::Drawers::SLight_BillboardingRequest&>(drawRequest.BaseBillboardingRequest());
		if (drawRequest.StorageClass() != PopcornFX::CParticleStorageManager_MainMemory::DefaultStorageClass())
		{
			PK_ASSERT_NOT_REACHED();
			return;
		}

		PK_TODO("renderManager->CullTest");

		const PopcornFX::CParticleStreamToRender_MainMemory	*lockedStream = drawRequest.StreamToRender_MainMemory();
		if (!PK_VERIFY(lockedStream != null)) // Light particles shouldn't handle GPU streams for now
			return;

		const PopcornFX::CGuid	volScatteringIntensityStreamId = bbRequest.StreamId(PopcornFX::CStringId("AffectsVolumetricFog.VolumetricScatteringIntensity")); // tmp

		static const float		kMinLightSize = 1e-3f;
		static const float		kExponent = 16.f; // simulate unlinearizeFallOff function
												  //------------------------------
												  // float A = (1.0f) - (1.0f / ((k + 1.0f) * (k + 1.0f)));
												  // float B = (1.0f / ((k + 1.0f) * (k + 1.0f)));
												  // float C = (1.0f / ((x * k + 1.0f) * (x * k + 1.0f)));
												  // return (C - B) / A;
												  //------------------------------
		static const float		kAttenuationSteepness = bbRequest.m_AttenuationSteepness;
		static const float		kAttenuationMultiplier = 4.32f; // represent the function f(x) = 4.32f x + 1.07f
		static const float		kAttenuationMinimumCoeff = 1.07f; // that simulate UE light intensity compared to PK FX unit-less light intensity

		const float				kColorMultiplier = (kAttenuationSteepness > 0.f) ? kAttenuationSteepness * kAttenuationMultiplier + kAttenuationMinimumCoeff : 0.f; // color multiplier is used to add light intensity for UE
		static const float		kLightRadiusMultiplier = 3.f; // multiplier to remove PBR render from UE

		const bool				lightsTranslucentGeometry = forceLitTranslucentGeometry | matDesc.m_LightsTranslucent;

		const u32	pageCount = lockedStream->PageCount();
		for (u32 pagei = 0; pagei < pageCount; ++pagei)
		{
			const PopcornFX::CParticlePageToRender_MainMemory		*page = lockedStream->Page(pagei);
			PK_ASSERT(page != null);
			const u32	pcount = page == null ? 0 : page->InputParticleCount();
			if (pcount == 0)
				continue;

			// Position
			TStridedMemoryView<const CFloat3>	positions = page->StreamForReading<CFloat3>(bbRequest.m_PositionStreamId);
			PK_ASSERT(positions.Count() == pcount);

			// Radius
			TStridedMemoryView<const float>		sizes;
			PK_ALIGN(0x10) float				defaultSize = 0.0f;
			if (PK_VERIFY(bbRequest.m_RangeStreamId.Valid()))
				sizes = page->StreamForReading<float>(bbRequest.m_RangeStreamId);
			else
				sizes = TStridedMemoryView<const float>(&defaultSize, pcount, 0);

			// Color
			TStridedMemoryView<const CFloat3>	colors(&CFloat3::ONE, pcount, 0);
			if (bbRequest.m_ColorStreamId.Valid())
				colors = TStridedMemoryView<const CFloat3>::Reinterpret(page->StreamForReading<CFloat4>(bbRequest.m_ColorStreamId));

			PK_ALIGN(0X10) float				defaultScatteringIntensity = 0.0f; // By default, lights do not affect volumetric fog if not present in the light material
			TStridedMemoryView<const float>		volumetricScatteringIntensity(&defaultScatteringIntensity, pcount, 0);
			if (volScatteringIntensityStreamId.Valid())
				volumetricScatteringIntensity = page->StreamForReading<float>(volScatteringIntensityStreamId);

			if (!PK_VERIFY(!positions.Empty()) ||
				!PK_VERIFY(!sizes.Empty()) ||
				!PK_VERIFY(!colors.Empty()))
				continue;

			const u8						enabledTrue = u8(-1);
			TStridedMemoryView<const u8>	enabledParticles = (bbRequest.m_EnabledStreamId.Valid()) ? page->StreamForReading<bool>(bbRequest.m_EnabledStreamId) : TStridedMemoryView<const u8>(&enabledTrue, pcount, 0);

			for (u32 parti = 0; parti < pcount; ++parti)
			{
				if (!enabledParticles[parti])
					continue;

				const float					radius = sizes[parti] * globalScale * kLightRadiusMultiplier;
				if (radius < kMinLightSize)
					continue;
				PopcornFX::CGuid			lposi = lightPositions.PushBack();
				FSimpleLightPerViewEntry	&lightpos = lightPositions[lposi];
				lightpos.Position = ToUE(positions[parti] * globalScale);

				PopcornFX::CGuid			ldatai = lightDatas.PushBack();
				FSimpleLightEntry			&lightdata = lightDatas[ldatai];
				lightdata.Color = ToUE(colors[parti] * kColorMultiplier);
				lightdata.Radius = radius;

				// Set the exponent to 0 if we want to enable inverse squared falloff
				// Note:
				//		- Radius will now represent the falloff's clamped area
				//		- Color will now represent the physical light's lumens
				lightdata.Exponent = kExponent;

				lightdata.VolumetricScatteringIntensity = volumetricScatteringIntensity[parti];

				lightdata.bAffectTranslucency = lightsTranslucentGeometry;
			}
		}
	}
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Light::EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass, const PopcornFX::SDrawCallDesc &toEmit)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Light::EmitDrawCall");
	PK_ASSERT(toEmit.m_DrawRequests.First() != null);

	// !Resolve material proxy and interface for first compatible material
	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);
	CRendererCache				*matCache = static_cast<CRendererCache*>(toEmit.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return true;

	_IssueDrawCall_Light(renderContext, toEmit);
	return true;
}

//----------------------------------------------------------------------------
