//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------
#pragma once

#include "PopcornFXSDK.h"

#include <pk_particles/include/Renderers/ps_renderer_base.h>
#include <pk_render_helpers/include/batch_jobs/rh_batch_jobs_light_std.h>

//----------------------------------------------------------------------------

struct	SUERenderContext;

//----------------------------------------------------------------------------

class	CBatchDrawer_Light : public PopcornFX::CRendererBatchJobs_Light_Std
{
	typedef PopcornFX::CRendererBatchJobs_Light_Std	Super;
public:
	CBatchDrawer_Light();
	~CBatchDrawer_Light();

	virtual bool		AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const override;

	virtual bool		CanRender(PopcornFX::SRenderContext &ctx) const override;

	virtual void		BeginFrame(PopcornFX::SRenderContext &ctx) override;
	virtual bool		EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SDrawCallDesc &toEmit) override;

private:
	void				_IssueDrawCall_Light(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc);
};

//----------------------------------------------------------------------------
