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
#include <pk_render_helpers/include/batch_jobs/rh_batch_jobs_billboard_gpu.h>

class	FPopcornFXGPUVertexFactory;
class	FPopcornFXGPUBillboardVSUniforms;

//----------------------------------------------------------------------------

PK_FIXME("Should be build from PopcornFXFile")
struct	FPopcornFXAtlasRectsVertexBuffer
{
	//FStructuredBufferRHIRef			m_AtlasBuffer_Structured;
#if (ENGINE_MAJOR_VERSION == 5)
	FBufferRHIRef					m_AtlasBuffer_Raw;
#else
	FVertexBufferRHIRef				m_AtlasBuffer_Raw;
#endif // (ENGINE_MAJOR_VERSION == 5)
	FShaderResourceViewRHIRef		m_AtlasBufferSRV;
	u32								m_AtlasRectsCount = 0;
	u32								m_AtlasBufferCapacity = 0;

	bool		Loaded() const { return m_AtlasRectsCount > 0; }
	void		Clear()
	{
		//m_AtlasBuffer_Structured = null;
		m_AtlasBuffer_Raw = null;
		m_AtlasBufferSRV = null;
		m_AtlasRectsCount = 0;
		m_AtlasBufferCapacity = 0;
	}
	bool		LoadRects(const PopcornFX::TMemoryView<const CFloat4> &rects);

private:
	bool		_LoadRects(const PopcornFX::TMemoryView<const CFloat4> &rects);
};

//----------------------------------------------------------------------------

struct	FPopcornFXDrawRequestsBuffer
{
#if (ENGINE_MAJOR_VERSION == 5)
	FBufferRHIRef				m_DrawRequestsBuffer;
#else
	FVertexBufferRHIRef			m_DrawRequestsBuffer;
#endif // (ENGINE_MAJOR_VERSION == 5)
	FShaderResourceViewRHIRef	m_DrawRequestsBufferSRV;

	~FPopcornFXDrawRequestsBuffer()
	{
		Unmap();
	}

	bool	LoadIFN();
	bool	Map(PopcornFX::TMemoryView<PopcornFX::Drawers::SBillboardDrawRequest> &outView, u32 elementCount);
	void	Unmap();

private:
	bool	m_Mapped = false;
};

//----------------------------------------------------------------------------

class	CBatchDrawer_Billboard_GPUBB : public PopcornFX::CRendererBatchJobs_Billboard_GPUBB
{
	typedef PopcornFX::CRendererBatchJobs_Billboard_GPUBB	Super;
public:
	enum	EPopcornFXStreamOffsets
	{
		StreamOffset_Positions = 0,
		StreamOffset_Sizes,
		StreamOffset_Size2s,
		StreamOffset_Rotations,
		StreamOffset_Axis0s,
		StreamOffset_Axis1s,
		__SupportedStreamCount
	};

	enum	EPopcornFXAdditionalStreamOffsets
	{
		StreamOffset_Colors = 0,
		StreamOffset_EmissiveColors,
		StreamOffset_TextureIDs,
		StreamOffset_AlphaCursors,
		StreamOffset_DynParam1s,
		StreamOffset_DynParam2s,
		StreamOffset_DynParam3s,
		__SupportedAdditionalStreamCount
	};

public:
	CBatchDrawer_Billboard_GPUBB();
	~CBatchDrawer_Billboard_GPUBB();

	virtual bool		Setup(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleRenderMedium *owner, const PopcornFX::CFrameCollector *fc, const PopcornFX::CStringId &storageClass) override;

	virtual bool		AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const override;

	virtual bool		CanRender(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) const override;

	virtual void		BeginFrame(PopcornFX::SRenderContext &ctx) override;
	virtual bool		AllocBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) override;
	virtual bool		MapBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) override;
	virtual bool		LaunchCustomTasks(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) override;

	virtual bool		UnmapBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) override;
	virtual bool		EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass, const PopcornFX::SDrawCallDesc &toEmit) override;

public:
	struct	SViewDependent
	{
		CPooledVertexBuffer		m_Indices;

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
	void		_ClearStreamOffsets();
	void		_Clear();
	void		_CleanRWBuffers();
	bool		_IsAdditionalInputSupported(const PopcornFX::CStringId &fieldName, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets &outStreamOffsetType);
	bool		_FillDrawCallUniforms_CPU(FPopcornFXGPUVertexFactory *vertexFactory, FPopcornFXGPUBillboardVSUniforms &vsUniforms, const PopcornFX::SDrawCallDesc &desc);
	bool		_FillDrawCallUniforms_GPU(u32 drId, const SUERenderContext &renderContext, FPopcornFXGPUVertexFactory *vertexFactory, FPopcornFXGPUBillboardVSUniforms &vsUniforms, const PopcornFX::SDrawCallDesc &desc);

private:
	u32			m_TotalParticleCount = 0;
	u32			m_TotalGPUBufferSize = 0;
	u32			m_ViewIndependentInputs = 0;
	u32			m_RendererType = 0;
	u32			m_RealViewCount = 0;
	u32			m_DrawIndirectArgsCapacity = 0;

	bool							m_HasAtlasBlending = false;
	bool							m_CapsulesDC = false;
	bool							m_ProcessViewIndependentData = false;
	ERHIFeatureLevel::Type			m_FeatureLevel = ERHIFeatureLevel::Num;

	FShaderResourceViewRHIRef		m_SimDataSRVRef;

	// View independent buffers
	CPooledVertexBuffer				m_Indices;
	CPooledVertexBuffer				m_SimData;

	// Additional input fields
	PopcornFX::TArray<SAdditionalInput>						m_AdditionalInputs;
	PopcornFX::TArray<PopcornFX::Drawers::SCopyFieldDesc>	m_MappedAdditionalInputs;

#if RHI_RAYTRACING
	FRWBuffer				m_RayTracingDynamicVertexBuffer;
	FRayTracingGeometry		m_RayTracingGeometry;
	bool					m_RTBuffers_Initialized = false;
#endif // RHI_RAYTRACING

	FPopcornFXAtlasRectsVertexBuffer	m_AtlasRects;
	FPopcornFXDrawRequestsBuffer		m_DrawRequests;
	//	FPopcornFXSortComputeShader_Sorter	m_Sorter;

	FRWBuffer							m_DrawIndirectBuffer;

	// View dependent buffers
	//PopcornFX::TStaticArray<SViewDependent, PopcornFX::CRendererSubView::kMaxViews>	m_ViewDependents;
	PopcornFX::TArray<SViewDependent>	m_ViewDependents;

	PopcornFX::TStaticArray<PopcornFX::CGuid, __SupportedStreamCount>			m_StreamOffsets;
	PopcornFX::TStaticArray<PopcornFX::CGuid, __SupportedAdditionalStreamCount>	m_AdditionalStreamOffsets;
	PopcornFX::TStaticArray<PopcornFX::CGuid, __SupportedAdditionalStreamCount>	m_AdditionalStreamIndices;
};

//----------------------------------------------------------------------------
