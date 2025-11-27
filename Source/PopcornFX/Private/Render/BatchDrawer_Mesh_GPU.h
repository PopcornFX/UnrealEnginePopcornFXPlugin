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
#include "Render/PopcornFXGPUMeshVertexFactory.h"
#include "Render/PopcornFXRenderUtils.h"
#include "GPUSim/PopcornFXBillboarderCS.h"

#include <pk_particles/include/Renderers/ps_renderer_base.h>
#include <pk_render_helpers/include/batch_jobs/rh_batch_jobs_mesh_gpu.h>

//----------------------------------------------------------------------------

class FPopcornFXGPUMeshVertexCollector : public FOneFrameResource
{
public:
	FPopcornFXGPUMeshVertexFactory		*m_VertexFactory = null;

	FPopcornFXGPUMeshVertexCollector() { }
	~FPopcornFXGPUMeshVertexCollector()
	{
		if (PK_VERIFY(m_VertexFactory != null))
		{
			m_VertexFactory->ReleaseResource();
			delete m_VertexFactory;
			m_VertexFactory = null;
		}
	}
};

//----------------------------------------------------------------------------

class	CBatchDrawer_Mesh_GPUBB : public PopcornFX::CRendererBatchJobs_Mesh_GPUBB
{
	typedef PopcornFX::CRendererBatchJobs_Mesh_GPUBB	Super;
public:
	enum	EPopcornFXStreamOffsets
	{
		StreamOffset_Positions = 0,
		StreamOffset_Scales,
		StreamOffset_Orientations,
		StreamOffset_Enableds,
		StreamOffset_MeshIDs,
		StreamOffset_MeshLODs,
		__SupportedStreamCount
	};
	enum	EPopcornFXAdditionalStreamOffsets
	{
		StreamOffset_Colors = 0,
		StreamOffset_EmissiveColors3,
		StreamOffset_EmissiveColors4,
		StreamOffset_Velocity,
		StreamOffset_AlphaCursors,
		StreamOffset_TextureIDs,
		StreamOffset_VATCursors,
		StreamOffset_DynParam0s,
		StreamOffset_DynParam1s,
		StreamOffset_DynParam2s,
		StreamOffset_DynParam3s,
		__SupportedAdditionalStreamCount
	};

public:
	CBatchDrawer_Mesh_GPUBB();
	~CBatchDrawer_Mesh_GPUBB();

	virtual bool		Setup(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleRenderMedium *owner, const PopcornFX::CFrameCollector *fc, const PopcornFX::CStringId &storageClass) override;

	virtual bool		AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const override;

	virtual bool		CanRender(PopcornFX::SRenderContext &ctx) const override;

	virtual void		BeginFrame(PopcornFX::SRenderContext &ctx) override;
	virtual bool		AllocBuffers(PopcornFX::SRenderContext &ctx) override;
	virtual bool		MapBuffers(PopcornFX::SRenderContext &ctx) override;
	virtual bool		LaunchCustomTasks(PopcornFX::SRenderContext &ctx) override;

	virtual bool		UnmapBuffers(PopcornFX::SRenderContext &ctx) override;
	virtual bool		EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SDrawCallDesc &toEmit) override;

private:
	struct STransientComputeData
	{
		ERHIFeatureLevel::Type							m_FeatureLevel;
		PopcornFX::TArray<u32>							m_PerDrParticleCount;
		PopcornFX::TArray<FShaderResourceViewRHIRef>	m_PerDrSimDataSRV;
		PopcornFX::TArray<FShaderResourceViewRHIRef>	m_PerDrLiveParticleCountSRV;
		const CParticleScene							*m_ParticleScene = null;

		STransientComputeData(ERHIFeatureLevel::Type featureLevel, u32 drCount, const CParticleScene *particleScene)
			: m_FeatureLevel(featureLevel), m_PerDrParticleCount(drCount), m_PerDrSimDataSRV(drCount), m_PerDrLiveParticleCountSRV(drCount), m_ParticleScene(particleScene)
		{
		}

		bool	Valid() const
		{
			return	m_PerDrParticleCount.Count() > 0 &&
					m_PerDrParticleCount.Count() == m_PerDrSimDataSRV.Count() &&
					m_PerDrParticleCount.Count() == m_PerDrLiveParticleCountSRV.Count();
		}
	};

	struct	STransientDrawData
	{
		u32	nDr;		// Total number of draw requests
		u32	nLOD;		// Total number of LODs
		u32	iDr;		// Current draw request ID
		u32	iLOD;		// Current LOD level
		u32	iMesh;		// Current submesh among all LODs
		u32 iSection;	// Current submesh within the current LOD
		ERHIFeatureLevel::Type featureLevel;
		FShaderResourceViewRHIRef simData;
	};

	void		_PreSetupVFData(const FStaticMeshRenderData *staticMeshRenderData);
	void		_PreSetupMeshData(FPopcornFXGPUMeshVertexFactory::FDataType	&meshData, const FStaticMeshLODResources &meshResources);
	bool		_IsInputSupported(EPopcornFXStreamOffsets streamOffsetType);
	bool		_IsAdditionalInputSupported(const PopcornFX::CStringId &fieldName, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets &outStreamOffsetType);
	bool		_IssueDrawCall_Mesh(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc);
	void		_IssueDrawCall_Mesh_Sections(	const FPopcornFXSceneProxy				*sceneProxy,
												const SUERenderContext					&renderContext,
												const PopcornFX::SDrawCallDesc			&desc,
												CMaterialDesc_RenderThread				&matDesc,
												STransientDrawData						drawData,
												FMeshElementCollector					*collector);
	void		_CreateMeshVertexFactory(	const CMaterialDesc_RenderThread	&matDesc,
											const STransientDrawData			&drawData,
											FMeshElementCollector				*collector,
											FPopcornFXGPUMeshVertexFactory		*&outFactory,
											FPopcornFXGPUMeshVertexCollector	*&outCollectorRes);
	bool		_BuildMeshBatch(	FMeshBatch							&meshBatch,
									const FPopcornFXSceneProxy			*sceneProxy,
									const FSceneView					*view,
									const CMaterialDesc_RenderThread	&matDesc,
									const STransientDrawData			&drawData);
	bool		_BuildMeshBatchElement(	FMeshBatchElement				&batchElement,
										const STransientDrawData		&drawData,
										const FStaticMeshLODResources	&LODResources);
	bool		_DispatchComputeShaders(const STransientComputeData		&computeData);
	void		_CleanBuffers();

private:
	bool					m_BuffersInitialized = false;
	bool					m_ShouldInitLODConsts = false;
	u32						m_DrawRequestCount = 0;
	u32						m_TotalParticleCount = 0;
	u32						m_ParticleBuffersSize = 0; // The number of elements in GPU buffers that contain an element per particle
	u32						m_MeshCount = 0;
	u32						m_MeshLODCount = 0;
	PopcornFX::TArray<u32>	m_PerMeshIndexCount;
	PopcornFX::TArray<u32>	m_PerMeshIndexOffset;
	PopcornFX::TArray<u32>	m_PerLODMeshOffset;
	PopcornFX::TArray<FPopcornFXGPUMeshVertexFactory::FDataType>	m_PerLODVFData;
	FPopcornFXAtlasRectsVertexBuffer	m_AtlasRects;

	CPooledVertexBuffer									m_DrawIndirectBuffer;
	PopcornFX::TMemoryView<FDrawIndexedIndirectArgs>	m_DrawIndirectMapped;

	CPooledVertexBuffer		m_IndirectionBuffer;
	CPooledVertexBuffer		m_IndirectionOffsetsBuffer;

	CPooledVertexBuffer			m_MatricesBuffer;
	CPooledVertexBuffer			m_MatricesOffsetsBuffer;
	PopcornFX::TMemoryView<u32>	m_MatricesOffsetsMapped;

	CPooledVertexBuffer			m_LODsConstantsBuffer;
	FShaderResourceViewRHIRef	m_LODsConstants_MeshCountsView;
	FShaderResourceViewRHIRef	m_LODsConstants_MeshOffsetsView;
	PopcornFX::TMemoryView<u32>	m_LODsConstantsMapped;

	PopcornFX::TStaticArray<CPooledVertexBuffer, __SupportedStreamCount>			m_StreamOffsetsBuffers;
	PopcornFX::TStaticArray<PopcornFX::TMemoryView<u32>, __SupportedStreamCount>	m_StreamOffsetsMapped;

	PopcornFX::TStaticArray<SStreamOffset, __SupportedAdditionalStreamCount>	m_AdditionalStreamOffsets;

#if WITH_EDITOR
	// Used to check if the target mesh is still valid or has been modified
	const FStaticMeshRenderData	*m_StaticMeshRenderData = null;
#endif
};

//----------------------------------------------------------------------------
