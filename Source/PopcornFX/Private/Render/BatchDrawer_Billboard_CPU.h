//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------
#pragma once

#include "PopcornFXSDK.h"
#include "RenderTypesPolicies.h"
#include "Render/PopcornFXBuffer.h"
#include "Render/RendererSubView.h"
#include "Render/MaterialDesc.h"

#include <pk_particles/include/Renderers/ps_renderer_base.h>
#include <pk_render_helpers/include/batch_jobs/rh_batch_jobs_billboard_cpu.h>

//----------------------------------------------------------------------------

class	CBatchDrawer_Billboard_CPUBB : public PopcornFX::CRendererBatchJobs_Billboard_CPUBB
{
	typedef PopcornFX::CRendererBatchJobs_Billboard_CPUBB	Super;
private:
	enum	EPopcornFXAdditionalStreamOffsets
	{
		StreamOffset_Colors = 0,
		StreamOffset_EmissiveColors,
		StreamOffset_PreviousPosition,
		StreamOffset_AlphaCursors,
		StreamOffset_DynParam1s,
		StreamOffset_DynParam2s,
		StreamOffset_DynParam3s,
		__SupportedAdditionalStreamCount
	};
public:
	CBatchDrawer_Billboard_CPUBB();
	~CBatchDrawer_Billboard_CPUBB();

	virtual bool		Setup(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleRenderMedium *owner, const PopcornFX::CFrameCollector *fc, const PopcornFX::CStringId &storageClass) override;

	virtual bool		AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const override;

	virtual bool		CanRender(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) const override;

	virtual void		BeginFrame(PopcornFX::SRenderContext &ctx) override;
	virtual bool		AllocBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) override;
	virtual bool		MapBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) override;

	virtual bool		UnmapBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) override;
	virtual bool		EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass, const PopcornFX::SDrawCallDesc &toEmit) override;

public:
	struct	SViewDependent
	{
		CPooledIndexBuffer		m_Indices;
		CPooledVertexBuffer		m_Positions;
		CPooledVertexBuffer		m_Normals;
		CPooledVertexBuffer		m_Tangents;

		u32						m_ViewIndex = 0;

		void	UnmapBuffers();
		void	ClearBuffers();
	};

	struct	SAdditionalInput
	{
		u32		m_BufferOffset = 0;
		u32		m_ByteSize = 0;
		u32		m_AdditionalInputIndex = 0;
	};

private:
	void		_IssueDrawCall_Billboard(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc);
	bool		_IsAdditionalInputSupported(const PopcornFX::CStringId& fieldName, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets& outStreamOffsetType);

	void		_ClearBuffers();
	void		_ClearStreamOffsets();

private:
	u32								m_TotalParticleCount = 0;
	u32								m_TotalVertexCount = 0;
	u32								m_TotalIndexCount = 0;
	u32								m_RealViewCount = 0;

	ERHIFeatureLevel::Type			m_FeatureLevel = ERHIFeatureLevel::Num;

	// UserData used to bind shader uniforms (Billboard & Ribbon)
	bool							m_SecondUVSet = false;
	bool							m_NeedsBTN = false;

	// View independent buffers
	CPooledIndexBuffer				m_Indices;

	// ~~
	CPooledVertexBuffer				m_Positions;
	CPooledVertexBuffer				m_Normals;
	CPooledVertexBuffer				m_Tangents;

	CPooledVertexBuffer				m_Texcoords;
	CPooledVertexBuffer				m_Texcoord2s;
	CPooledVertexBuffer				m_AtlasIDs;

	// Additional input fields
	PopcornFX::TArray<SAdditionalInput>						m_AdditionalInputs;
	PopcornFX::TArray<PopcornFX::Drawers::SCopyFieldDesc>	m_MappedAdditionalInputs;

	// View dependent buffers
	PopcornFX::TArray<SViewDependent>	m_ViewDependents;

	CPooledVertexBuffer															m_SimData;
	u32																			m_CapsulesOffset = 0;
	u32																			m_SimDataBufferSizeInBytes = 0;
	PopcornFX::TStaticArray<SStreamOffset, __SupportedAdditionalStreamCount>	m_AdditionalStreamOffsets;
};

//----------------------------------------------------------------------------
