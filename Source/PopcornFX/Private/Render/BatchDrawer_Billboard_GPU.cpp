//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "BatchDrawer_Billboard_GPU.h"

#include "RenderBatchManager.h"
#include "SceneManagement.h"
#include "MaterialDesc.h"
#include "ParticleResources.h"

#include "World/PopcornFXSceneProxy.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "Render/PopcornFXGPUVertexFactory.h"
#include "Render/PopcornFXVertexFactoryCommon.h"
#include "Render/PopcornFXShaderUtils.h"
#include "Render/PopcornFXRendererProperties.h"
#include "PopcornFXStats.h"

#include <pk_render_helpers/include/render_features/rh_features_basic.h>

#if (PK_HAS_GPU == 1)
	// DrawIndexedInstanced (ie. https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_draw_indexed_instanced_indirect_args)
#	define POPCORNFX_INDIRECT_ARGS_ARG_COUNT	5
#	include "GPUSim/PopcornFXBillboarderCS.h"
#endif // (PK_HAS_GPU == 1)

// To remove once old FC impl is removed
DECLARE_GPU_STAT_NAMED(PopcornFXBillboardCopySizeBuffer, TEXT("PopcornFX Billboards copy size buffer"));

DEFINE_LOG_CATEGORY_STATIC(LogVertexBillboardingPolicy, Log, All);

//----------------------------------------------------------------------------
//
// FPopcornFXAtlasRectsVertexBuffer
//
//----------------------------------------------------------------------------

bool	FPopcornFXAtlasRectsVertexBuffer::LoadRects(const PopcornFX::TMemoryView<const CFloat4> &rects)
{
	if (rects.Empty())
	{
		Clear();
		return false;
	}
	if (!_LoadRects(rects))
	{
		Clear();
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	FPopcornFXAtlasRectsVertexBuffer::_LoadRects(const PopcornFX::TMemoryView<const CFloat4> &rects)
{
	const u32		bytes = rects.CoveredBytes();
	const bool		empty = m_AtlasBuffer_Raw == null;

	if (empty || bytes > m_AtlasBufferCapacity)
	{
		m_AtlasBufferCapacity = PopcornFX::Mem::Align<0x100>(bytes + 0x10);

#if (ENGINE_MAJOR_VERSION == 5)
		FRHIResourceCreateInfo	info(TEXT("PopcornFX Atlas buffer"));
#else
		FRHIResourceCreateInfo	info;
#endif // (ENGINE_MAJOR_VERSION == 5)
		const EBufferUsageFlags	usage = BUF_Static | BUF_ShaderResource;

		m_AtlasBuffer_Raw = RHICreateVertexBuffer(m_AtlasBufferCapacity, usage, info);
		if (!PK_VERIFY(IsValidRef(m_AtlasBuffer_Raw)))
			return false;
		m_AtlasBufferSRV = RHICreateShaderResourceView(m_AtlasBuffer_Raw, sizeof(CFloat4), PF_A32B32G32R32F);

		if (!PK_VERIFY(IsValidRef(m_AtlasBufferSRV)))
			return false;
	}
	else
	{
		PK_ASSERT(m_AtlasBufferSRV != null);
	}

#if (ENGINE_MAJOR_VERSION == 5)
	void	*map = RHILockBuffer(m_AtlasBuffer_Raw, 0, bytes, RLM_WriteOnly);
#else
	void	*map = RHILockVertexBuffer(m_AtlasBuffer_Raw, 0, bytes, RLM_WriteOnly);
#endif // (ENGINE_MAJOR_VERSION == 5)
	if (!PK_VERIFY(map != null))
		return false;
	PopcornFX::Mem::Copy(map, rects.Data(), bytes);
#if (ENGINE_MAJOR_VERSION == 5)
	RHIUnlockBuffer(m_AtlasBuffer_Raw);
#else
	RHIUnlockVertexBuffer(m_AtlasBuffer_Raw);
#endif // (ENGINE_MAJOR_VERSION == 5)

	m_AtlasRectsCount = rects.Count();
	return true;
}
//----------------------------------------------------------------------------
//
//	FPopcornFXDrawRequestsBuffer
//
//----------------------------------------------------------------------------

bool	FPopcornFXDrawRequestsBuffer::LoadIFN()
{
	if (m_DrawRequestsBuffer == null)
	{
		const u32		totalByteCount = 0x100 * sizeof(PopcornFX::Drawers::SBillboardDrawRequest); // Is not exposed yet to PK-RenderHelpers

#if (ENGINE_MAJOR_VERSION == 5)
		FRHIResourceCreateInfo	info(TEXT("PopcornFX Draw requests buffer"));
#else
		FRHIResourceCreateInfo	info;
#endif // (ENGINE_MAJOR_VERSION == 5)
		const EBufferUsageFlags	usage = BUF_Dynamic | BUF_ShaderResource;

		m_DrawRequestsBuffer = RHICreateVertexBuffer(totalByteCount, usage, info);
		if (!PK_VERIFY(IsValidRef(m_DrawRequestsBuffer)))
		{
			UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("Couldn't create draw requests structured buffer"));
			return false;
		}
		m_DrawRequestsBufferSRV = RHICreateShaderResourceView(m_DrawRequestsBuffer, sizeof(PopcornFX::Drawers::SBillboardDrawRequest), PF_A32B32G32R32F);

		if (!PK_VERIFY(IsValidRef(m_DrawRequestsBufferSRV)))
		{
			UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("Invalid draw requests structured buffer ref"));
			return false;
		}
	}
	else
	{
		PK_ASSERT(m_DrawRequestsBufferSRV != null);
	}
	return true;
}

//----------------------------------------------------------------------------

bool	FPopcornFXDrawRequestsBuffer::Map(PopcornFX::TMemoryView<PopcornFX::Drawers::SBillboardDrawRequest> &outView, u32 elementCount)
{
	if (!PK_VERIFY(m_DrawRequestsBuffer != null) ||
		!PK_VERIFY(!m_Mapped))
	{
		if (m_Mapped)
		{
			UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("Map called on already mapped draw requests buffer"));
		}

		outView.Clear();
		return false;
	}
	m_Mapped = true;
	const u32	totalByteCount = elementCount * sizeof(PopcornFX::Drawers::SBillboardDrawRequest); // Is not exposed yet to PK-RenderHelpers
#if (ENGINE_MAJOR_VERSION == 5)
	void		*mappedBuffer = RHILockBuffer(m_DrawRequestsBuffer, 0, totalByteCount, RLM_WriteOnly);
#else
	void		*mappedBuffer = RHILockVertexBuffer(m_DrawRequestsBuffer, 0, totalByteCount, RLM_WriteOnly);
#endif // (ENGINE_MAJOR_VERSION == 5)
	if (!PK_VERIFY(mappedBuffer != null))
	{
		outView.Clear();
		UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("Couldn't map draw requests structured buffer"));
		return false;
	}
	outView = PopcornFX::TMemoryView<PopcornFX::Drawers::SBillboardDrawRequest>(reinterpret_cast<PopcornFX::Drawers::SBillboardDrawRequest*>(mappedBuffer), elementCount);
	return true;
}

//----------------------------------------------------------------------------

void	FPopcornFXDrawRequestsBuffer::Unmap()
{
	if (m_Mapped)
	{
		m_Mapped = false;
#if (ENGINE_MAJOR_VERSION == 5)
		RHIUnlockBuffer(m_DrawRequestsBuffer);
#else
		RHIUnlockVertexBuffer(m_DrawRequestsBuffer);
#endif // (ENGINE_MAJOR_VERSION == 5)
	}
}

//----------------------------------------------------------------------------
//
//	FPopcornFXGPUBillboardCollector
//
//----------------------------------------------------------------------------

class FPopcornFXGPUBillboardCollector : public FOneFrameResource
{
public:
	FPopcornFXGPUVertexFactory		*m_VertexFactory = null;
	//	CPooledIndexBuffer				m_IndexBufferForRefCount;

	FPopcornFXGPUBillboardCollector() { }
	~FPopcornFXGPUBillboardCollector()
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
//
//	FPopcornFXGPUParticlesIndexBuffer
//
//----------------------------------------------------------------------------

class	FPopcornFXGPUParticlesIndexBuffer : public FIndexBuffer
{
public:
	virtual void	InitRHI() override
	{
		const uint32			sizeInBytes = sizeof(u16) * 12;
		const uint32			stride = sizeof(u16);
		FRHIResourceCreateInfo	info(TEXT("PopcornFXGPUParticlesIndexBuffer"));

		void	*data = null;
#if (ENGINE_MAJOR_VERSION == 5)
		IndexBufferRHI = RHICreateBuffer(sizeInBytes, BUF_Static | BUF_IndexBuffer, stride, ERHIAccess::VertexOrIndexBuffer, info);
		data = RHILockBuffer(IndexBufferRHI, 0, sizeInBytes, RLM_WriteOnly);
#else
		IndexBufferRHI = RHICreateAndLockIndexBuffer(stride, sizeInBytes, BUF_Static, info, data);
#endif // (ENGINE_MAJOR_VERSION == 5)

		u16*	indices = (u16*)data;

		indices[0] = 1;
		indices[1] = 3;
		indices[2] = 0;
		indices[3] = 2;
		indices[4] = 3;
		indices[5] = 1;
		indices[6] = 3;
		indices[7] = 5;
		indices[8] = 0;
		indices[9] = 1;
		indices[10] = 4;
		indices[11] = 2;

#if (ENGINE_MAJOR_VERSION == 5)
		RHIUnlockBuffer(IndexBufferRHI);
#else
		RHIUnlockIndexBuffer(IndexBufferRHI);
#endif // (ENGINE_MAJOR_VERSION == 5)
	}
};

TGlobalResource<FPopcornFXGPUParticlesIndexBuffer>	GPopcornFXGPUParticlesIndexBuffer;

//----------------------------------------------------------------------------
//
//	CBatchDrawer_Billboard_GPUBB::SViewDependent
//
//----------------------------------------------------------------------------

void	CBatchDrawer_Billboard_GPUBB::SViewDependent::UnmapBuffers()
{
	m_Indices.Unmap();
}

//----------------------------------------------------------------------------
//
//	CBatchDrawer_Billboard_GPUBB
//
//----------------------------------------------------------------------------

void	CBatchDrawer_Billboard_GPUBB::SViewDependent::ClearBuffers()
{
	m_Indices.UnmapAndClear();
}

//----------------------------------------------------------------------------

CBatchDrawer_Billboard_GPUBB::CBatchDrawer_Billboard_GPUBB()
{
}

//----------------------------------------------------------------------------

CBatchDrawer_Billboard_GPUBB::~CBatchDrawer_Billboard_GPUBB()
{
	_Clear();

	PK_ASSERT(!m_Indices.Valid());
	PK_ASSERT(!m_SimData.Valid());

	for (u32 iView = 0; iView < m_ViewDependents.Count(); ++iView)
		PK_ASSERT(!m_ViewDependents[iView].m_Indices.Valid());

	// Render resources cannot be released on the game thread.
	// When re-importing an effect, render medium gets destroyed on the main thread (expected scenario)
	// When unloading a scene, render mediums are destroyed on the render thread (RenderBatchManager.cpp::Clean())
	if (IsInGameThread())
	{
		ENQUEUE_RENDER_COMMAND(ReleaseRenderResourcesCommand)(
			[this](FRHICommandListImmediate &RHICmdList)
			{
				_CleanRWBuffers();
			});

		FlushRenderingCommands(); // so we can safely release frames
	}
	else
	{
		_CleanRWBuffers();
	}
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Billboard_GPUBB::_CleanRWBuffers()
{
#if RHI_RAYTRACING
	if (IsRayTracingEnabled())
	{
		m_RayTracingGeometry.ReleaseResource();
		m_RayTracingDynamicVertexBuffer.Release();
	}
#endif // RHI_RAYTRACING

	m_DrawIndirectBuffer.Release();
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Billboard_GPUBB::Setup(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleRenderMedium *owner, const PopcornFX::CFrameCollector *fc, const PopcornFX::CStringId &storageClass)
{
	if (!Super::Setup(renderer, owner, fc, storageClass))
		return false;

	m_HasAtlasBlending = renderer->m_RendererCache->m_Flags.m_HasAtlasBlending;

	// Billboarding mode
	{
		const PopcornFX::SRendererDeclaration		&decl = renderer->m_Declaration;
		const PopcornFX::EBillboardMode				billboardingMode = decl.GetPropertyValue_Enum<PopcornFX::EBillboardMode>(PopcornFX::BasicRendererProperties::SID_BillboardingMode(), PopcornFX::BillboardMode_ScreenAligned);

		m_CapsulesDC = billboardingMode == PopcornFX::BillboardMode_AxisAlignedCapsule; // Right now, capsules are not batched with other billboarding modes
	}
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Billboard_GPUBB::AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const
{
	if (!Super::AreRenderersCompatible(rendererA, rendererB))
		return false;
	const CRendererCache	*firstMatCache = static_cast<const CRendererCache*>(rendererA->m_RendererCache.Get());
	const CRendererCache	*secondMatCache = static_cast<const CRendererCache*>(rendererB->m_RendererCache.Get());

	if (firstMatCache == null || secondMatCache == null)
		return false;

	const CMaterialDesc_GameThread	&rDesc0 = firstMatCache->GameThread_Desc();
	const CMaterialDesc_GameThread	&rDesc1 = secondMatCache->GameThread_Desc();
	if (rDesc0.m_RendererMaterial == null || rDesc1.m_RendererMaterial == null)
		return false;

	if (rDesc0.m_RendererClass != rDesc1.m_RendererClass) // We could try batching ribbons with billboards
		return false;
	if (rDesc0.m_CastShadows != rDesc1.m_CastShadows) // We could batch those, but it's easier for later culling if we split them
		return false;
	if (firstMatCache == secondMatCache)
		return true;
	if (rDesc0.m_RendererMaterial == rDesc1.m_RendererMaterial)
		return true;

	PK_ASSERT(rDesc0.m_RendererClass == PopcornFX::Renderer_Billboard);
	const FPopcornFXSubRendererMaterial	*mat0 = rDesc0.m_RendererMaterial->GetSubMaterial(0);
	const FPopcornFXSubRendererMaterial	*mat1 = rDesc1.m_RendererMaterial->GetSubMaterial(0);
	if (mat0 == null || mat1 == null)
		return false;
	if (mat0 == mat1 ||
		mat0->CanBeMergedWith(*mat1))
		return true;
	return false;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Billboard_GPUBB::CanRender(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) const
{
	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);
	CMaterialDesc_RenderThread	&matDesc = static_cast<CRendererCache*>(drawPass.m_RendererCaches.First().Get())->RenderThread_Desc();

	return renderContext.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Main ||
#if RHI_RAYTRACING
		(renderContext.m_RendererSubView->Pass() == CRendererSubView::RenderPass_RT_AccelStructs && matDesc.m_Raytraced) ||
#endif // RHI_RAYTRACING
		(renderContext.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Shadow && matDesc.m_CastShadows);
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Billboard_GPUBB::BeginFrame(PopcornFX::SRenderContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::BeginFrame");

	Super::BeginFrame(ctx);

	INC_DWORD_STAT_BY(STAT_PopcornFX_BatchesCount, 1);

	_Clear();
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Billboard_GPUBB::_Clear()
{
	m_FeatureLevel = ERHIFeatureLevel::Num;

	m_SimDataSRVRef = null;

	m_Indices.UnmapAndClear();
	m_SimData.UnmapAndClear();

	m_DrawRequests.Unmap();

	for (u32 iView = 0; iView < m_ViewDependents.Count(); ++iView)
		m_ViewDependents[iView].ClearBuffers();
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Billboard_GPUBB::_ClearStreamOffsets()
{
	for (u32 iStream = 0; iStream < __SupportedStreamCount; ++iStream)
		m_StreamOffsets[iStream] = PopcornFX::CGuid::INVALID;

	for (u32 iStream = 0; iStream < __SupportedAdditionalStreamCount; ++iStream)
	{
		m_AdditionalStreamOffsets[iStream] = PopcornFX::CGuid::INVALID;
		m_AdditionalStreamIndices[iStream] = PopcornFX::CGuid::INVALID;
	}
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Billboard_GPUBB::_IsAdditionalInputSupported(const PopcornFX::CStringId &fieldName, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets &outStreamOffsetType)
{
	// TODO: PK-RenderHelpers improvement, this doesn't need to be resolved every frame
	if (type == PopcornFX::BaseType_Float4)
	{
		if (fieldName == PopcornFX::BasicRendererProperties::SID_Diffuse_Color() ||
			fieldName == PopcornFX::BasicRendererProperties::SID_Distortion_Color() ||
			fieldName == UERendererProperties::SID_VolumetricFog_AlbedoColor() ||
			fieldName == UERendererProperties::SID_SixWayLightMap_Color())
			outStreamOffsetType = StreamOffset_Colors;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput1_Input1())
			outStreamOffsetType = StreamOffset_DynParam1s;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput2_Input2())
			outStreamOffsetType = StreamOffset_DynParam2s;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput3_Input3())
			outStreamOffsetType = StreamOffset_DynParam3s;
	}
	else if (type == PopcornFX::BaseType_Float3)
	{
		if (fieldName == PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveColor())
			outStreamOffsetType = StreamOffset_EmissiveColors;
	}
	else if (type == PopcornFX::BaseType_Float)
	{
		if (fieldName == PopcornFX::BasicRendererProperties::SID_Atlas_TextureID())
			outStreamOffsetType = StreamOffset_TextureIDs;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_AlphaRemap_Cursor())
			outStreamOffsetType = StreamOffset_AlphaCursors;
	}
	return outStreamOffsetType != EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Billboard_GPUBB::AllocBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::AllocBuffers");

	const bool	resizeBuffers = m_TotalParticleCount != drawPass.m_TotalParticleCount;
	m_TotalParticleCount = drawPass.m_TotalParticleCount;

	if (drawPass.m_RendererType != PopcornFX::Renderer_Billboard)
	{
		PK_ASSERT_NOT_REACHED();
		return false;
	}
#if RHI_RAYTRACING
	if (IsRayTracingEnabled() && !m_RTBuffers_Initialized)
	{
		// Will be resized if necessary
#if (ENGINE_MAJOR_VERSION == 5)
		m_RayTracingDynamicVertexBuffer.Initialize(TEXT("PopcornFXRayTracingDynamicVertexBuffer"), 4, 256, PF_R32_FLOAT, BUF_UnorderedAccess | BUF_ShaderResource);
#else
		m_RayTracingDynamicVertexBuffer.Initialize(4, 256, PF_R32_FLOAT, BUF_UnorderedAccess | BUF_ShaderResource, TEXT("RayTracingDynamicVertexBuffer"));
#endif // (ENGINE_MAJOR_VERSION == 5)

		FRayTracingGeometryInitializer	initializer;
		initializer.IndexBuffer = null;
		initializer.GeometryType = RTGT_Triangles;
		initializer.bFastBuild = true;
		initializer.bAllowUpdate = false; // fully dynamic geom

		m_RayTracingGeometry.SetInitializer(initializer);
		m_RayTracingGeometry.InitResource();

		m_RTBuffers_Initialized = true;
	}
#endif // RHI_RAYTRACING

	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);
	m_FeatureLevel = renderContext.m_RendererSubView->ViewFamily()->GetFeatureLevel();

	m_RealViewCount = renderContext.m_Views.Count();

	// View independent
	const PopcornFX::SGeneratedInputs	&toGenerate = drawPass.m_ToGenerate;

#if (PK_HAS_GPU == 1)
	if (drawPass.m_GPUStorage)
	{
		if (m_DrawIndirectBuffer.Buffer == null ||
			drawPass.m_DrawRequests.Count() > m_DrawIndirectArgsCapacity) // Only grow
		{
			m_DrawIndirectArgsCapacity = drawPass.m_DrawRequests.Count(); // One per draw request
			m_DrawIndirectBuffer.Release();

#if (ENGINE_MAJOR_VERSION == 5)
			m_DrawIndirectBuffer.Initialize(TEXT("PopcornFXGPUDrawIndirectArgs"), sizeof(u32), m_DrawIndirectArgsCapacity * POPCORNFX_INDIRECT_ARGS_ARG_COUNT, EPixelFormat::PF_R32_UINT, BUF_Static | BUF_DrawIndirect);
#else
			m_DrawIndirectBuffer.Initialize(sizeof(u32), m_DrawIndirectArgsCapacity * POPCORNFX_INDIRECT_ARGS_ARG_COUNT, EPixelFormat::PF_R32_UINT, BUF_Static | BUF_DrawIndirect, TEXT("PopcornFXGPUDrawIndirectArgs"));
#endif // (ENGINE_MAJOR_VERSION == 5)
		}
		// TODO: Sort for gpu particles
		if (drawPass.m_IsNewFrame ||
			resizeBuffers) // m_TotalParticleCount can differ between passes, realloc buffers if that's the case
		{
			m_ViewIndependentInputs = toGenerate.m_GeneratedInputs;

			_ClearStreamOffsets();

			// Additional inputs sent to shaders (AlphaRemapCursor, Colors, ..)
			// We know additional input must match for all draw requests (Drawers::SBillboard_BillboardingRequest::IsGeomCompatibleWith)
			// So we'll just flag here the additional inputs that'll need to be filled per draw request
			const u32	aFieldCount = toGenerate.m_AdditionalGeneratedInputs.Count();
			for (u32 iField = 0; iField < aFieldCount; ++iField)
			{
				const PopcornFX::SRendererFeatureFieldDefinition	&additionalInput = toGenerate.m_AdditionalGeneratedInputs[iField];

				const PopcornFX::CStringId			&fieldName = additionalInput.m_Name;
				EPopcornFXAdditionalStreamOffsets	streamOffsetType = EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount;

				if (_IsAdditionalInputSupported(fieldName, additionalInput.m_Type, streamOffsetType))
				{
					PK_ASSERT(streamOffsetType < EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount);
					m_AdditionalStreamIndices[streamOffsetType] = iField; // Flag that stream offset to be filled
				}
			}
		}
	}
	else
#endif // (PK_HAS_GPU == 1)
	{
		CRenderBatchManager	*rbManager = renderContext.m_RenderBatchManager;
		PK_ASSERT(rbManager != null);

		CVertexBufferPool	*mainVBPool = &rbManager->VBPool_VertexBB();
		PK_ASSERT(mainVBPool != null);

		const u32	totalParticleCount = m_TotalParticleCount;

		const bool	needsIndices = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Indices) != 0;
		if (drawPass.m_IsNewFrame ||
			resizeBuffers) // m_TotalParticleCount can differ between passes, realloc buffers if that's the case
		{
			PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::AllocBuffers - New frame");
			_ClearStreamOffsets();

			if (!m_DrawRequests.LoadIFN())
				return false;

			// GPU BB view independent
			m_TotalGPUBufferSize = 0;
			if ((toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_ParticlePosition) != 0)
			{
				m_StreamOffsets[StreamOffset_Positions] = m_TotalGPUBufferSize;
				m_TotalGPUBufferSize += sizeof(CFloat4/*CFloat3 when position w not used*/) * totalParticleCount;
			}
			if ((toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_ParticleSize) != 0)
			{
				m_StreamOffsets[StreamOffset_Sizes] = m_TotalGPUBufferSize;
				m_TotalGPUBufferSize += sizeof(float) * totalParticleCount;
			}
			else if ((toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_ParticleSize2) != 0)
			{
				m_StreamOffsets[StreamOffset_Size2s] = m_TotalGPUBufferSize;
				m_TotalGPUBufferSize += sizeof(CFloat2) * totalParticleCount;
			}
			if ((toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_ParticleRotation) != 0)
			{
				m_StreamOffsets[StreamOffset_Rotations] = m_TotalGPUBufferSize;
				m_TotalGPUBufferSize += sizeof(float) * totalParticleCount;
			}
			if ((toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_ParticleAxis0) != 0)
			{
				m_StreamOffsets[StreamOffset_Axis0s] = m_TotalGPUBufferSize;
				m_TotalGPUBufferSize += sizeof(CFloat3) * totalParticleCount;
			}
			if ((toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_ParticleAxis1) != 0)
			{
				m_StreamOffsets[StreamOffset_Axis1s] = m_TotalGPUBufferSize;
				m_TotalGPUBufferSize += sizeof(CFloat3) * totalParticleCount;
			}

			// Additional inputs sent to shaders (AlphaRemapCursor, Colors, ..)
			const u32	aFieldCount = toGenerate.m_AdditionalGeneratedInputs.Count();

			m_AdditionalInputs.Clear();
			if (!PK_VERIFY(m_AdditionalInputs.Reserve(aFieldCount))) // Max possible additional field count
			{
				UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("GPUBB: Couldn't reserve %d additional inputs"), aFieldCount);
				return false;
			}

			for (u32 iField = 0; iField < aFieldCount; ++iField)
			{
				const PopcornFX::SRendererFeatureFieldDefinition& additionalInput = toGenerate.m_AdditionalGeneratedInputs[iField];

				const PopcornFX::CStringId			&fieldName = additionalInput.m_Name;
				const u32							typeSize = PopcornFX::CBaseTypeTraits::Traits(additionalInput.m_Type).Size;
				EPopcornFXAdditionalStreamOffsets	streamOffsetType = EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount;

				if (_IsAdditionalInputSupported(fieldName, additionalInput.m_Type, streamOffsetType))
				{
					PK_ASSERT(streamOffsetType < EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount);
					m_AdditionalStreamOffsets[streamOffsetType] = m_TotalGPUBufferSize;
				}
				else
					continue; // Unsupported shader input, discard

				if (!PK_VERIFY(m_AdditionalInputs.PushBack().Valid()))
					return false;
				SAdditionalInput	&newAdditionalInput = m_AdditionalInputs.Last();

				newAdditionalInput.m_BufferOffset = m_TotalGPUBufferSize;
				newAdditionalInput.m_ByteSize = typeSize;
				newAdditionalInput.m_AdditionalInputIndex = iField;
				m_TotalGPUBufferSize += typeSize * totalParticleCount;
			}

			if (PK_VERIFY(m_TotalGPUBufferSize > 0))
			{
				const u32	elementCount = m_TotalGPUBufferSize / sizeof(float);
				if (!mainVBPool->Allocate(m_SimData, elementCount, sizeof(float), false))
				{
					UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("GPUBB: Couldn't allocate %d floats for sim data"), elementCount);
					return false;
				}

				PK_TODO("Handle u16 indices");
				if (!mainVBPool->AllocateIf(needsIndices, m_Indices, totalParticleCount, sizeof(u32), false))
				{
					UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("GPUBB: Couldn't allocate %d indices"), totalParticleCount);
					return false;
				}

				// Set the number of view dependent buffers when 'is new frame' being true:
				// Main pass might have sorted indices
				// Shadow passes do not require sorted indices, & is new frame will be false, we don't want to reset m_ViewDependents when that's the case

				// TODO: This shouldn't be a resize but a push back
				if (!PK_VERIFY(m_ViewDependents.Resize(toGenerate.m_PerViewGeneratedInputs.Count())))
					return false;
			}
		}

		const u32	activeViewCount = toGenerate.m_PerViewGeneratedInputs.Count();
		if (activeViewCount > 0)
		{
			PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::AllocBuffers_ViewDependents");

			// View deps
			for (u32 iView = 0; iView < activeViewCount; ++iView)
			{
				const u32	inputFlags = toGenerate.m_PerViewGeneratedInputs[iView].m_GeneratedInputs;
				if (inputFlags == 0)
					continue;

				const bool	viewNeedsIndices = (inputFlags & PopcornFX::Drawers::GenInput_Indices) != 0;

				SViewDependent& viewDep = m_ViewDependents[iView];

				viewDep.m_ViewIndex = toGenerate.m_PerViewGeneratedInputs[iView].m_ViewIndex;
				if (!mainVBPool->AllocateIf(viewNeedsIndices, viewDep.m_Indices, totalParticleCount, sizeof(u32), false))
				{
					UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("GPUBB: Couldn't allocate %d indices for view %d"), totalParticleCount, iView);
					return false;
				}
			}
		}
	}

	m_ProcessViewIndependentData = drawPass.m_IsNewFrame || resizeBuffers;
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Billboard_GPUBB::MapBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass)
{
	if (drawPass.m_GPUStorage)
	{
		// GPU storage, nothing to map
		return true;
	}

	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::MapBuffers_Billboard");

	const u32	totalParticleCount = m_TotalParticleCount;

	// View independent
	if (m_ProcessViewIndependentData)
	{
		if (drawPass.m_ToGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Indices)
		{
			PK_ASSERT(m_Indices.Valid());
			PopcornFX::TMemoryView<u32>	indices;

			PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::MapBuffers_Billboard (view independent indices)");
			if (!m_Indices->Map(indices, totalParticleCount) ||
				!m_BBJobs_Billboard.m_Exec_Indices.m_IndexStream.Setup(indices.Data(), totalParticleCount, true))
			{
				UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("GPUBB: Couldn't map/setup %d indices"), totalParticleCount);
				return false;
			}
		}
		// Map common streams
		if (drawPass.m_ToGenerate.m_GeneratedInputs != 0) // Do we have any view independent streams (& isNewFrame)
		{
			PK_ASSERT(m_SimData.Valid());
			PopcornFX::TMemoryView<float>	simData;
			{
				PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::MapBuffers_Billboard (sim data buffer)");
				const u32	elementCount = m_TotalGPUBufferSize / sizeof(float);
				if (!m_SimData->Map(simData, elementCount))
				{
					UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("GPUBB: Couldn't map %d elements from sim data buffer"), elementCount);
					return false;
				}
			}
			float	*_data = simData.Data();
			{
				if (drawPass.m_ToGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_ParticlePosition)
					m_BBJobs_Billboard.m_Exec_CopyBillboardingStreams.m_PositionsDrIds = PopcornFX::TMemoryView<PopcornFX::Drawers::SVertex_PositionDrId/*CFloat3*/>(reinterpret_cast<PopcornFX::Drawers::SVertex_PositionDrId*>(PopcornFX::Mem::AdvanceRawPointer(_data, m_StreamOffsets[StreamOffset_Positions])), totalParticleCount);
				if (drawPass.m_ToGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_ParticleSize)
					m_BBJobs_Billboard.m_Exec_CopyBillboardingStreams.m_Sizes = PopcornFX::TMemoryView<float>(reinterpret_cast<float*>(PopcornFX::Mem::AdvanceRawPointer(_data, m_StreamOffsets[StreamOffset_Sizes])), totalParticleCount);
				if (drawPass.m_ToGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_ParticleSize2)
					m_BBJobs_Billboard.m_Exec_CopyBillboardingStreams.m_Sizes2 = PopcornFX::TMemoryView<CFloat2>(reinterpret_cast<CFloat2*>(PopcornFX::Mem::AdvanceRawPointer(_data, m_StreamOffsets[StreamOffset_Size2s])), totalParticleCount);
				if (drawPass.m_ToGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_ParticleRotation)
					m_BBJobs_Billboard.m_Exec_CopyBillboardingStreams.m_Rotations = PopcornFX::TMemoryView<float>(reinterpret_cast<float*>(PopcornFX::Mem::AdvanceRawPointer(_data, m_StreamOffsets[StreamOffset_Rotations])), totalParticleCount);
				if (drawPass.m_ToGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_ParticleAxis0)
					m_BBJobs_Billboard.m_Exec_CopyBillboardingStreams.m_Axis0 = PopcornFX::TMemoryView<CFloat3>(reinterpret_cast<CFloat3*>(PopcornFX::Mem::AdvanceRawPointer(_data, m_StreamOffsets[StreamOffset_Axis0s])), totalParticleCount);
				if (drawPass.m_ToGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_ParticleAxis1)
					m_BBJobs_Billboard.m_Exec_CopyBillboardingStreams.m_Axis1 = PopcornFX::TMemoryView<CFloat3>(reinterpret_cast<CFloat3*>(PopcornFX::Mem::AdvanceRawPointer(_data, m_StreamOffsets[StreamOffset_Axis1s])), totalParticleCount);
			}
			{
				// Draw requests
				PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::MapBuffers_Billboard (draw requests buffer)");
				PopcornFX::TMemoryView<PopcornFX::Drawers::SBillboardDrawRequest>	drawRequests;
				if (!m_DrawRequests.Map(drawRequests, drawPass.m_DrawRequests.Count()))
				{
					UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("GPUBB: Couldn't map draw requests buffer"));
					return false;
				}
				m_BBJobs_Billboard.m_Exec_GeomBillboardDrawRequests.m_GeomDrawRequests = drawRequests;
			}

			if (!drawPass.m_ToGenerate.m_AdditionalGeneratedInputs.Empty())
			{
				// Additional inputs
				const u32	aFieldCount = m_AdditionalInputs.Count();

				if (!PK_VERIFY(m_MappedAdditionalInputs.Resize(aFieldCount)))
				{
					UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("GPUBB: Couldn't resize %d mapped additional inputs"), aFieldCount);
					return false;
				}
				for (u32 iField = 0; iField < aFieldCount; ++iField)
				{
					SAdditionalInput					&field = m_AdditionalInputs[iField];
					PopcornFX::Drawers::SCopyFieldDesc	&desc = m_MappedAdditionalInputs[iField];

					desc.m_Storage.m_Count = totalParticleCount;
					desc.m_Storage.m_RawDataPtr = reinterpret_cast<u8*>(PopcornFX::Mem::AdvanceRawPointer(_data, field.m_BufferOffset));
					desc.m_Storage.m_Stride = field.m_ByteSize;
					desc.m_AdditionalInputIndex = field.m_AdditionalInputIndex;
				}

				m_BBJobs_Billboard.m_Exec_CopyAdditionalFields.m_FieldsToCopy = m_MappedAdditionalInputs.View();
			}
		}
	}

	// View deps
	const u32	activeViewCount = m_BBJobs_Billboard.m_PerView.Count();
	PK_ASSERT(activeViewCount == 0 || activeViewCount <= m_ViewDependents.Count()); // <= because m_ViewDependents is only resized for the main pass (isNewFrame). In VR projects, there are two views in main pass, but 1 view for additional passes
	for (u32 iView = 0; iView < activeViewCount; ++iView)
	{
		const u32										viewGeneratedInputs = drawPass.m_ToGenerate.m_PerViewGeneratedInputs[iView].m_GeneratedInputs;
		SViewDependent									&viewDep = m_ViewDependents[iView];
		PopcornFX::SGPUBillboardBatchJobs::SPerView		&dstView = m_BBJobs_Billboard.m_PerView[iView];

		if (viewGeneratedInputs & PopcornFX::Drawers::GenInput_Indices)
		{
			PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::MapBuffers_Billboard (view dependent indices)");

			PK_ASSERT(viewDep.m_Indices.Valid());
			PopcornFX::TMemoryView<u32>	indices;

			if (!viewDep.m_Indices->Map(indices, totalParticleCount) ||
				!dstView.m_Exec_Indices.m_IndexStream.Setup(indices.Data(), totalParticleCount, true))
			{
				UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("GPUBB: Couldn't map/setup %d indices for view %d"), totalParticleCount, iView);
				return false;
			}
		}
		// UVRemaps?
	}
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Billboard_GPUBB::LaunchCustomTasks(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass)
{
	const SUERenderContext																&renderContext = static_cast<SUERenderContext&>(ctx);
	PopcornFX::TMemoryView<const PopcornFX::Drawers::SBillboard_DrawRequest * const>	drawRequests = drawPass.DrawRequests<PopcornFX::Drawers::SBillboard_DrawRequest>();
#if (PK_HAS_GPU != 0)
	if (drawPass.m_GPUStorage) // GPU storage
	{
		using namespace	PopcornFXBillboarder;

		FRHICommandListImmediate			&RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
		const ERHIFeatureLevel::Type		featureLevel = m_FeatureLevel;
		TShaderMapRef< FCopySizeBufferCS >	copySizeBufferCS(GetGlobalShaderMap(featureLevel));

		// stream size buf is already in read state
		SCOPED_DRAW_EVENT(RHICmdList, PopcornFXBillboarder_Billboard_CopySizeBuffer);
		SCOPED_GPU_STAT(RHICmdList, PopcornFXBillboardCopySizeBuffer);

		RHICmdList.Transition(FRHITransitionInfo(m_DrawIndirectBuffer.UAV, ERHIAccess::IndirectArgs, ERHIAccess::UAVCompute));

		for (u32 iDr = 0; iDr < drawRequests.Count(); ++iDr)
		{
			const PopcornFX::Drawers::SBillboard_DrawRequest			&dr = *drawRequests[iDr];
			const PopcornFX::Drawers::SBillboard_BillboardingRequest	&br = dr.m_BB;
			PK_ASSERT(dr.Valid());

			FCSCopySizeBuffer_Params	copyParams;
#if (PK_GPU_D3D11 == 1)
			if (renderContext.m_API == SUERenderContext::D3D11)
			{
				const PopcornFX::CParticleStreamToRender_D3D11	&stream_D3D11 = static_cast<const PopcornFX::CParticleStreamToRender_D3D11&>(dr.StreamToRender());
				copyParams.m_LiveParticleCount = StreamBufferSRVToRHI(&stream_D3D11.StreamSizeBuf(), sizeof(u32) * 4, sizeof(u32), PF_R32_UINT);
			}
#endif
#if (PK_GPU_D3D12 == 1)
			if (renderContext.m_API == SUERenderContext::D3D12)
			{
				const PopcornFX::CParticleStreamToRender_D3D12	&stream_D3D12 = static_cast<const PopcornFX::CParticleStreamToRender_D3D12&>(dr.StreamToRender());
				copyParams.m_LiveParticleCount = StreamBufferSRVToRHI(&stream_D3D12.StreamSizeBuf(), sizeof(u32));
			}
#endif
			copyParams.m_DrawIndirectArgsBuffer = m_DrawIndirectBuffer.UAV;
			copyParams.m_IsCapsule = m_CapsulesDC;
			copyParams.m_DrawIndirectArgsOffset = iDr * POPCORNFX_INDIRECT_ARGS_ARG_COUNT;

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
			SetComputePipelineState(RHICmdList, copySizeBufferCS.GetComputeShader());
#else
			RHICmdList.SetComputeShader(copySizeBufferCS.GetComputeShader());
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
			copySizeBufferCS->Dispatch(RHICmdList, copyParams);
		}

		RHICmdList.Transition(FRHITransitionInfo(m_DrawIndirectBuffer.UAV, ERHIAccess::UAVCompute, ERHIAccess::IndirectArgs));
	}
#endif // (PK_HAS_GPU != 0)
	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::LaunchCustomTasks (map atlas data)");

		// CPU particles, only map atlas data
		// TODO: move that in the renderer cache instead of doing that each frame
		PK_ASSERT(!drawRequests.Empty());
		PK_ASSERT(drawRequests.First() != null);
		const PopcornFX::Drawers::SBillboard_BillboardingRequest	&compatBr = drawRequests.First()->m_BB;

		if (compatBr.m_Atlas != null)
		{
			if (!PK_VERIFY(m_AtlasRects.LoadRects(compatBr.m_Atlas->m_RectsFp32)))
			{
				UE_LOG(LogVertexBillboardingPolicy, Error, TEXT("GPUBB: Couldn't load atlas rects"));
				return false;
			}
			PK_ASSERT(m_AtlasRects.Loaded());
		}
	}
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Billboard_GPUBB::UnmapBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::UnmapBuffers");

	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::MapBuffers_Billboard (unmap view independent indices)");
		m_Indices.Unmap();
	}
	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::MapBuffers_Billboard (unmap sim data)");
		m_SimData.Unmap();
	}

	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::MapBuffers_Billboard (draw requests buffer)");
		m_DrawRequests.Unmap();
	}

	for (u32 iView = 0; iView < m_ViewDependents.Count(); ++iView)
	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::MapBuffers_Billboard (view dependent indices)");
		m_ViewDependents[iView].UnmapBuffers();
	}
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Billboard_GPUBB::_FillDrawCallUniforms_CPU(FPopcornFXGPUVertexFactory			*vertexFactory,
																FPopcornFXUniforms					&vsUniforms,
																FPopcornFXGPUBillboardVSUniforms	&vsUniformsGPUBillboard,
																const PopcornFX::SDrawCallDesc		&desc)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::_FillDrawCallUniforms_CPU");

	// Try and resolve from active views
	SViewDependent	*viewDep = null;
	for (u32 iViewDep = 0; iViewDep < m_ViewDependents.Count(); ++iViewDep)
	{
		if (m_ViewDependents[iViewDep].m_ViewIndex == desc.m_ViewIndex)
		{
			viewDep = &m_ViewDependents[iViewDep];
			break;
		}
	}

	const bool	fullViewIndependent = viewDep == null;
	const bool	viewIndependentIndices = (fullViewIndependent && m_Indices.Valid()) || (!fullViewIndependent && !viewDep->m_Indices.Valid());

	// Assert at least a set of indices is valid.
	// If we have view dependent indices, use those:
	// Main pass will have generated those, while shadow passes do not require sorted indices, we can use the sorted indices directly
	PK_ASSERT(m_Indices.Valid() || (viewDep != null && viewDep->m_Indices.Valid()));

	vsUniforms.InSimData = m_SimData.Buffer()->SRV();

	vsUniformsGPUBillboard.DrawRequestID = -1;
	vsUniformsGPUBillboard.HasSortedIndices = 1; // By default we always generate indices, even when not required. TODO: Fix that to save perf/mem
	vsUniformsGPUBillboard.InIndicesOffset = desc.m_IndexOffset;
	vsUniformsGPUBillboard.InSortedIndices = viewIndependentIndices ? m_Indices.Buffer()->SRV() : viewDep->m_Indices.Buffer()->SRV();
	vsUniformsGPUBillboard.DrawRequest = FVector4f(ForceInitToZero);
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Billboard_GPUBB::_FillDrawCallUniforms_GPU(u32									drId,
																const SUERenderContext				&renderContext,
																FPopcornFXGPUVertexFactory			*vertexFactory,
																FPopcornFXUniforms				&vsUniforms,
																FPopcornFXGPUBillboardVSUniforms	&vsUniformsGPUBillboard,
																const PopcornFX::SDrawCallDesc		&desc)
{
#if (PK_HAS_GPU == 1)
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::_FillDrawCallUniforms_GPU");

	PK_ASSERT(drId < desc.m_DrawRequests.Count());
	PK_ASSERT(desc.m_DrawRequests[drId] != null);
	const PopcornFX::Drawers::SBillboard_DrawRequest			*dr = static_cast<const PopcornFX::Drawers::SBillboard_DrawRequest*>(desc.m_DrawRequests[drId]);
	const PopcornFX::Drawers::SBillboard_BillboardingRequest	&br = dr->m_BB;

	const PopcornFX::CParticleStreamToRender_GPU	*stream = dr->StreamToRender_GPU();
	if (!PK_VERIFY(stream != null))
		return false;

	if ((m_ViewIndependentInputs & PopcornFX::Drawers::GenInput_ParticlePosition) != 0)
		m_StreamOffsets[StreamOffset_Positions] = stream->StreamOffset(br.m_PositionStreamId);
	if ((m_ViewIndependentInputs & PopcornFX::Drawers::GenInput_ParticleSize) != 0)
		m_StreamOffsets[StreamOffset_Sizes] = stream->StreamOffset(br.m_SizeStreamId);
	else if ((m_ViewIndependentInputs & PopcornFX::Drawers::GenInput_ParticleSize2) != 0)
		m_StreamOffsets[StreamOffset_Size2s] = stream->StreamOffset(br.m_SizeStreamId);
	if ((m_ViewIndependentInputs & PopcornFX::Drawers::GenInput_ParticleRotation) != 0)
		m_StreamOffsets[StreamOffset_Rotations] = stream->StreamOffset(br.m_RotationStreamId);
	if ((m_ViewIndependentInputs & PopcornFX::Drawers::GenInput_ParticleAxis0) != 0)
		m_StreamOffsets[StreamOffset_Axis0s] = stream->StreamOffset(br.m_Axis0StreamId);
	if ((m_ViewIndependentInputs & PopcornFX::Drawers::GenInput_ParticleAxis1) != 0)
		m_StreamOffsets[StreamOffset_Axis1s] = stream->StreamOffset(br.m_Axis1StreamId);

	for (u32 iStream = 0; iStream < __SupportedAdditionalStreamCount; ++iStream)
	{
		if (!m_AdditionalStreamIndices[iStream].Valid())
			continue;

		PK_ASSERT(m_AdditionalStreamIndices[iStream] < br.m_AdditionalInputs.Count());
		const u32	streamId = br.m_AdditionalInputs[m_AdditionalStreamIndices[iStream]].m_StreamId;

		m_AdditionalStreamOffsets[iStream] = stream->StreamOffset(streamId);
	}

#if (PK_GPU_D3D11 == 1)
	if (renderContext.m_API == SUERenderContext::D3D11)
	{
		const PopcornFX::CParticleStreamToRender_D3D11	&stream_D3D11 = static_cast<const PopcornFX::CParticleStreamToRender_D3D11&>(dr->StreamToRender());
		m_SimDataSRVRef = StreamBufferSRVToRHI(&stream_D3D11.StreamBuffer(), stream_D3D11.StreamSizeEst(), sizeof(float), PF_R32_UINT);
	}
#endif
#if (PK_GPU_D3D12 == 1)
	if (renderContext.m_API == SUERenderContext::D3D12)
	{
		const PopcornFX::CParticleStreamToRender_D3D12	&stream_D3D12 = static_cast<const PopcornFX::CParticleStreamToRender_D3D12&>(dr->StreamToRender());
		m_SimDataSRVRef = StreamBufferSRVToRHI(&stream_D3D12.StreamBuffer(), sizeof(u32));
	}
#endif
	PK_ASSERT(m_SimDataSRVRef.IsValid());
	vsUniforms.InSimData = m_SimDataSRVRef.GetReference();

	// GPU particles draw requests are not currently batched
	PopcornFX::Drawers::SBillboardDrawRequest	drDesc;
	drDesc.Setup(br);
	vsUniformsGPUBillboard.DrawRequest = *reinterpret_cast<FVector4f*>(&drDesc);
	vsUniformsGPUBillboard.DrawRequestID = drId;

	vsUniformsGPUBillboard.HasSortedIndices = 0; // TODO: Sorting
	vsUniformsGPUBillboard.InIndicesOffset = 0;//desc.m_IndexOffset;
	vsUniformsGPUBillboard.InSortedIndices = vsUniforms.InSimData/*m_DrawRequests.m_DrawRequestsBufferSRV*/;// TODO
#endif // (PK_HAS_GPU == 1)
	return true;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Billboard_GPUBB::_IssueDrawCall_Billboard(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::IssueDrawCall_Billboard");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);

	PK_ASSERT(desc.m_IndexOffset + desc.m_TotalParticleCount <= m_TotalParticleCount);

	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return;
	CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();
	PK_ASSERT(matDesc.ValidForRendering());

	const bool	gpuStorage = desc.m_DrawRequests.First()->GPUStorage();
#if RHI_RAYTRACING
	const bool											isRayTracingPass = view->RenderPass() == CRendererSubView::RenderPass_RT_AccelStructs;
	::TArray<FRayTracingDynamicGeometryUpdateParams>	*dynamicRayTracingGeometries = isRayTracingPass ? view->DynamicRayTracingGeometries() : null; // can be null if !isRayTracingPass
	if (isRayTracingPass)
	{
		PK_ASSERT(dynamicRayTracingGeometries != null);
		if (gpuStorage) // not supported yet
			return;
		if (desc.m_ViewIndex != 0) // Submit once
			return;

		// TODO: Implement slicing with raytracing ?
		if (desc.m_TotalParticleCount != m_TotalParticleCount)
			return;
	}

	FMeshElementCollector		*collector = isRayTracingPass ? view->RTCollector() : view->Collector();
	const u32					viewCount = isRayTracingPass ? 1 : m_RealViewCount; // raytracing data: submit is not per view (yet?)
#else
	FMeshElementCollector		*collector = view->Collector();
	const u32					viewCount = m_RealViewCount;
#endif // RHI_RAYTRACING

	const FPopcornFXSceneProxy	*sceneProxy = view->SceneProxy();
	PK_ASSERT(collector != null);
	PK_ASSERT(sceneProxy != null);

	PK_ASSERT(desc.m_ViewIndex < m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() == m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() <= renderContext.m_RendererSubView->SceneViews().Count());
	for (u32 iView = 0; iView < viewCount; ++iView)
	{
		if (desc.m_ViewIndex != iView)
			continue;

		const u32	realViewIndex = renderContext.m_RendererSubView->BBViews()[iView].m_ViewIndex;
		const u32	drCount = gpuStorage ? desc.m_DrawRequests.Count() : 1;
		for (u32 iDr = 0; iDr < drCount; ++iDr)
		{
			FPopcornFXGPUVertexFactory			*vertexFactory = new FPopcornFXGPUVertexFactory(m_FeatureLevel);
			FPopcornFXGPUBillboardCollector		*collectorRes = &(collector->AllocateOneFrameResource<FPopcornFXGPUBillboardCollector>());
			PK_ASSERT(collectorRes->m_VertexFactory == null);
			collectorRes->m_VertexFactory = vertexFactory;

			FPopcornFXUniforms					vsUniforms;
			FPopcornFXGPUBillboardVSUniforms	vsUniformsGPUBillboard;

#if (PK_HAS_GPU == 1)
			if (gpuStorage)
			{
				if (!_FillDrawCallUniforms_GPU(iDr, renderContext, vertexFactory, vsUniforms, vsUniformsGPUBillboard, desc))
					return;
			}
			else
#endif // (PK_HAS_GPU == 1)
			{
				if (!_FillDrawCallUniforms_CPU(vertexFactory, vsUniforms, vsUniformsGPUBillboard, desc))
					return;
			}

			// Common
			{
				vsUniforms.DynamicParameterMask = matDesc.m_DynamicParameterMask;

				vsUniformsGPUBillboard.CapsulesDC = m_CapsulesDC ? 1 : 0;
				vsUniformsGPUBillboard.HasSecondUVSet = m_HasAtlasBlending ? 1 : 0;
				vsUniformsGPUBillboard.InPositionsOffset = m_StreamOffsets[StreamOffset_Positions].Valid() ? static_cast<s32>(m_StreamOffsets[StreamOffset_Positions] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.InSizesOffset = m_StreamOffsets[StreamOffset_Sizes].Valid() ? static_cast<s32>(m_StreamOffsets[StreamOffset_Sizes] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.InSize2sOffset = m_StreamOffsets[StreamOffset_Size2s].Valid() ? static_cast<s32>(m_StreamOffsets[StreamOffset_Size2s] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.InRotationsOffset = m_StreamOffsets[StreamOffset_Rotations].Valid() ? static_cast<s32>(m_StreamOffsets[StreamOffset_Rotations] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.InAxis0sOffset = m_StreamOffsets[StreamOffset_Axis0s].Valid() ? static_cast<s32>(m_StreamOffsets[StreamOffset_Axis0s] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.InAxis1sOffset = m_StreamOffsets[StreamOffset_Axis1s].Valid() ? static_cast<s32>(m_StreamOffsets[StreamOffset_Axis1s] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.InTextureIDsOffset = m_AdditionalStreamOffsets[StreamOffset_TextureIDs].Valid() ? static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_TextureIDs] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.InColorsOffset = m_AdditionalStreamOffsets[StreamOffset_Colors].Valid() ? static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_Colors] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.InEmissiveColorsOffset = m_AdditionalStreamOffsets[StreamOffset_EmissiveColors].Valid() ? static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_EmissiveColors] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.InAlphaCursorsOffset = m_AdditionalStreamOffsets[StreamOffset_AlphaCursors].Valid() ? static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_AlphaCursors] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.InDynamicParameter1sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam1s].Valid() ? static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_DynParam1s] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.InDynamicParameter2sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam2s].Valid() ? static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_DynParam2s] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.InDynamicParameter3sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam3s].Valid() ? static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_DynParam3s] / sizeof(float)) : -1;
				vsUniformsGPUBillboard.AtlasRectCount = m_AtlasRects.m_AtlasRectsCount;

				if (m_AtlasRects.m_AtlasBufferSRV != null)
					vsUniformsGPUBillboard.AtlasBuffer = m_AtlasRects.m_AtlasBufferSRV;
				else
					vsUniformsGPUBillboard.AtlasBuffer = vsUniforms.InSimData; // Dummy SRV

				if (gpuStorage)
					vsUniformsGPUBillboard.DrawRequests = vsUniforms.InSimData;
				else
					vsUniformsGPUBillboard.DrawRequests = m_DrawRequests.m_DrawRequestsBufferSRV; // Dummy SRV

				vertexFactory->m_VSUniformBuffer = FPopcornFXUniformsRef::CreateUniformBufferImmediate(vsUniforms, UniformBuffer_SingleFrame);
				vertexFactory->m_GPUBillboardVSUniformBuffer = FPopcornFXGPUBillboardVSUniformsRef::CreateUniformBufferImmediate(vsUniformsGPUBillboard, UniformBuffer_SingleFrame);

				PK_ASSERT(!vertexFactory->IsInitialized());
				vertexFactory->InitResource();
			}

			if (!PK_VERIFY(vertexFactory->IsInitialized()))
				return;

#if RHI_RAYTRACING
			FMeshBatch		_meshBatch;
			FMeshBatch		&meshBatch = isRayTracingPass ? _meshBatch : collector->AllocateMesh();

			meshBatch.CastRayTracedShadow = matDesc.m_CastShadows;
			meshBatch.SegmentIndex = 0;
#else
			FMeshBatch		&meshBatch = collector->AllocateMesh();
#endif // RHI_RAYTRACING

			meshBatch.VertexFactory = vertexFactory;
			meshBatch.CastShadow = matDesc.m_CastShadows;
			meshBatch.bUseAsOccluder = false;
			meshBatch.ReverseCulling = sceneProxy->IsLocalToWorldDeterminantNegative();
			meshBatch.Type = PT_TriangleList;
			meshBatch.DepthPriorityGroup = sceneProxy->GetDepthPriorityGroup(renderContext.m_RendererSubView->SceneViews()[realViewIndex].m_SceneView);
			meshBatch.bUseWireframeSelectionColoring = sceneProxy->IsSelected();
			meshBatch.bCanApplyViewModeOverrides = true;
			meshBatch.MaterialRenderProxy = matDesc.RenderProxy();

			FMeshBatchElement	&meshElement = meshBatch.Elements[0];

			meshElement.IndexBuffer = &GPopcornFXGPUParticlesIndexBuffer;
			meshElement.PrimitiveUniformBuffer = sceneProxy->GetUniformBuffer();
			meshElement.FirstIndex = 0;
			meshElement.MinVertexIndex = 0;
			meshElement.MaxVertexIndex = 0;

#if (PK_HAS_GPU == 1)
			if (gpuStorage)
			{
				meshElement.IndirectArgsBuffer = m_DrawIndirectBuffer.Buffer;
				meshElement.IndirectArgsOffset = iDr * POPCORNFX_INDIRECT_ARGS_ARG_COUNT * sizeof(u32);
				meshElement.NumPrimitives = 0; // indirect draw
			}
			else
#endif // (PK_HAS_GPU == 1)
			{
				meshElement.NumInstances = desc.m_TotalParticleCount;
				meshElement.NumPrimitives = m_CapsulesDC ? 4 : 2;
			}

			PK_ASSERT(meshElement.NumPrimitives > 0 || gpuStorage);

#if RHI_RAYTRACING
			if (isRayTracingPass)
			{
				FRayTracingInstance		rayTracingInstance;
				rayTracingInstance.Geometry = &m_RayTracingGeometry;
				rayTracingInstance.InstanceTransforms.Add(FMatrix::Identity);

				INC_DWORD_STAT_BY(STAT_PopcornFX_RayTracing_DrawCallsCount, 1);
				rayTracingInstance.Materials.Add(meshBatch);

				// Cannot use this, hardcoded vertex factory names: RayTracingDynamicGeometry.cpp::IsSupportedDynamicVertexFactoryType()
				const u32	totalVertexCount = m_CapsulesDC ? desc.m_TotalParticleCount * 12 : desc.m_TotalParticleCount * 6;
				const u32	totalTriangleCount = m_CapsulesDC ? desc.m_TotalParticleCount * 4 : desc.m_TotalParticleCount * 2;
				// Update dynamic ray tracing geometry
				dynamicRayTracingGeometries->Add(
					FRayTracingDynamicGeometryUpdateParams
					{
						rayTracingInstance.Materials,
						meshBatch.Elements[0].NumPrimitives == 0, // gpu sim (indirect draw)
						totalVertexCount,
						totalVertexCount * (u32)sizeof(CFloat3), // output from compute shader ?
						totalTriangleCount,
						&m_RayTracingGeometry,
						&m_RayTracingDynamicVertexBuffer
					}
				);

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 1)
				rayTracingInstance.BuildInstanceMaskAndFlags(m_FeatureLevel);
#else
				rayTracingInstance.BuildInstanceMaskAndFlags();
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)
				view->OutRayTracingInstances()->Add(rayTracingInstance);
			}
			else
#endif // RHI_RAYTRACING
			{
				INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsCount, 1);
				INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsMeshCount, 1);

				collector->AddMesh(realViewIndex, meshBatch);
			}
		}
	}
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Billboard_GPUBB::EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass, const PopcornFX::SDrawCallDesc &toEmit)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Billboard_GPUBB::EmitDrawCall");
	PK_ASSERT(toEmit.m_TotalParticleCount <= m_TotalParticleCount); // <= if slicing is enabled
	PK_ASSERT(drawPass.m_RendererType == toEmit.m_Renderer);
	PK_ASSERT(toEmit.m_DrawRequests.First() != null);

	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);

	// !Resolve material proxy and interface for first compatible material
	CRendererCache	*matCache = static_cast<CRendererCache*>(drawPass.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return true;

	CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();

	if (!matDesc.ResolveMaterial(PopcornFX::Drawers::BillboardingLocation_VertexShader))
		return true;
	if (!matDesc.ValidForRendering())
		return true;

#if WITH_EDITOR
	// Can happen in editor if a material is destroyed (Hot reload / auto reimport) between a frame collect and render
	// Will trigger for a single frame until pickup up next update
	CRenderBatchManager	*rbManager = renderContext.m_RenderBatchManager;
	PK_ASSERT(rbManager != null);
	if (!rbManager->RenderThread_CollectedForRendering(matDesc.MaterialInterface()))
		return true;
#endif // WITH_EDITOR

	switch (drawPass.m_RendererType)
	{
	case	PopcornFX::ERendererClass::Renderer_Billboard:
		_IssueDrawCall_Billboard(renderContext, toEmit);
		break;
	default:
		PK_ASSERT_NOT_REACHED();
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------
