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
#include "Render/PopcornFXSkeletalMeshVertexFactory.h"
#include "Render/BatchDrawer_Billboard_GPU.h" // FPopcornFXAtlasRectsVertexBuffer

#include <pk_particles/include/Renderers/ps_renderer_base.h>
#include <pk_render_helpers/include/batch_jobs/rh_batch_jobs_mesh_cpu.h>

//----------------------------------------------------------------------------

class FPopcornFXSkelMeshCollector : public FOneFrameResource
{
public:
	FPopcornFXSkelMeshVertexFactory		*m_VertexFactory = null;

	FPopcornFXSkelMeshCollector() { }
	~FPopcornFXSkelMeshCollector()
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

class	CBatchDrawer_SkeletalMesh_CPUBB : public PopcornFX::CRendererBatchJobs_Mesh_CPUBB
{
	typedef PopcornFX::CRendererBatchJobs_Mesh_CPUBB	Super;
private:
	enum	EPopcornFXAdditionalStreamOffsets
	{
		StreamOffset_Colors = 0,
		StreamOffset_EmissiveColors3,
		StreamOffset_EmissiveColors4,
		StreamOffset_AlphaCursors,
		StreamOffset_TextureIDs,
		StreamOffset_VATCursors,
		StreamOffset_VATCursorNexts,
		StreamOffset_VATTracks,
		StreamOffset_VATTrackNexts,
		StreamOffset_VATTransitionCursors,
		StreamOffset_PrevVATCursors,
		StreamOffset_PrevVATCursorNexts,
		StreamOffset_PrevVATTracks,
		StreamOffset_PrevVATTrackNexts,
		StreamOffset_PrevVATTransitionCursors,
		StreamOffset_DynParam0s,
		StreamOffset_DynParam1s,
		StreamOffset_DynParam2s,
		StreamOffset_DynParam3s,
		__SupportedAdditionalStreamCount
	};
public:
	CBatchDrawer_SkeletalMesh_CPUBB();
	~CBatchDrawer_SkeletalMesh_CPUBB();

	virtual bool		Setup(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleRenderMedium *owner, const PopcornFX::CFrameCollector *fc, const PopcornFX::CStringId &storageClass) override;

	virtual bool		AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const override;

	virtual bool		CanRender(PopcornFX::SRenderContext &ctx) const override;

	virtual void		BeginFrame(PopcornFX::SRenderContext &ctx) override;
	virtual bool		AllocBuffers(PopcornFX::SRenderContext &ctx) override;
	virtual bool		MapBuffers(PopcornFX::SRenderContext &ctx) override;

	virtual bool		UnmapBuffers(PopcornFX::SRenderContext &ctx) override;
	virtual bool		EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SDrawCallDesc &toEmit) override;

public:
	struct	SAdditionalInput
	{
		u32		m_BufferOffset = 0;
		u32		m_ByteSize = 0;
		u32		m_AdditionalInputIndex = 0;

		void	Clear();
	};

private:
	void		_IssueDrawCall_Mesh(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc);
	void		_IssueDrawCall_Mesh_Sections(	const FPopcornFXSceneProxy				*sceneProxy,
												const SUERenderContext					&renderContext,
												const PopcornFX::SDrawCallDesc			&desc,
												const PopcornFX::TMemoryView<const u32>	&perSectionPCount,
												CMaterialDesc_RenderThread				&matDesc,
												u32										&buffersOffset,
												u32										&boneIndicesReorderOffset,
												u32										LODLevel,
												FMeshElementCollector					*collector);
	bool		_IsAdditionalInputSupported(const PopcornFX::CStringId &fieldName, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets &outStreamOffsetType);

	bool		_BuildMeshBatch_Mesh(	FMeshBatch							&meshBatch,
										const FPopcornFXSceneProxy			*sceneProxy,
										const FSceneView					*view,
										const CMaterialDesc_RenderThread	&matDesc,
										const FSkeletalMeshLODRenderData	&LODRenderData,
										u32									LODLevel,
										u32									iSection,
										u32									sectionParticleCount,
										u32									boneIndicesReorderOffset);
	void		_FillUniforms(	CMaterialDesc_RenderThread		&matDesc,
								u32								buffersOffset,
								FPopcornFXUniforms				&outUniforms,
								FPopcornFXSkelMeshUniforms		&outUniformsMesh);
	void		_PreSetupMeshData(FPopcornFXSkelMeshVertexFactory::FDataType &meshData, const FSkeletalMeshLODRenderData &meshResources);
	void		_CreateSkelMeshVertexFactory(	CMaterialDesc_RenderThread		&matDesc,
												u32								buffersOffset,
												FMeshElementCollector			*collector,
												FPopcornFXSkelMeshVertexFactory	*&outFactory,
												FPopcornFXSkelMeshCollector			*&outCollectorRes);

#if RHI_RAYTRACING
	void		_IssueDrawCall_Mesh_AccelStructs(const SUERenderContext & renderContext, const PopcornFX::SDrawCallDesc & desc);
#endif // RHI_RAYTRACING

	void		_ClearBuffers();
	void		_ClearStreamOffsets();

private:
	u32								m_TotalParticleCount = 0;
	u32								m_TotalBoneCount = 0;
	u32								m_BoneMatricesBufferStride = 0;
	u32								m_PrevMatricesOffset = 0;
	u32								m_RealViewCount = 0;
	bool							m_MotionBlur = false;

	ERHIFeatureLevel::Type			m_FeatureLevel = ERHIFeatureLevel::Num;

	// Meshes buffers
	PopcornFX::TMemoryView<const u32>			m_PerMeshParticleCount;
	bool										m_HasMeshIDs;

#if RHI_RAYTRACING
	PopcornFX::CWorkingBuffer					m_RayTracing_Matrices;
	PopcornFX::TStridedMemoryView<CFloat4x4>	m_Mapped_RayTracing_Matrices;
#endif // RHI_RAYTRACING

	PopcornFX::TStridedMemoryView<CFloat4x4>	m_Mapped_Matrices;
	PopcornFX::TStridedMemoryView<CFloat4x4>	m_Mapped_PrevMatrices;
	FPopcornFXSkelMeshVertexFactory::FDataType	m_VFData;

	// Additional input fields
	PopcornFX::TArray<SAdditionalInput>						m_AdditionalInputs;
	PopcornFX::TArray<PopcornFX::Drawers::SCopyFieldDesc>	m_MappedAdditionalInputs;

	FPopcornFXAtlasRectsVertexBuffer						m_AtlasRects;

	CPooledVertexBuffer										m_SimData;
	CPooledVertexBuffer										m_BoneIndicesReorder;
#if (PK_PREBUILD_BONE_TRANSFORMS != 0)
	u32														m_BoneMatricesCapacity = 0;
	FRWBuffer												m_BoneMatrices;
	FRWBuffer												m_PreviousBoneMatrices;
#endif // (PK_PREBUILD_BONE_TRANSFORMS != 0)

	u32																			m_SimDataBufferSizeInBytes = 0;
	PopcornFX::TStaticArray<SStreamOffset, __SupportedAdditionalStreamCount>	m_AdditionalStreamOffsets;
};

//----------------------------------------------------------------------------
