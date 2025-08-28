//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------
#pragma once

#include "PopcornFXSDK.h"

#include "SceneManagement.h"
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
#include "SceneProxies/DeferredDecalProxy.h"
#endif

#include <pk_particles/include/Renderers/ps_renderer_base.h>
#include <pk_render_helpers/include/batch_jobs/rh_batch_jobs_decal_cpu.h>

//----------------------------------------------------------------------------

struct	SUERenderContext;
class	FDeferredDecalProxy;
class	UWorld;
class	USceneComponent;
class	UMaterialInterface;
class	UPopcornFXSceneComponent;

//----------------------------------------------------------------------------

class	CBatchDrawer_Decal_CPUBB : public PopcornFX::CRendererBatchJobs_Decal_CPUBB
{
	typedef PopcornFX::CRendererBatchJobs_Decal_CPUBB	Super;
public:
	CBatchDrawer_Decal_CPUBB();
	~CBatchDrawer_Decal_CPUBB();

	virtual bool		Setup(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleRenderMedium *owner, const PopcornFX::CFrameCollector *fc, const PopcornFX::CStringId &storageClass) override;

	virtual bool		AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const override;
	static bool			IsCompatible(UMaterialInterface *material);

	virtual bool		CanRender(PopcornFX::SRenderContext &ctx) const override;

	virtual void		BeginFrame(PopcornFX::SRenderContext &ctx) override;
	virtual bool		EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SDrawCallDesc &toEmit) override;

private:
	struct	SDecalStreams
	{
		// Enabled particles, positions, orientations, scales
		TStridedMemoryView<const u8>			enableds;
		TStridedMemoryView<const CFloat3>		positions;
		TStridedMemoryView<const CQuaternion>	orientations;
		TStridedMemoryView<const CFloat3>		scalesF3;
		TStridedMemoryView<const float>			scalesF1;
		bool									isScaleFloat3 = true;

		// Additionnal inputs
		TStridedMemoryView<const CFloat4>	colorsDiffuse;
		TStridedMemoryView<const CFloat4>	colorsEmissive;
		TStridedMemoryView<const CFloat3>	colorsEmissiveLegacy;
		TStridedMemoryView<const float>		atlasTextureIDs;
		TStridedMemoryView<const float>		alphaRemapCursors;
	};

	template<typename T>
	void				_GetStreamForReading(	TStridedMemoryView<const T> &outStream, const PopcornFX::CParticlePageToRender_MainMemory *page, 
												PopcornFX::CGuid streamId, u32 pcount);

	void				_GetDecalStreams(	SDecalStreams &outStreams, const PopcornFX::Drawers::SDecal_BillboardingRequest &bbRequest, 
											const PopcornFX::CParticlePageToRender_MainMemory *page, u32 pcount);

	void				_BuildDecalUpdates(	TArray<FDeferredDecalUpdateParams> &decalUpdates, const SDecalStreams &ds, 
											const UPopcornFXSceneComponent *sceneComp, u32 &activeDecalProxiesCounter, u32 pcount, float globalScale);

	void				_IssueDrawCall_Decal(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc);

	void				_ReleaseAllDecals();

private:
	PopcornFX::TArray<FDeferredDecalProxy*>	m_ActiveDecalProxies;
	TWeakObjectPtr<UMaterialInterface>		m_WeakMaterial = nullptr;
	TWeakObjectPtr<const UWorld>			m_WeakWorld = nullptr;
};

//----------------------------------------------------------------------------
