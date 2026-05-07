//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "BatchDrawer_Sound.h"
#include "RenderBatchManager.h"
#include "MaterialDesc.h"

#include "Render/AudioPools.h"

#if (ENGINE_MAJOR_VERSION == 5)
#	include "AudioDevice.h"
#else
#	if !PLATFORM_PS4
#		include "AudioDevice.h"
#	else
#		define PK_CAN_USE_AUDIO_DEVICE	0
#	endif // !PLATFORM_PS4
#endif // (ENGINE_MAJOR_VERSION == 5)

#ifndef PK_CAN_USE_AUDIO_DEVICE
#	define PK_CAN_USE_AUDIO_DEVICE	1
#endif

#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "PopcornFXStats.h"

#include <pk_render_helpers/include/render_features/rh_features_basic.h>
#include <pk_render_helpers/include/render_features/rh_features_vat_static.h>
#include <pk_particles/include/Storage/MainMemory/storage_ram.h>

//----------------------------------------------------------------------------

CBatchDrawer_Sound::CBatchDrawer_Sound()
{
}

//----------------------------------------------------------------------------

CBatchDrawer_Sound::~CBatchDrawer_Sound()
{
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Sound::AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const
{
	if (!Super::AreRenderersCompatible(rendererA, rendererB))
		return false;

	// we MUST return false,
	// because we only use SelfID, which is uniq only inside ParticleMediums
	// @FIXME: use SelfID+"MediumID?" to make AreRenderersCompatible return rendererA->CompatibleWith(rendererB))
	// @FIXME2: Unless batches are cleared each frame, this will cause issues
	return false;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Sound::CanRender(PopcornFX::SRenderContext &ctx) const
{
	PK_ASSERT(DrawPass().m_RendererCaches.First() != null);

	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);
	PK_ASSERT(renderContext.m_RendererSubView != null);

	return renderContext.m_RendererSubView->Pass() == CRendererSubView::UpdatePass_PostUpdate;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Sound::BeginFrame(PopcornFX::SRenderContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Sound::BeginFrame");

	Super::BeginFrame(ctx);

	INC_DWORD_STAT_BY(STAT_PopcornFX_BatchesCount, 1);
}

//----------------------------------------------------------------------------
//
// Sounds
//
//----------------------------------------------------------------------------

namespace
{
	bool		IsAudioEnabled(UWorld *world)
	{
		if (GEngine == null || !GEngine->UseSound())
			return false;
		if (world == null)
			return false;
		if (!world->GetAudioDevice().IsValid())
			return false;
		if (world->WorldType != EWorldType::Game &&
			world->WorldType != EWorldType::PIE &&
			world->WorldType != EWorldType::Editor)
			return false;
		return true;
	}
} // namespace

//----------------------------------------------------------------------------

void	CBatchDrawer_Sound::_IssueDrawCall_Sound(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Sound");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);
	const float				globalScale = view->GlobalScale();

	PK_ASSERT(!desc.m_DrawRequests.Empty());
	PK_ASSERT(desc.m_TotalParticleCount > 0);
	const u32		totalParticleCount = desc.m_TotalParticleCount;

	PK_ASSERT(view->Pass() == CRendererSubView::UpdatePass_PostUpdate);
	PK_ASSERT(IsInGameThread());

	UWorld	*world = renderContext.GetWorld();
	if (!IsAudioEnabled(world))
		return;

	PK_ASSERT(desc.m_DrawRequests.Count() == 1);

	PK_ASSERT(world->WorldType == EWorldType::PIE ||
		world->WorldType == EWorldType::Game ||
		world->WorldType == EWorldType::Editor);

	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return;
	CMaterialDesc_GameThread		&matDesc = matCache->GameThread_Desc();
	CSoundDescriptorPoolCollection	*sPoolCollection = matDesc.m_SoundPoolCollection;

	if (!PK_VERIFY(sPoolCollection != null))
		return;

	sPoolCollection->BeginInsert(world);

	const u32		soundPoolCount = sPoolCollection->m_Pools.Count();
	const u32		maxSoundPoolID = soundPoolCount - 1;
	const float		maxSoundPoolsFp = maxSoundPoolID;

	for (u32 iDr = 0; iDr < desc.m_DrawRequests.Count(); ++iDr)
	{
		const PopcornFX::Drawers::SSound_DrawRequest			&drawRequest = *static_cast<const PopcornFX::Drawers::SSound_DrawRequest*>(desc.m_DrawRequests[iDr]);
		const PopcornFX::Drawers::SSound_BillboardingRequest	&bbRequest = static_cast<const PopcornFX::Drawers::SSound_BillboardingRequest&>(drawRequest.BaseBillboardingRequest());
		PK_ASSERT(drawRequest.Valid());
		PK_ASSERT(drawRequest.RenderedParticleCount() > 0);

		const float		dopplerFactor = bbRequest.m_DopplerFactor;
		const float		isBlended = false; // TODO: Implement sound atlas

		if (drawRequest.StorageClass() != PopcornFX::CParticleStorageManager_MainMemory::DefaultStorageClass())
		{
			PK_ASSERT_NOT_REACHED();
			return;
		}

		const PopcornFX::CParticleStreamToRender_MainMemory	*lockedStream = drawRequest.StreamToRender_MainMemory();
		if (!PK_VERIFY(lockedStream != null)) // Light particles shouldn't handle GPU streams for now
			return;

		const u32	pageCount = lockedStream->PageCount();
		for (u32 iPage = 0; iPage < pageCount; ++iPage)
		{
			const PopcornFX::CParticlePageToRender_MainMemory	*page = lockedStream->Page(iPage);
			PK_ASSERT(page != null && !page->Empty());

			const u32	particleCount = page->InputParticleCount();

			PopcornFX::TStridedMemoryView<const float>		lifeRatios = page->StreamForReading<float>(bbRequest.m_LifeRatioStreamId);
			PopcornFX::TStridedMemoryView<const float>		invLives = page->StreamForReading<float>(bbRequest.m_InvLifeStreamId);
			PopcornFX::TStridedMemoryView<const CInt2>		selfIDs = page->StreamForReading<CInt2>(bbRequest.m_SelfIDStreamId);
			PopcornFX::TStridedMemoryView<const CFloat3>	positions = page->StreamForReading<CFloat3>(bbRequest.m_PositionStreamId);

			PopcornFX::TStridedMemoryView<const CFloat3>	velocities = bbRequest.m_VelocityStreamId.Valid() ? page->StreamForReading<CFloat3>(bbRequest.m_VelocityStreamId) : PopcornFX::TStridedMemoryView<const CFloat3>(&CFloat4::ZERO.xyz(), positions.Count(), 0);
			//			PopcornFX::TStridedMemoryView<const float>		soundIDs = bbRequest.m_SoundIDStreamId.Valid() ? page->StreamForReading<float>(bbRequest.m_SoundIDStreamId) : PopcornFX::TStridedMemoryView<const float>(&CFloat4::ZERO.x(), positions.Count(), 0);
			PopcornFX::TStridedMemoryView<const float>		volumes = bbRequest.m_VolumeStreamId.Valid() ? page->StreamForReading<float>(bbRequest.m_VolumeStreamId) : PopcornFX::TStridedMemoryView<const float>(&CFloat4::ONE.x(), positions.Count(), 0);
			PopcornFX::TStridedMemoryView<const float>		radii = bbRequest.m_RangeStreamId.Valid() ? page->StreamForReading<float>(bbRequest.m_RangeStreamId) : PopcornFX::TStridedMemoryView<const float>(&CFloat4::ONE.x(), positions.Count(), 0);

			const u8										enabledTrue = u8(-1);
			TStridedMemoryView<const u8>					enabledParticles = (bbRequest.m_EnabledStreamId.Valid()) ? page->StreamForReading<bool>(bbRequest.m_EnabledStreamId) : TStridedMemoryView<const u8>(&enabledTrue, particleCount, 0);

			const u32	posCount = positions.Count();
			PK_ASSERT(
				posCount == particleCount &&
				posCount == lifeRatios.Count() &&
				posCount == invLives.Count() &&
				posCount == selfIDs.Count() &&
				posCount == velocities.Count() &&
				//posCount == soundIDs.Count() &&
				posCount == volumes.Count() &&
				posCount == radii.Count());

			for (u32 iParticle = 0; iParticle < posCount; ++iParticle)
			{
				if (!enabledParticles[iParticle])
					continue;

				const float		volume = volumes[iParticle];

				const FVector	pos = FVector(ToUE(positions[iParticle] * globalScale));
				const float		radius = radii[iParticle] * globalScale;

#if (PK_CAN_USE_AUDIO_DEVICE == 0)
				const bool		audible = true; // Since 4.19.1/2 ....
#else
				const bool		audible = world->GetAudioDevice()->LocationIsAudible(pos, radius + radius * 0.5f);
#endif // (PK_CAN_USE_AUDIO_DEVICE == 0)

				const float		soundId = 0;//PopcornFX::PKMin(soundIDs[iParticle], maxSoundPoolsFp);
				const float		soundIdFrac = PopcornFX::PKFrac(soundId) * isBlended;
				const u32		soundId0 = u32(soundId);
				const u32		soundId1 = PopcornFX::PKMin(soundId0 + 1, maxSoundPoolID);

				SSoundInsertDesc	sDesc;
				sDesc.m_SelfID = selfIDs[iParticle];
				sDesc.m_Position = pos;
				sDesc.m_Velocity = FVector(ToUE(velocities[iParticle] * globalScale));
				sDesc.m_Radius = radius;
				sDesc.m_DopplerLevel = dopplerFactor;
				sDesc.m_Age = lifeRatios[iParticle] / invLives[iParticle];
				sDesc.m_Audible = audible;

				sDesc.m_Volume = volume * (1.0f - soundIdFrac);
				sPoolCollection->m_Pools[soundId0].InsertSoundIFP(sDesc);

				if (!isBlended)
					continue;

				sDesc.m_Volume = volume * soundIdFrac;
				sPoolCollection->m_Pools[soundId1].InsertSoundIFP(sDesc);
			}
		}
	}

	sPoolCollection->EndInsert(world);

	INC_DWORD_STAT_BY(STAT_PopcornFX_SoundCount, totalParticleCount);
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Sound::EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SDrawCallDesc &toEmit)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Sound::EmitDrawCall");
	PK_ASSERT(toEmit.m_DrawRequests.First() != null);

	// !Resolve material proxy and interface for first compatible material
	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);
	CRendererCache				*matCache = static_cast<CRendererCache*>(toEmit.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return true;

	_IssueDrawCall_Sound(renderContext, toEmit);
	return true;
}

//----------------------------------------------------------------------------
