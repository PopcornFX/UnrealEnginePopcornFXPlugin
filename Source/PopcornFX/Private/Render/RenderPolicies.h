//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXSDK.h"
#include "RenderTypesPolicies.h"
#include "Render/PopcornFXBuffer.h"
#include "Render/RendererSubView.h"
#include "Render/PopcornFXMeshVertexFactory.h"
#include "Render/PopcornFXVertexFactoryShaderParameters.h"
#include "GPUSim/PopcornFXSortComputeShader.h"
#include "Render/MaterialDesc.h"
#include "Render/AudioPools.h"

#include <pk_particles/include/Renderers/ps_renderer_base.h>
#include <pk_render_helpers/include/frame_collector/rh_particle_render_data_factory.h>
#include <pk_render_helpers/include/batches/rh_particle_batch_policy_data.h>
#include <pk_render_helpers/include/draw_requests/rh_copystream_cpu.h>

#include <pk_kernel/include/kr_threads_basics.h>

class	CRenderBatchManager;
class	FPopcornFXMeshCollector;
class	FPopcornFXGPUVertexFactory;
class	FPopcornFXGPUBillboardVSUniforms;
class	FPopcornFXBillboardVSUniforms;

//----------------------------------------------------------------------------

class	CRenderDataFactory : public PopcornFX::TParticleRenderDataFactory<CPopcornFXRenderTypes>
{
private:
	typedef PopcornFX::TBillboardingBatchInterface<CPopcornFXRenderTypes>	CBillboardingBatchInterface;
public:
	CRenderDataFactory();
	virtual ~CRenderDataFactory() { }

	virtual PopcornFX::PRendererCacheBase		UpdateThread_CreateRendererCache(const PopcornFX::PRendererDataBase &renderer, const PopcornFX::CParticleDescriptor *particleDesc) override;
	virtual void								UpdateThread_CollectedForRendering(const PopcornFX::PRendererCacheBase &rendererCache) override;

	void										RenderThread_BuildPendingCaches();

	virtual CBillboardingBatchInterface			*CreateBillboardingBatch(PopcornFX::ERendererClass rendererType, const PopcornFX::PRendererCacheBase &rendererCache, bool gpuStorage) override;

public:
	CRenderBatchManager	*m_RenderBatchManager;

	PopcornFX::Threads::CCriticalSection	m_PendingCachesLock;
	PopcornFX::TArray<PRendererCache>		m_PendingCaches;
};

//----------------------------------------------------------------------------

PK_FIXME("Should be build from PopcornFXFile")
struct	FPopcornFXAtlasRectsVertexBuffer
{
	//FStructuredBufferRHIRef			m_AtlasBuffer_Structured;
	FVertexBufferRHIRef				m_AtlasBuffer_Raw;
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
	bool		LoadRects(const SRenderContext &renderContext, const TMemoryView<const CFloat4> &rects);

private:
	bool		_LoadRects(const SRenderContext &renderContext, const TMemoryView<const CFloat4> &rects);
};

//----------------------------------------------------------------------------

struct	FPopcornFXDrawRequestsBuffer
{
	FVertexBufferRHIRef			m_DrawRequestsBuffer;
	FShaderResourceViewRHIRef	m_DrawRequestsBufferSRV;

	~FPopcornFXDrawRequestsBuffer()
	{
		Unmap();
	}

	bool	LoadIFN();
	bool	Map(TMemoryView<PopcornFX::Drawers::SBillboardDrawRequest> &outView, u32 elementCount);
	void	Unmap();

private:
	bool	m_Mapped = false;
};

//----------------------------------------------------------------------------

struct	SStreamOffset
{
	u32					m_OffsetInBytes;
	u32					m_InputId; // Optional input id
	bool				m_ValidOffset;
	bool				m_ValidInputId;

	SStreamOffset()
	{
		Reset();
	}

	void				Reset()
	{
		m_ValidOffset = false;
		m_ValidInputId = false;
		m_OffsetInBytes = 0;
		m_InputId = 0;
	}

	s32				OffsetForShaderConstant() const { return m_ValidOffset ? m_OffsetInBytes / sizeof(float) : -1; }
	u32				InputId() const { PK_ASSERT(m_ValidOffset && m_ValidInputId); return m_InputId; }
	bool			Valid() const { return m_ValidOffset; }
	operator u32() const { PK_VERIFY(m_ValidOffset); return m_OffsetInBytes; }

	SStreamOffset	& operator = (u32 offsetInBytes)
	{
		m_OffsetInBytes = offsetInBytes;
		m_ValidOffset = true;
		m_ValidInputId = false;
		m_InputId = 0;
		return *this;
	}

	void			Setup(u32 offsetInBytes, u32 inputId)
	{
		m_OffsetInBytes = offsetInBytes;
		m_InputId = inputId;
		m_ValidOffset = true;
		m_ValidInputId = true;
	}
};

//----------------------------------------------------------------------------

class	CRenderBatchPolicy
{
	typedef PopcornFX::TSceneView<CBBView>	SBBView;
private:
	enum	EPopcornFXAdditionalStreamOffsets
	{
		StreamOffset_Colors = 0,
		StreamOffset_EmissiveColors,
		StreamOffset_AlphaCursors,
		StreamOffset_DynParam0s, // Only for mesh renderers
		StreamOffset_VATCursors, // Only for mesh renderers
		StreamOffset_DynParam1s,
		StreamOffset_DynParam2s,
		StreamOffset_DynParam3s,
		__SupportedAdditionalStreamCount
	};
public:
	CRenderBatchPolicy();
	~CRenderBatchPolicy();

	static bool		CanRender(const PopcornFX::Drawers::SBillboard_DrawRequest* request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext& ctx);
	static bool		CanRender(const PopcornFX::Drawers::SRibbon_DrawRequest* request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext& ctx);
	static bool		CanRender(const PopcornFX::Drawers::STriangle_DrawRequest* request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext& ctx);
	static bool		CanRender(const PopcornFX::Drawers::SMesh_DrawRequest* request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext& ctx);
	static bool		CanRender(const PopcornFX::Drawers::SLight_DrawRequest* request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext& ctx);
	static bool		CanRender(const PopcornFX::Drawers::SSound_DrawRequest* request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext& ctx);

	bool		Tick(const SRenderContext& renderContext, const TMemoryView<SBBView>& views);
	bool		IsStateless() { return false; }

	bool		AllocBuffers(const SRenderContext& renderContext, const PopcornFX::SBuffersToAlloc& allocBuffers, const TMemoryView<SBBView>& views, PopcornFX::ERendererClass rendererType);

	bool		MapBuffers(const SRenderContext& renderContext, const TMemoryView<SBBView>& views, PopcornFX::SBillboardBatchJobs* billboardBatch, const PopcornFX::SGeneratedInputs& toMap);
	bool		MapBuffers(const SRenderContext& renderContext, const TMemoryView<SBBView>& views, PopcornFX::SGPUBillboardBatchJobs* geomBBBatch, const PopcornFX::SGeneratedInputs& toMap);
	bool		MapBuffers(const SRenderContext& renderContext, const TMemoryView<SBBView>& views, PopcornFX::SRibbonBatchJobs* ribbonBatch, const PopcornFX::SGeneratedInputs& toMap);
	bool		MapBuffers(const SRenderContext& renderContext, const TMemoryView<SBBView>& views, PopcornFX::SGPURibbonBatchJobs* ribbonBatch, const PopcornFX::SGeneratedInputs& toMap);
	bool		MapBuffers(const SRenderContext& renderContext, const TMemoryView<SBBView>& views, PopcornFX::STriangleBatchJobs* triangleBatch, const PopcornFX::SGeneratedInputs& toMap);
	bool		MapBuffers(const SRenderContext& renderContext, const TMemoryView<SBBView>& views, PopcornFX::SGPUTriangleBatchJobs* triangleBatch, const PopcornFX::SGeneratedInputs& toMap);
	bool		MapBuffers(const SRenderContext& renderContext, const TMemoryView<SBBView>& views, PopcornFX::SMeshBatchJobs* meshBatch, const PopcornFX::SGeneratedInputs& toMap);

	bool		LaunchCustomTasks(const SRenderContext& renderContext, const TMemoryView<const PopcornFX::Drawers::SBillboard_DrawRequest* const>& drawRequests, PopcornFX::Drawers::CBillboard_CPU* billboardBatch);
	bool		LaunchCustomTasks(const SRenderContext& renderContext, const TMemoryView<const PopcornFX::Drawers::SBillboard_DrawRequest* const>& drawRequests, PopcornFX::Drawers::CCopyStream_CPU* billboardBatch);
	bool		LaunchCustomTasks(const SRenderContext& renderContext, const TMemoryView<const PopcornFX::Drawers::SRibbon_DrawRequest* const>& drawRequests, PopcornFX::Drawers::CRibbon_CPU* ribbonBatch);
	bool		LaunchCustomTasks(const SRenderContext& renderContext, const TMemoryView<const PopcornFX::Drawers::SRibbon_DrawRequest* const>& drawRequests, PopcornFX::Drawers::CCopyStream_CPU* ribbonBatch);
	bool		LaunchCustomTasks(const SRenderContext& renderContext, const TMemoryView<const PopcornFX::Drawers::STriangle_DrawRequest* const>& drawRequests, PopcornFX::Drawers::CTriangle_CPU* triangleBatch);
	bool		LaunchCustomTasks(const SRenderContext& renderContext, const TMemoryView<const PopcornFX::Drawers::STriangle_DrawRequest* const>& drawRequests, PopcornFX::Drawers::CCopyStream_CPU* triangleBatch);
	bool		LaunchCustomTasks(const SRenderContext& renderContext, const TMemoryView<const PopcornFX::Drawers::SMesh_DrawRequest* const>& drawRequests, PopcornFX::Drawers::CMesh_CPU* meshBatch);
	bool		LaunchCustomTasks(const SRenderContext& renderContext, const TMemoryView<const PopcornFX::Drawers::SLight_DrawRequest* const>& drawRequests, PopcornFX::Drawers::CBillboard_CPU* emptyBatch);
	bool		LaunchCustomTasks(const SRenderContext& renderContext, const TMemoryView<const PopcornFX::Drawers::SSound_DrawRequest* const>& drawRequests, PopcornFX::Drawers::CBillboard_CPU* emptyBatch);

	bool		WaitForCustomTasks(const SRenderContext& renderContext) { return true; }

	bool		UnmapBuffers(const SRenderContext& renderContext);
	void		ClearBuffers(const SRenderContext& renderContext);

	bool		AreBillboardingBatchable(const PopcornFX::PCRendererCacheBase& firstCache, const PopcornFX::PCRendererCacheBase& secondCache) const;

	bool		EmitDrawCall(const SRenderContext& renderContext, const PopcornFX::SDrawCallDesc& desc, SCollectedDrawCalls& output);
public:
	struct	SViewDependent
	{
		CPooledIndexBuffer		m_Indices;
		CPooledVertexBuffer		m_Positions;
		CPooledVertexBuffer		m_Normals;
		CPooledVertexBuffer		m_Tangents;
		CPooledVertexBuffer		m_UVFactors;
		CFloat4x4				m_BBMatrix; // Used by compute shader billboarding

		u32						m_ViewIndex;

		void	UnmapBuffers();
		void	ClearBuffers();
	};

	struct	SAdditionalInput
	{
		CPooledVertexBuffer	m_VertexBuffer; // Only valid for mesh renderers for now, will be removed entirely later
		u32					m_BufferOffset;
		u32					m_ByteSize;
		u32					m_AdditionalInputIndex;

		void	UnmapBuffer();
		void	ClearBuffer();
	};

private:
	void		_IssueDrawCall_Billboard(const SRenderContext& renderContext, const PopcornFX::SDrawCallDesc& desc);
	void		_IssueDrawCall_Ribbon(const SRenderContext& renderContext, const PopcornFX::SDrawCallDesc& desc);
	void		_IssueDrawCall_Triangle(const SRenderContext& renderContext, const PopcornFX::SDrawCallDesc& desc);
	void		_IssueDrawCall_Mesh(const SRenderContext& renderContext, const PopcornFX::SDrawCallDesc& desc);
	void		_IssueDrawCall_Light(const SRenderContext& renderContext, const PopcornFX::SDrawCallDesc& desc, SCollectedDrawCalls& output);
	void		_IssueDrawCall_Sound(const SRenderContext& renderContext, const PopcornFX::SDrawCallDesc& desc);

	bool		_BuildMeshBatch_Mesh(FMeshBatch& meshBatch,
		const FPopcornFXSceneProxy* sceneProxy,
		const FSceneView* view,
		const CMaterialDesc_RenderThread& matDesc,
		u32 iSection,
		u32 sectionParticleCount);

#if RHI_RAYTRACING
	void		_IssueDrawCall_Mesh_AccelStructs(const SRenderContext& renderContext, const PopcornFX::SDrawCallDesc& desc);
#endif // RHI_RAYTRACING

	bool		_IsAdditionalInputSupported(const PopcornFX::CStringId& fieldName, bool meshRenderer, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets& outStreamOffsetType);
	void		_PreSetupMeshData(FPopcornFXMeshVertexFactory::FDataType& meshData, const FStaticMeshLODResources& meshResources);
	void		_CreateMeshVertexFactory(u32 buffersOffset, FMeshElementCollector* collector, FPopcornFXMeshVertexFactory*& outFactory, FPopcornFXMeshCollector*& outCollectorRes);

	void		_ClearBuffers();
	void		_ClearStreamOffsets();

private:
	u32								m_UnusedCounter = 0;
	u32								m_TotalParticleCount = 0;
	u32								m_TotalVertexCount = 0;
	u32								m_TotalIndexCount = 0;
	u32								m_RendererType = 0;
	u32								m_RealViewCount = 0;

	ERHIFeatureLevel::Type			m_FeatureLevel;

	// UserData used to bind shader uniforms (Billboard & Ribbon)
	bool							m_SecondUVSet = false;
	bool							m_FlipUVs = false;
	bool							m_NeedsBTN = false;
	bool							m_CorrectRibbonDeformation = false;

	// View independent buffers
	CPooledIndexBuffer				m_Indices;

	// ~~
	CPooledVertexBuffer				m_Positions;
	CPooledVertexBuffer				m_Normals;
	CPooledVertexBuffer				m_Tangents;

	CPooledVertexBuffer				m_Texcoords;
	CPooledVertexBuffer				m_Texcoord2s;
	CPooledVertexBuffer				m_AtlasIDs;
	CPooledVertexBuffer				m_UVRemaps;
	CPooledVertexBuffer				m_UVFactors;

	// Meshes buffers
	PopcornFX::TMemoryView<const u32>			m_PerMeshParticleCount;
	bool										m_HasMeshIDs;
#if (ENGINE_MINOR_VERSION < 25)
	bool										m_Instanced;
#endif // (ENGINE_MINOR_VERSION < 25)

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

#if (PK_HAS_GPU != 0)
	FPopcornFXAtlasRectsVertexBuffer	m_AtlasRects;
	FPopcornFXSortComputeShader_Sorter	m_Sorter;
#endif // (PK_HAS_GPU != 0)

	// View dependent buffers
	//PopcornFX::TStaticArray<SViewDependent, PopcornFX::CRendererSubView::kMaxViews>	m_ViewDependents;
	PopcornFX::TArray<SViewDependent>	m_ViewDependents;

	CPooledVertexBuffer															m_SimData;
	u32																			m_CapsulesOffset = 0;
	u32																			m_SimDataBufferSizeInBytes = 0;
	PopcornFX::TStaticArray<SStreamOffset, __SupportedAdditionalStreamCount>	m_AdditionalStreamOffsets;
};

//----------------------------------------------------------------------------

class	CVertexBillboardingRenderBatchPolicy
{
	typedef PopcornFX::TSceneView<CBBView>	SBBView;
private:
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
	CVertexBillboardingRenderBatchPolicy();
	~CVertexBillboardingRenderBatchPolicy();

	static bool		CanRender(const PopcornFX::Drawers::SBillboard_DrawRequest *request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext &ctx);
	static bool		IsStateless() { return false; }
	bool			Tick(const SRenderContext &renderContext, const TMemoryView<SBBView> &views);
	bool			AllocBuffers(const SRenderContext &renderContext, const PopcornFX::SBuffersToAlloc &allocBuffers, const TMemoryView<SBBView> &views, PopcornFX::ERendererClass rendererType);
	bool			MapBuffers(const SRenderContext &renderContext, const TMemoryView<SBBView> &views, PopcornFX::SBillboardBatchJobs *billboardBatch, const PopcornFX::SGeneratedInputs &toMap);
	bool			MapBuffers(const SRenderContext &renderContext, const TMemoryView<SBBView> &views, PopcornFX::SGPUBillboardBatchJobs *geomBBBatch, const PopcornFX::SGeneratedInputs &toMap);
	bool			LaunchCustomTasks(const SRenderContext &renderContext, const TMemoryView<const PopcornFX::Drawers::SBillboard_DrawRequest * const> &drawRequests, PopcornFX::Drawers::CBillboard_CPU *billboardBatch);
	bool			LaunchCustomTasks(const SRenderContext &renderContext, const TMemoryView<const PopcornFX::Drawers::SBillboard_DrawRequest * const> &drawRequests, PopcornFX::Drawers::CCopyStream_CPU *billboardBatch);
	bool			WaitForCustomTasks(const SRenderContext &renderContext) { return true; }
	bool			UnmapBuffers(const SRenderContext &renderContext);
	void			ClearBuffers(const SRenderContext &renderContext);
	bool			AreBillboardingBatchable(const PopcornFX::PCRendererCacheBase &firstCache, const PopcornFX::PCRendererCacheBase &secondCache) const;
	bool			EmitDrawCall(const SRenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc, SCollectedDrawCalls &output);
public:
	struct	SViewDependent
	{
		CPooledVertexBuffer		m_Indices;

		u32						m_ViewIndex;

		void	UnmapBuffers();
		void	ClearBuffers();
	};

	struct	SAdditionalInput
	{
		u32		m_BufferOffset;
		u32		m_ByteSize;
		u32		m_AdditionalInputIndex;
	};

private:
	void		_IssueDrawCall_Billboard(const SRenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc);
	void		_ClearBuffers();
	void		_ClearStreamOffsets();
	bool		_IsAdditionalInputSupported(const PopcornFX::CStringId &fieldName, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets &outStreamOffsetType);
	bool		_FillDrawCallUniforms_CPU(FPopcornFXGPUVertexFactory *vertexFactory, FPopcornFXGPUBillboardVSUniforms &vsUniforms, const PopcornFX::SDrawCallDesc &desc);
	bool		_FillDrawCallUniforms_GPU(u32 drId, const SRenderContext &renderContext, FPopcornFXGPUVertexFactory *vertexFactory, FPopcornFXGPUBillboardVSUniforms &vsUniforms, const PopcornFX::SDrawCallDesc &desc);

private:
	u32			m_UnusedCounter = 0;
	u32			m_TotalParticleCount = 0;
	u32			m_TotalIndexCount = 0;
	u32			m_TotalGPUBufferSize = 0;
	u32			m_ViewIndependentInputs = 0;
	u32			m_RendererType = 0;
	u32			m_RealViewCount = 0;
	u32			m_DrawIndirectArgsCapacity = 0;

	bool							m_HasAtlasBlending = false;
	bool							m_CapsulesDC = false;
	ERHIFeatureLevel::Type			m_FeatureLevel;

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

	PopcornFX::TStaticArray<SStreamOffset, __SupportedStreamCount>				m_StreamOffsets;
	PopcornFX::TStaticArray<SStreamOffset, __SupportedAdditionalStreamCount>	m_AdditionalStreamOffsets;
	PopcornFX::TStaticArray<PopcornFX::CGuid, __SupportedAdditionalStreamCount>	m_AdditionalStreamIndices;
};

//----------------------------------------------------------------------------
