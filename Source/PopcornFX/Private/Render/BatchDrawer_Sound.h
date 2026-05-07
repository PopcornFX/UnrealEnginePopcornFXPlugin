//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------
#pragma once

#include "PopcornFXSDK.h"

#include <pk_particles/include/Renderers/ps_renderer_base.h>
#include <pk_render_helpers/include/batch_jobs/rh_batch_jobs_sound_std.h>

//----------------------------------------------------------------------------

struct	SUERenderContext;

//----------------------------------------------------------------------------

class	CBatchDrawer_Sound : public PopcornFX::CRendererBatchJobs_Sound_Std
{
	typedef PopcornFX::CRendererBatchJobs_Sound_Std	Super;
public:
	CBatchDrawer_Sound();
	~CBatchDrawer_Sound();

	virtual bool		AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const override;

	virtual bool		CanRender(PopcornFX::SRenderContext &ctx) const override;

	virtual void		BeginFrame(PopcornFX::SRenderContext &ctx) override;
	virtual bool		EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SDrawCallDesc &toEmit) override;

	virtual bool	Step_AllocBuffers(PopcornFX::SRenderContext &ctx) { return false;};
	virtual bool	Step_MapBuffers(PopcornFX::SRenderContext &ctx) { return false;};
	virtual bool	Step_LaunchBillboardingTasks(PopcornFX::SRenderContext &ctx, PopcornFX::Drawers::PAsynchronousJob_PostRenderTasks &syncJob) { return false;};
	virtual bool	Step_WaitForBillboardingTasks(PopcornFX::SRenderContext &ctx) { return false;};
	virtual bool	Step_UnmapBuffers(PopcornFX::SRenderContext &ctx) { return false;};


private:
	void				_IssueDrawCall_Sound(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc);
};

//----------------------------------------------------------------------------
