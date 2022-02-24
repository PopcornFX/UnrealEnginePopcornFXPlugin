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
#include "Render/PopcornFXMeshVertexFactory.h"

#include <pk_particles/include/Renderers/ps_renderer_base.h>
#include <pk_render_helpers/include/batch_jobs/rh_batch_jobs_mesh_cpu.h>

//----------------------------------------------------------------------------

class	FPopcornFXMeshCollector;

class	CBatchDrawer_Mesh_CPUBB : public PopcornFX::CRendererBatchJobs_Mesh_CPUBB
{
	typedef PopcornFX::CRendererBatchJobs_Mesh_CPUBB	Super;
private:
	enum	EPopcornFXAdditionalStreamOffsets
	{
		StreamOffset_Colors = 0,
		StreamOffset_EmissiveColors,
		StreamOffset_AlphaCursors,
		StreamOffset_DynParam0s,
		StreamOffset_VATCursors,
		StreamOffset_DynParam1s,
		StreamOffset_DynParam2s,
		//StreamOffset_DynParam3s,
		__SupportedAdditionalStreamCount
	};
public:
	CBatchDrawer_Mesh_CPUBB();
	~CBatchDrawer_Mesh_CPUBB();

	virtual bool		Setup(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleRenderMedium *owner, const PopcornFX::CFrameCollector *fc, const PopcornFX::CStringId &storageClass) override;

	virtual bool		AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const override;

	virtual bool		CanRender(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) const override;

	virtual void		BeginFrame(PopcornFX::SRenderContext &ctx) override;
	virtual bool		AllocBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) override;
	virtual bool		MapBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) override;

	virtual bool		UnmapBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) override;
	virtual bool		EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass, const PopcornFX::SDrawCallDesc &toEmit) override;

public:
	struct	SAdditionalInput
	{
		CPooledVertexBuffer	m_VertexBuffer; // Only valid for mesh renderers for now, will be removed entirely later
		u32					m_BufferOffset = 0;
		u32					m_ByteSize = 0;
		u32					m_AdditionalInputIndex = 0;

		void	UnmapBuffer();
		void	ClearBuffer();
	};

private:
	void		_IssueDrawCall_Mesh(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc);
	bool		_IsAdditionalInputSupported(const PopcornFX::CStringId& fieldName, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets& outStreamOffsetType);

	bool		_BuildMeshBatch_Mesh(	FMeshBatch &meshBatch,
										const FPopcornFXSceneProxy *sceneProxy,
										const FSceneView *view,
										const CMaterialDesc_RenderThread &matDesc,
										u32 iSection,
										u32 sectionParticleCount);
	void		_PreSetupMeshData(FPopcornFXMeshVertexFactory::FDataType& meshData, const FStaticMeshLODResources& meshResources);
	void		_CreateMeshVertexFactory(u32 buffersOffset, FMeshElementCollector* collector, FPopcornFXMeshVertexFactory*& outFactory, FPopcornFXMeshCollector*& outCollectorRes);

#if RHI_RAYTRACING
	void		_IssueDrawCall_Mesh_AccelStructs(const SUERenderContext & renderContext, const PopcornFX::SDrawCallDesc & desc);
#endif // RHI_RAYTRACING

	void		_ClearBuffers();
	void		_ClearStreamOffsets();

private:
	u32								m_TotalParticleCount = 0;
	u32								m_TotalVertexCount = 0;
	u32								m_TotalIndexCount = 0;
	u32								m_RealViewCount = 0;

	ERHIFeatureLevel::Type			m_FeatureLevel = ERHIFeatureLevel::Num;

	// Meshes buffers
	PopcornFX::TMemoryView<const u32>			m_PerMeshParticleCount;
	bool										m_HasMeshIDs;

#if RHI_RAYTRACING
	PopcornFX::CWorkingBuffer					m_RayTracing_Matrices;
	PopcornFX::TStridedMemoryView<CFloat4x4>	m_Mapped_RayTracing_Matrices;
#endif // RHI_RAYTRACING

	CPooledVertexBuffer							m_Instanced_Matrices;
	PopcornFX::CWorkingBuffer					m_NonInstanced_Wb;

	PopcornFX::TStridedMemoryView<CFloat4x4>	m_Mapped_Matrices;
	PopcornFX::TStridedMemoryView<CFloat4>		m_Mapped_Param_Colors;
	PopcornFX::TStridedMemoryView<CFloat4>		m_Mapped_Param_EmissiveColors;
	PopcornFX::TStridedMemoryView<float>		m_Mapped_Param_VATCursors;
	PopcornFX::TStridedMemoryView<CFloat4>		m_Mapped_Param_DynamicParameter0;
	PopcornFX::TStridedMemoryView<CFloat4>		m_Mapped_Param_DynamicParameter1;
	PopcornFX::TStridedMemoryView<CFloat4>		m_Mapped_Param_DynamicParameter2;
	FPopcornFXMeshVertexFactory::FDataType		m_VFData;

	// Additional input fields
	PopcornFX::TArray<SAdditionalInput>						m_AdditionalInputs;
	PopcornFX::TArray<PopcornFX::Drawers::SCopyFieldDesc>	m_MappedAdditionalInputs;

	CPooledVertexBuffer															m_SimData;
	u32																			m_SimDataBufferSizeInBytes = 0;
	PopcornFX::TStaticArray<SStreamOffset, __SupportedAdditionalStreamCount>	m_AdditionalStreamOffsets;
};

//----------------------------------------------------------------------------
