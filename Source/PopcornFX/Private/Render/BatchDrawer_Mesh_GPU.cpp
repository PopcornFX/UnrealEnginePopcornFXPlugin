//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "BatchDrawer_Mesh_GPU.h"

#include "RenderBatchManager.h"
#include "SceneManagement.h"
#include "MaterialDesc.h"
#include "StaticMeshResources.h"
#include "ParticleResources.h"
#include "Render/PopcornFXVertexFactoryCommon.h"
#include "Render/PopcornFXMeshVertexFactory.h"

#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "World/PopcornFXSceneProxy.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "Render/PopcornFXShaderUtils.h"
#include "Render/PopcornFXRendererProperties.h"
#include "PopcornFXStats.h"

#include <pk_render_helpers/include/render_features/rh_features_basic.h>
#include <pk_render_helpers/include/render_features/rh_features_vat_static.h>
#include <pk_maths/include/pk_numeric_tools_int.h>

#if (PK_HAS_GPU == 1)
#	include "GPUSim/PopcornFXBillboarderCS.h"
#endif // (PK_HAS_GPU == 1)

DEFINE_LOG_CATEGORY_STATIC(LogGPUMeshPolicy, Log, All);

//----------------------------------------------------------------------------

CBatchDrawer_Mesh_GPUBB::CBatchDrawer_Mesh_GPUBB()
{
}

//----------------------------------------------------------------------------

CBatchDrawer_Mesh_GPUBB::~CBatchDrawer_Mesh_GPUBB()
{
	// Render resources cannot be released on the game thread.
	// When re-importing an effect, render medium gets destroyed on the main thread (expected scenario)
	// When unloading a scene, render mediums are destroyed on the render thread (RenderBatchManager.cpp::Clean())
	if (IsInGameThread())
	{
		ENQUEUE_RENDER_COMMAND(ReleaseRenderResourcesCommand)(
			[this](FRHICommandListImmediate &RHICmdList)
			{
				_CleanBuffers();
			});

		FlushRenderingCommands(); // so we can safely release frames
	}
	else
	{
		_CleanBuffers();
	}
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_GPUBB::_CleanBuffers()
{
	m_BuffersInitialized = false;
	m_LODsConstants_MeshCountsView.SafeRelease();
	m_LODsConstants_MeshOffsetsView.SafeRelease();
	m_DrawIndirectBuffer.UnmapAndClear();
	m_IndirectionBuffer.UnmapAndClear();
	m_IndirectionOffsetsBuffer.UnmapAndClear();
	m_MatricesBuffer.UnmapAndClear();
	m_MatricesOffsetsBuffer.UnmapAndClear();
	m_LODsConstantsBuffer.UnmapAndClear();
	for (u32 iStream = 0; iStream < __SupportedStreamCount; ++iStream)
		m_StreamOffsetsBuffers[iStream].UnmapAndClear();
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_GPUBB::Setup(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleRenderMedium *owner, const PopcornFX::CFrameCollector *fc, const PopcornFX::CStringId &storageClass)
{
	if (!Super::Setup(renderer, owner, fc, storageClass))
		return false;

#if (PK_HAS_GPU == 1)
	CRendererCache	*matCache = static_cast<CRendererCache*>(renderer->m_RendererCache.Get());
	if (!PK_VERIFY(matCache != null))
		return false;

	_PreSetupVFData(matCache->GameThread_Desc().m_StaticMeshRenderData);
#endif // (PK_HAS_GPU == 1)
	return true;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_GPUBB::_PreSetupVFData(const FStaticMeshRenderData *staticMeshRenderData)
{
#if WITH_EDITOR
	m_StaticMeshRenderData = staticMeshRenderData;
#endif

	// Setup mesh data and get total mesh/index count per submesh in each LOD
	const FStaticMeshLODResourcesArray	&LODResourcesArray = staticMeshRenderData->LODResources;
	m_MeshLODCount	= LODResourcesArray.Num();
	m_MeshCount		= 0;
	m_PerLODVFData.Resize(m_MeshLODCount);
	m_PerLODMeshOffset.Reserve(m_MeshLODCount+1);
	for (u32 i = 0; i < m_MeshLODCount; ++i)
	{
		const FStaticMeshLODResources	&LODResources = LODResourcesArray[i];
		_PreSetupMeshData(m_PerLODVFData[i], LODResources);
		m_PerLODMeshOffset.PushBack(m_MeshCount);

		u32	curLODIndexCount = 0;
		for (const FStaticMeshSection &lodSection : LODResources.Sections)
		{
			const u32	indexCount = lodSection.NumTriangles * 3;
			m_PerMeshIndexCount.PushBack(indexCount);
			m_PerMeshIndexOffset.PushBack(curLODIndexCount);
			curLODIndexCount += indexCount;
			++m_MeshCount;
		}
	}

	// Write the total number of submeshes at last LOD + 1, to simplify compute shaders a bit
	m_PerLODMeshOffset.PushBack(m_MeshCount);
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_GPUBB::_PreSetupMeshData(FPopcornFXGPUMeshVertexFactory::FDataType &meshData, const FStaticMeshLODResources &meshResources)
{
	// Note: see Engine/Private/Particles/ParticleSystemRender.cpp -> FDynamicMeshEmitterData::SetupVertexFactory

	meshData.bInitialized = false;

	// Warning: nothing seems to be done with these bindings in vertex factory, this might change in next UE versions
	meshResources.VertexBuffers.PositionVertexBuffer.BindPositionVertexBuffer(null, meshData);
	meshResources.VertexBuffers.StaticMeshVertexBuffer.BindTangentVertexBuffer(null, meshData);
	meshResources.VertexBuffers.StaticMeshVertexBuffer.BindTexCoordVertexBuffer(null, meshData, MAX_TEXCOORDS);
	meshResources.VertexBuffers.ColorVertexBuffer.BindColorVertexBuffer(null, meshData);

	meshData.bInitialized = true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_GPUBB::AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const
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

	if (rDesc0.m_RendererClass != rDesc1.m_RendererClass)
		return false;
	if (rDesc0.m_CastShadows != rDesc1.m_CastShadows) // We could batch those, but it's easier for later culling if we split them
		return false;
	if (firstMatCache == secondMatCache)
		return true;
	if (rDesc0.m_RendererMaterial == rDesc1.m_RendererMaterial)
		return true;

	PK_ASSERT(rDesc0.m_RendererClass == PopcornFX::Renderer_Mesh);
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

bool	CBatchDrawer_Mesh_GPUBB::_IsInputSupported(EPopcornFXStreamOffsets streamOffsetType)
{
	switch (streamOffsetType)
	{
	case StreamOffset_MeshIDs:
		return m_HasMeshIDs;
	case StreamOffset_MeshLODs:
		return m_HasMeshLODs;
	default:
		return streamOffsetType < __SupportedStreamCount;
	}
}

bool	CBatchDrawer_Mesh_GPUBB::_IsAdditionalInputSupported(const PopcornFX::CStringId &fieldName, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets &outStreamOffsetType)
{
	// Node: SID_MeshLOD_LOD additional field is ignored

	if (type == PopcornFX::BaseType_Float4)
	{
		if (fieldName == PopcornFX::BasicRendererProperties::SID_Diffuse_Color() || // Legacy
			fieldName == PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseColor() ||
			fieldName == PopcornFX::BasicRendererProperties::SID_Distortion_Color())
			outStreamOffsetType = StreamOffset_Colors;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput0_Input0())
			outStreamOffsetType = StreamOffset_DynParam0s;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput1_Input1())
			outStreamOffsetType = StreamOffset_DynParam1s;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput2_Input2())
			outStreamOffsetType = StreamOffset_DynParam2s;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput3_Input3())
			outStreamOffsetType = StreamOffset_DynParam3s;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveColor())
			outStreamOffsetType = StreamOffset_EmissiveColors4;
	}
	else if (type == PopcornFX::BaseType_Float3)
	{
		if (fieldName == PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveColor()) // Legacy
			outStreamOffsetType = StreamOffset_EmissiveColors3;
		if (fieldName == PopcornFX::BasicRendererProperties::SID_ComputeVelocity_MoveVector())
			outStreamOffsetType = StreamOffset_Velocity;
	}
	else if (type == PopcornFX::BaseType_Float)
	{
		if (fieldName == PopcornFX::BasicRendererProperties::SID_AlphaRemap_Cursor())
			outStreamOffsetType = StreamOffset_AlphaCursors;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_Atlas_TextureID())
			outStreamOffsetType = StreamOffset_TextureIDs;
		else if (fieldName == PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_Cursor() ||
			fieldName == PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft_Cursor() ||
			fieldName == PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid_Cursor())
			outStreamOffsetType = StreamOffset_VATCursors;
	}
	return outStreamOffsetType != EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_GPUBB::CanRender(PopcornFX::SRenderContext &ctx) const
{
	PK_ASSERT(DrawPass().m_RendererCaches.First() != null);

	const SUERenderContext				&renderContext = static_cast<SUERenderContext&>(ctx);
	const CMaterialDesc_RenderThread	&matDesc = static_cast<CRendererCache*>(DrawPass().m_RendererCaches.First().Get())->RenderThread_Desc();

	const bool	validRenderAPI =	(PK_GPU_D3D12 == 1 && renderContext.m_API == SUERenderContext::D3D12) ||
									(PK_GPU_D3D11 == 1 && renderContext.m_API == SUERenderContext::D3D11);

	const bool	validRenderPass =	renderContext.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Main ||
									(renderContext.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Shadow && matDesc.m_CastShadows) ||
#if RHI_RAYTRACING
									(renderContext.m_RendererSubView->Pass() == CRendererSubView::RenderPass_RT_AccelStructs && matDesc.m_Raytraced);
#endif // RHI_RAYTRACING

	return validRenderAPI && validRenderPass;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_GPUBB::BeginFrame(PopcornFX::SRenderContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_GPUBB::BeginFrame");

	Super::BeginFrame(ctx);

	INC_DWORD_STAT_BY(STAT_PopcornFX_BatchesCount, 1);
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_GPUBB::AllocBuffers(PopcornFX::SRenderContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_GPUBB::AllocBuffers");
	const PopcornFX::SRendererBatchDrawPass &drawPass = DrawPass();

	m_TotalParticleCount	= drawPass.m_TotalParticleCount;

	const u32	drCount		= drawPass.m_DrawRequests.Count();
	const u32	drCountDiff	= drCount - m_DrawRequestCount;
	m_DrawRequestCount		= drCount;

	if (drawPass.m_RendererType != PopcornFX::Renderer_Mesh)
	{
		PK_ASSERT_NOT_REACHED();
		return false;
	}

#if (PK_HAS_GPU == 1)
	if (!drawPass.m_GPUStorage)
		return false;

	const SUERenderContext	&renderContext = static_cast<SUERenderContext&>(ctx);
	CRenderBatchManager		*rbManager = renderContext.m_RenderBatchManager;
	PK_ASSERT(rbManager != null);

	CVertexBufferPool	*VBPoolDrawIndirect	= &rbManager->VBPool_DrawIndirect();
	CVertexBufferPool	*VBPoolRW			= &rbManager->VBPool_GPU();
	CVertexBufferPool	*VBPoolReadonly		= &rbManager->VBPool_VertexBB();
	PK_ASSERT(VBPoolDrawIndirect != null && VBPoolRW != null && VBPoolReadonly != null);

	if (!VBPoolDrawIndirect->AllocateIf(!m_DrawIndirectBuffer.Valid() || drCountDiff > 0, // Only grow
										m_DrawIndirectBuffer,
										drCount * m_MeshCount * FDrawIndexedIndirectArgs::kCount, // One per mesh per draw request
										sizeof(u32)))
		return false;

	if (!VBPoolRW->AllocateIf(	!m_IndirectionOffsetsBuffer.Valid() || drCountDiff > 0, // Only grow
								m_IndirectionOffsetsBuffer,
								2 * drCount * (m_HasMeshIDs ? m_MeshCount : (m_HasMeshLODs ? m_MeshLODCount : 1)), // Two per draw call
								sizeof(u32)))
		return false;
	
	if (!m_IndirectionBuffer.Valid() || !m_MatricesBuffer.Valid() ||
		m_TotalParticleCount > m_ParticleBuffersSize) // Only grow
	{
		// Use vertex buffer policy to compute the exact allocated element count of buffers
		CBufferPool_VertexBufferPolicy	vbPolicy;
		const u32	level	= vbPolicy.SizeToLevel(m_TotalParticleCount);
		const u32	size	= vbPolicy.LevelToSize(FMath::Clamp(level, 0, vbPolicy.kLevelCount-1));
		m_ParticleBuffersSize = size-1;

		if (!VBPoolRW->Allocate(m_IndirectionBuffer, m_ParticleBuffersSize, sizeof(u32)))
			return false;

		// 4x4 matrices are stored in a Buffer<float4> so there are 4 elements per particle
		if (!VBPoolRW->Allocate(m_MatricesBuffer, m_ParticleBuffersSize * 4, sizeof(float) * 4))
			return false;
	}

	if (!m_BuffersInitialized)
	{
		if (!VBPoolReadonly->Allocate(m_MatricesOffsetsBuffer, PK_GPU_MAX_DRAW_REQUEST_COUNT, sizeof(u32)))
			return false;

		// Alloc stream offsets buffers
		for (u32 iStream = 0; iStream < __SupportedStreamCount; ++iStream)
		{
			if (!_IsInputSupported((EPopcornFXStreamOffsets)iStream))
				continue;

			if (!VBPoolReadonly->Allocate(m_StreamOffsetsBuffers[iStream], PK_GPU_MAX_DRAW_REQUEST_COUNT, sizeof(u32)))
				return false;
		}

		// Alloc LOD constant data
		if (m_HasMeshLODs)
		{
			if (!VBPoolReadonly->Allocate(m_LODsConstantsBuffer, m_MeshLODCount * 2 + 1, sizeof(u32)))
				return false;

			m_LODsConstants_MeshCountsView	= m_LODsConstantsBuffer->CreateSubSRV(m_MeshLODCount);
			m_LODsConstants_MeshOffsetsView	= m_LODsConstantsBuffer->CreateSubSRV(m_MeshLODCount + 1, m_MeshLODCount);

			m_ShouldInitLODConsts = true;
		}

		m_BuffersInitialized = true;
	}

	return true;
#else
	return false;
#endif // (PK_HAS_GPU == 1)
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_GPUBB::MapBuffers(PopcornFX::SRenderContext &ctx)
{
#if (PK_HAS_GPU == 1)
	const PopcornFX::SRendererBatchDrawPass &drawPass = DrawPass();
	if (!drawPass.m_GPUStorage)
		return false;

	if (!m_DrawIndirectBuffer->Map(m_DrawIndirectMapped, drawPass.m_DrawRequests.Count() * m_MeshCount))
		return false;

	if (!m_MatricesOffsetsBuffer->Map(m_MatricesOffsetsMapped, PK_GPU_MAX_DRAW_REQUEST_COUNT))
		return false;

	if (m_ShouldInitLODConsts)
	{
		if (!m_LODsConstantsBuffer->Map(m_LODsConstantsMapped, m_MeshLODCount * 2 + 1))
			return false;
		m_ShouldInitLODConsts = false;
	}

	for (u32 iStream = 0; iStream < __SupportedStreamCount; ++iStream)
	{
		if (!_IsInputSupported((EPopcornFXStreamOffsets)iStream))
			continue;

		if (!m_StreamOffsetsBuffers[iStream]->Map(m_StreamOffsetsMapped[iStream], PK_GPU_MAX_DRAW_REQUEST_COUNT))
			return false;
	}

	return true;
#else
	return false;
#endif // (PK_HAS_GPU == 1)
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_GPUBB::LaunchCustomTasks(PopcornFX::SRenderContext &ctx)
{
#if (PK_HAS_GPU == 1)
	const PopcornFX::SRendererBatchDrawPass	&drawPass = DrawPass();
	const u32								drCount = drawPass.m_DrawRequests.Count();

	if (!drawPass.m_GPUStorage)
		return false;

	// Init indirect draw arguments
	if (!m_DrawIndirectMapped.Empty())
	{
		for (u32 iDr = 0; iDr < drCount; iDr++)
		{
			for (u32 iMesh = 0; iMesh < m_MeshCount; iMesh++)
			{
				m_DrawIndirectMapped[m_MeshCount * iDr + iMesh].m_IndexCount = m_PerMeshIndexCount[iMesh];
				m_DrawIndirectMapped[m_MeshCount * iDr + iMesh].m_InstanceCount = 0; // Instance count will be set by compute shader
				m_DrawIndirectMapped[m_MeshCount * iDr + iMesh].m_IndexOffset = m_PerMeshIndexOffset[iMesh];
				m_DrawIndirectMapped[m_MeshCount * iDr + iMesh].m_VertexOffset = 0;
				m_DrawIndirectMapped[m_MeshCount * iDr + iMesh].m_InstanceOffset = 0;
			}
		}
	}

	// Reset indirection offsets
	FRHICommandListExecutor::GetImmediateCommandList().ClearUAVUint(m_IndirectionOffsetsBuffer->UAV(), FUintVector4(0));

	// Setup matrices offsets
	u32	matricesOffset = 0;
	for (u32 iDr = 0; iDr < drCount; iDr++)
	{
		m_MatricesOffsetsMapped[iDr] = matricesOffset;
		const PopcornFX::Drawers::SMesh_DrawRequest	*dr = static_cast<const PopcornFX::Drawers::SMesh_DrawRequest*>(drawPass.m_DrawRequests[iDr]);
		matricesOffset += dr->InputParticleCount() * 4; // 4x4 matrices are stored in a Buffer<float4> so there are 4 elements per particle
	}

	// Init LOD constants buffer: write PerLODMeshCount into the first part of the buffer and PerLODMeshOffset into the second
	if (!m_LODsConstantsMapped.Empty())
	{
		PopcornFX::Mem::Clear_Uncached(m_LODsConstantsMapped.Data(), m_LODsConstantsMapped.CoveredBytes());
		PopcornFX::Mem::CopyT_Uncached(m_LODsConstantsMapped.Data(), m_DrawPass->m_RendererCaches.First()->m_PerLODMeshCount.RawDataPointer(), m_MeshLODCount);
		PopcornFX::Mem::CopyT_Uncached(m_LODsConstantsMapped.Data() + m_MeshLODCount, m_PerLODMeshOffset.RawDataPointer(), m_MeshLODCount + 1);
	}

	// Setup stream offsets
	//	Pos/Scale/Orientation/Enabled [ + MeshIds ] [ + MeshLODs ]
	const u32	streamCount = 4 + (m_HasMeshIDs ? 1 : 0) + (m_HasMeshLODs ? 1 : 0);
	PK_STACKALIGNEDMEMORYVIEW(u32, streamsOffsets, drCount * streamCount, 0x10);
	for (u32 iDr = 0; iDr < drCount; iDr++)
	{
		// Fill in stream offsets for each draw request
		const PopcornFX::Drawers::SMesh_DrawRequest			*dr = static_cast<const PopcornFX::Drawers::SMesh_DrawRequest*>(drawPass.m_DrawRequests[iDr]);
		const PopcornFX::Drawers::SMesh_BillboardingRequest	*bbRequest = static_cast<const PopcornFX::Drawers::SMesh_BillboardingRequest*>(&dr->BaseBillboardingRequest());
		const PopcornFX::CParticleStreamToRender_GPU		*streamToRender = dr->StreamToRender_GPU();
		if (!PK_VERIFY(streamToRender != null))
			continue;

		u32	offset = 0;
		streamsOffsets[offset++ * drCount + iDr] = streamToRender->StreamOffset(bbRequest->m_PositionStreamId);
		streamsOffsets[offset++ * drCount + iDr] = streamToRender->StreamOffset(bbRequest->m_ScaleStreamId);
		streamsOffsets[offset++ * drCount + iDr] = streamToRender->StreamOffset(bbRequest->m_OrientationStreamId);
		streamsOffsets[offset++ * drCount + iDr] = streamToRender->StreamOffset(bbRequest->m_EnabledStreamId);
		if (m_HasMeshIDs)
			streamsOffsets[offset++ * drCount + iDr] = streamToRender->StreamOffset(bbRequest->m_MeshIDStreamId);
		if (m_HasMeshLODs)
			streamsOffsets[offset++ * drCount + iDr] = streamToRender->StreamOffset(bbRequest->m_MeshLODStreamId);
		PK_ASSERT(offset == streamCount);
	}

	// Non temporal writes to gpu mem, aligned and contiguous
	u32	streamOffset = 0;
	PopcornFX::Mem::CopyT_Uncached(m_StreamOffsetsMapped[StreamOffset_Positions].Data(), &streamsOffsets[streamOffset++ * drCount], drCount);
	PopcornFX::Mem::CopyT_Uncached(m_StreamOffsetsMapped[StreamOffset_Scales].Data(), &streamsOffsets[streamOffset++ * drCount], drCount);
	PopcornFX::Mem::CopyT_Uncached(m_StreamOffsetsMapped[StreamOffset_Orientations].Data(), &streamsOffsets[streamOffset++ * drCount], drCount);
	PopcornFX::Mem::CopyT_Uncached(m_StreamOffsetsMapped[StreamOffset_Enableds].Data(), &streamsOffsets[streamOffset++ * drCount], drCount);
	if (m_HasMeshIDs)
		PopcornFX::Mem::CopyT_Uncached(m_StreamOffsetsMapped[StreamOffset_MeshIDs].Data(), &streamsOffsets[streamOffset++ * drCount], drCount);
	if (m_HasMeshLODs)
		PopcornFX::Mem::CopyT_Uncached(m_StreamOffsetsMapped[StreamOffset_MeshLODs].Data(), &streamsOffsets[streamOffset++ * drCount], drCount);

	// Update additional inputs sent to shaders
	{
		for (u32 iStream = 0; iStream < __SupportedAdditionalStreamCount; ++iStream)
			m_AdditionalStreamOffsets[iStream].Reset();

		// We know additional inputs match for all draw requests: just flag those that will need to be filled per draw request
		const u32	aFieldCount = drawPass.m_ToGenerate.m_AdditionalGeneratedInputs.Count();
		for (u32 iField = 0; iField < aFieldCount; ++iField)
		{
			const PopcornFX::SRendererFeatureFieldDefinition	&additionalInput = drawPass.m_ToGenerate.m_AdditionalGeneratedInputs[iField];
			EPopcornFXAdditionalStreamOffsets					streamOffsetType = EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount;

			if (!_IsAdditionalInputSupported(additionalInput.m_Name, additionalInput.m_Type, streamOffsetType))
				continue; // Unsupported shader input, discard

			m_AdditionalStreamOffsets[streamOffsetType].m_ValidInputId = true; // Flag that stream offset to be filled
			m_AdditionalStreamOffsets[streamOffsetType].m_InputId = iField;
		}
	}

	// Map atlas data
	// TODO: move that in the renderer cache instead of doing that each frame
	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_GPUBB::AllocBuffers (map atlas data)");

		PopcornFX::TMemoryView<const PopcornFX::Drawers::SMesh_DrawRequest * const>	drawRequests = drawPass.DrawRequests<PopcornFX::Drawers::SMesh_DrawRequest>();
		const PopcornFX::Drawers::SMesh_BillboardingRequest							&compatBr = drawRequests.First()->m_BB;

		if (compatBr.m_Atlas != null)
		{
			if (!PK_VERIFY(m_AtlasRects.LoadRects(compatBr.m_Atlas->m_RectsFp32)))
				return false;
			PK_ASSERT(m_AtlasRects.Loaded());
		}
	}

	return true;
#else
	return false;
#endif // (PK_HAS_GPU == 1)
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_GPUBB::UnmapBuffers(PopcornFX::SRenderContext &ctx)
{
#if (PK_HAS_GPU == 1)
	if (!DrawPass().m_GPUStorage)
		return false;

	if (!m_DrawIndirectMapped.Empty())
		m_DrawIndirectBuffer.Unmap();

	if (!m_MatricesOffsetsMapped.Empty())
		m_MatricesOffsetsBuffer.Unmap();

	if (!m_LODsConstantsMapped.Empty())
		m_LODsConstantsBuffer.Unmap();

	for (u32 iStream = 0; iStream < __SupportedStreamCount; ++iStream)
	{
		if (!_IsInputSupported((EPopcornFXStreamOffsets)iStream))
			continue;

		if (!m_StreamOffsetsMapped[iStream].Empty())
			m_StreamOffsetsBuffers[iStream].Unmap();
	}

	m_DrawIndirectMapped.Clear();
	m_MatricesOffsetsMapped.Clear();
	m_LODsConstantsMapped.Clear();
	PopcornFX::Mem::Clear(m_StreamOffsetsMapped);

	return true;
#else
	return false;
#endif // (PK_HAS_GPU == 1)
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_GPUBB::_BuildMeshBatchElement(	FMeshBatchElement				&batchElement,
															const STransientDrawData		&drawData,
															const FStaticMeshLODResources	&LODResources)
{
	const FStaticMeshSection	&section = LODResources.Sections[drawData.iSection];
	if (!PK_VERIFY(section.NumTriangles > 0))
		return false;

	batchElement.FirstIndex		= section.FirstIndex;
	batchElement.MinVertexIndex	= section.MinVertexIndex;
	batchElement.MaxVertexIndex	= section.MaxVertexIndex;
	batchElement.IndexBuffer	= &LODResources.IndexBuffer;

	batchElement.IndirectArgsBuffer = m_DrawIndirectBuffer->GetRHI();
	batchElement.IndirectArgsOffset = (drawData.iDr * m_MeshCount + drawData.iMesh) * sizeof(FDrawIndexedIndirectArgs);

	// Set to 0 for IndirectArgsBuffer to be used
	batchElement.NumPrimitives	= 0;
	batchElement.NumInstances	= 0;

	// Used to set MeshBatchElementId in PopcornFXGPUMeshVertexFactory.ush
	// Bound in function FPopcornFXGPUMeshVertexFactoryShaderParameters::GetElementShaderBindings
	batchElement.UserIndex = drawData.iSection;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	batchElement.VisualizeElementIndex = drawData.iSection;
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_GPUBB::_BuildMeshBatch(	FMeshBatch							&meshBatch,
													const FPopcornFXSceneProxy			*sceneProxy,
													const FSceneView					*view,
													const CMaterialDesc_RenderThread	&matDesc,
													const STransientDrawData			&drawData)
{
	PK_ASSERT(sceneProxy != null);

	meshBatch.LCI = NULL;
	meshBatch.bUseAsOccluder = false;
	meshBatch.ReverseCulling = sceneProxy->IsLocalToWorldDeterminantNegative();
	meshBatch.CastShadow = matDesc.m_CastShadows;

#if RHI_RAYTRACING
	meshBatch.CastRayTracedShadow = matDesc.m_CastShadows;
#endif // RHI_RAYTRACING

	meshBatch.DepthPriorityGroup = sceneProxy->GetDepthPriorityGroup(view);
	meshBatch.Type = PT_TriangleList;
	meshBatch.MaterialRenderProxy = matDesc.RenderProxy()[0]; // TODO: Check with v1 impl.
	meshBatch.bCanApplyViewModeOverrides = true;
	meshBatch.LODIndex = drawData.iLOD;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	meshBatch.VisualizeLODIndex = drawData.iLOD;
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	return true;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_GPUBB::_CreateMeshVertexFactory(	const CMaterialDesc_RenderThread	&matDesc,
															const STransientDrawData			&drawData,
															FMeshElementCollector				*collector,
															FPopcornFXGPUMeshVertexFactory		*&outFactory,
															FPopcornFXGPUMeshVertexCollector	*&outCollectorRes)
{
	FPopcornFXUniforms			vsUniforms;
	FPopcornFXMeshVSUniforms	vsUniformsMesh;
	FPopcornFXGPUMeshVSUniforms	vsUniformsGPUMesh;

	vsUniforms.InSimData = drawData.simData;
	vsUniforms.DynamicParameterMask = matDesc.m_DynamicParameterMask;
	
	vsUniformsMesh.AtlasRectCount = m_AtlasRects.m_AtlasRectsCount;
	if (m_AtlasRects.m_AtlasBufferSRV != null)
		vsUniformsMesh.AtlasBuffer = m_AtlasRects.m_AtlasBufferSRV;
	else
		vsUniformsMesh.AtlasBuffer = vsUniforms.InSimData; // Dummy SRV

	vsUniformsMesh.InAlphaCursorsOffset =		m_AdditionalStreamOffsets[StreamOffset_AlphaCursors].OffsetForShaderConstant();
	vsUniformsMesh.InTextureIDsOffset =			m_AdditionalStreamOffsets[StreamOffset_TextureIDs].OffsetForShaderConstant();
	vsUniformsMesh.InVATCursorsOffset =			m_AdditionalStreamOffsets[StreamOffset_VATCursors].OffsetForShaderConstant();

	vsUniformsMesh.InEmissiveColorsOffset3 =	m_AdditionalStreamOffsets[StreamOffset_EmissiveColors3].OffsetForShaderConstant();
	vsUniformsMesh.InEmissiveColorsOffset4 =	m_AdditionalStreamOffsets[StreamOffset_EmissiveColors4].OffsetForShaderConstant();

	vsUniformsMesh.InColorsOffset =				m_AdditionalStreamOffsets[StreamOffset_Colors].OffsetForShaderConstant();
	vsUniformsMesh.InVelocityOffset =			m_AdditionalStreamOffsets[StreamOffset_Velocity].OffsetForShaderConstant();
	vsUniformsMesh.InDynamicParameter0sOffset =	m_AdditionalStreamOffsets[StreamOffset_DynParam0s].OffsetForShaderConstant();
	vsUniformsMesh.InDynamicParameter1sOffset =	m_AdditionalStreamOffsets[StreamOffset_DynParam1s].OffsetForShaderConstant();
	vsUniformsMesh.InDynamicParameter2sOffset =	m_AdditionalStreamOffsets[StreamOffset_DynParam2s].OffsetForShaderConstant();
	vsUniformsMesh.InDynamicParameter3sOffset =	m_AdditionalStreamOffsets[StreamOffset_DynParam3s].OffsetForShaderConstant();
	
	vsUniformsGPUMesh.DrawRequest = drawData.iDr;

	// The indirection offsets buffer has two halves, each with one element per draw call
	// This index corresponds to the first draw call of the current draw request
	// We add nDr to index into the 2nd half of the buffer
	vsUniformsGPUMesh.IndirectionOffsetsIndex = drawData.iDr + drawData.nDr;

	// With mesh IDs, draw calls are split per mesh section (which includes LODs)
	if (m_HasMeshIDs)
	{
		vsUniformsGPUMesh.IndirectionOffsetsIndex *= m_MeshCount;
		vsUniformsGPUMesh.IndirectionOffsetsIndex += drawData.iMesh;
	}

	// With mesh LODs (not using mesh IDs), draw calls are split per LOD
	else if (m_HasMeshLODs)
	{
		vsUniformsGPUMesh.IndirectionOffsetsIndex *= drawData.nLOD;
		vsUniformsGPUMesh.IndirectionOffsetsIndex += drawData.iLOD;
	}

	vsUniformsGPUMesh.Indirection			= m_IndirectionBuffer->SRV();
	vsUniformsGPUMesh.IndirectionOffsets	= m_IndirectionOffsetsBuffer->SRV();
	vsUniformsGPUMesh.Matrices				= m_MatricesBuffer->SRV();
	vsUniformsGPUMesh.MatricesOffsets		= m_MatricesOffsetsBuffer->SRV();

	// Create the vertex factory
	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_GPUBB::IssueDrawCall_Mesh CollectorRes");

		outCollectorRes = &(collector->AllocateOneFrameResource<FPopcornFXGPUMeshVertexCollector>());
		check(outCollectorRes != null);

		PK_ASSERT(outCollectorRes->m_VertexFactory == null);
		outCollectorRes->m_VertexFactory = new FPopcornFXGPUMeshVertexFactory(drawData.featureLevel);

		outFactory = outCollectorRes->m_VertexFactory;
		outFactory->SetData(m_PerLODVFData[drawData.iLOD]);
		outFactory->m_VSUniformBuffer = FPopcornFXUniformsRef::CreateUniformBufferImmediate(vsUniforms, UniformBuffer_SingleFrame);
		outFactory->m_MeshVSUniformBuffer = FPopcornFXMeshVSUniformsRef::CreateUniformBufferImmediate(vsUniformsMesh, UniformBuffer_SingleFrame);
		outFactory->m_GPUMeshVSUniformBuffer = FPopcornFXGPUMeshVSUniformsRef::CreateUniformBufferImmediate(vsUniformsGPUMesh, UniformBuffer_SingleFrame);
		PK_ASSERT(!outFactory->IsInitialized());
		outFactory->InitResource(FRHICommandListExecutor::GetImmediateCommandList());
	}
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_GPUBB::_IssueDrawCall_Mesh_Sections(	const FPopcornFXSceneProxy		*sceneProxy,
																const SUERenderContext			&renderContext,
																const PopcornFX::SDrawCallDesc	&desc,
																CMaterialDesc_RenderThread		&matDesc,
																STransientDrawData				drawData,
																FMeshElementCollector			*collector)
{
	const FStaticMeshRenderData	*renderData = matDesc.m_StaticMeshRenderData;
	CRendererSubView			*view = renderContext.m_RendererSubView;
	const u32					viewCount = renderContext.m_Views.Count();

	PK_ASSERT(view != null);
	PK_ASSERT(renderData != null);
	PK_ASSERT(desc.m_ViewIndex < viewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() == viewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() <= renderContext.m_RendererSubView->SceneViews().Count());
	PK_ASSERT(drawData.iLOD < (u32)renderData->LODResources.Num());

	const FStaticMeshLODResources	&LODResources = renderData->LODResources[drawData.iLOD];
	const u32						curLODMeshOffset = drawData.iMesh;

	// Create vertex factory and collector resource used to render all sections
	FPopcornFXGPUMeshVertexFactory		*vertexFactory = null;
	FPopcornFXGPUMeshVertexCollector	*collectorRes = null;
	_CreateMeshVertexFactory(matDesc, drawData, collector, vertexFactory, collectorRes);

	for (u32 iView = 0; iView < viewCount; ++iView)
	{
		if (desc.m_ViewIndex != iView)
			continue;

		PK_ASSERT(view->RenderPass() != CRendererSubView::RenderPass_Shadow || matDesc.m_CastShadows);
		const u32	realViewIndex = renderContext.m_RendererSubView->BBViews()[iView].m_ViewIndex;
		const u32	sectionCount = LODResources.Sections.Num();

		// Create and build mesh batch that will contain all sections
		FMeshBatch	&meshBatch = collector->AllocateMesh();
		if (!PK_VERIFY(_BuildMeshBatch(meshBatch, sceneProxy, view->SceneViews()[realViewIndex].m_SceneView, matDesc, drawData)))
			continue;
		meshBatch.VertexFactory = vertexFactory;
		meshBatch.Elements.SetNum(0); // Elements is initialized with Num=1
		meshBatch.Elements.Reserve(sectionCount);

		for (u32 iSection = 0; iSection < sectionCount; ++iSection)
		{
			drawData.iSection = iSection;
			drawData.iMesh = curLODMeshOffset + iSection;

			// Create and build mesh batch element for current section
			FMeshBatchElement batchElem;
			if (!PK_VERIFY(_BuildMeshBatchElement(batchElem, drawData, LODResources)))
				continue;
			batchElem.PrimitiveUniformBuffer = sceneProxy->GetUniformBuffer();
			meshBatch.Elements.Add(batchElem);

			INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsCount, 1);
			INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsMeshCount, 1);
		}

		collector->AddMesh(realViewIndex, meshBatch);
	}
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_GPUBB::_IssueDrawCall_Mesh(const SUERenderContext & renderContext, const PopcornFX::SDrawCallDesc & desc)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_GPUBB::_IssueDrawCall_Mesh");
	using namespace	PopcornFXBillboarder;

	PK_ASSERT(desc.m_IndexOffset + desc.m_TotalParticleCount <= m_TotalParticleCount);

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);

	FMeshElementCollector		*collector = view->Collector();
	const FPopcornFXSceneProxy	*sceneProxy = view->SceneProxy();
	PK_ASSERT(collector != null);
	PK_ASSERT(sceneProxy != null);

	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return false;
	CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();
	if (!PK_VERIFY(matDesc.ValidForRendering()))
		return false;

	const FStaticMeshRenderData	*renderData = matDesc.m_StaticMeshRenderData;
	if (!PK_VERIFY(renderData != null && renderData->IsInitialized()))
		return false;
	PK_ASSERT(matDesc.m_BaseLODLevel < (u32)renderData->LODResources.Num());

#if WITH_EDITOR
	// If our mesh in cache is different from the one in mat desc, it was modified and we need to re-do the setup
	if (m_StaticMeshRenderData != renderData)
	{
		_CleanBuffers();
		_PreSetupVFData(renderData);
		return false; // Buffers will be re-allocated on the next frame
	}
#endif

	// Set the transient compute data used by the SubmitComputeShaders method
	const CParticleScene			*particleScene	= &renderContext.m_RenderBatchManager->ParticleScene();
	const ERHIFeatureLevel::Type	featureLevel	= renderContext.m_RendererSubView->ViewFamily()->GetFeatureLevel();
	const u32						drCount			= desc.m_DrawRequests.Count();
	STransientComputeData	computeData = STransientComputeData(featureLevel, drCount, particleScene);

	// Iterate on draw requests
	for (u32 iDr = 0; iDr < drCount; ++iDr)
	{
		const PopcornFX::Drawers::SMesh_DrawRequest			*dr = static_cast<const PopcornFX::Drawers::SMesh_DrawRequest*>(desc.m_DrawRequests[iDr]);
		if (!PK_VERIFY(dr->Valid()))
			continue;

		const PopcornFX::Drawers::SMesh_BillboardingRequest	*bbRequest = static_cast<const PopcornFX::Drawers::SMesh_BillboardingRequest*>(&dr->BaseBillboardingRequest());
		const PopcornFX::CParticleStreamToRender_GPU		*streamToRender = dr->StreamToRender_GPU();
		if (!PK_VERIFY(streamToRender != null))
			continue;

		// Get the simulation data and live particle count buffers from the draw request stream
		FShaderResourceViewRHIRef	simData = null;
		FShaderResourceViewRHIRef	liveParticleCount = null;
#if (PK_GPU_D3D12 == 1)
		if (renderContext.m_API == SUERenderContext::D3D12)
		{
			const PopcornFX::CParticleStreamToRender_D3D12	&stream_D3D12 = static_cast<const PopcornFX::CParticleStreamToRender_D3D12&>(*streamToRender);
			simData				= StreamBufferSRVToRHI(&stream_D3D12.StreamBuffer(), sizeof(float));
			liveParticleCount	= StreamBufferSRVToRHI(&stream_D3D12.StreamSizeBuf(), sizeof(u32));
		}
#endif // (PK_GPU_D3D12 == 1)
#if (PK_GPU_D3D11 == 1)
		if (renderContext.m_API == SUERenderContext::D3D11)
		{
			const PopcornFX::CParticleStreamToRender_D3D11	&stream_D3D11 = static_cast<const PopcornFX::CParticleStreamToRender_D3D11&>(*streamToRender);
			simData				= StreamBufferSRVToRHI(&stream_D3D11.StreamBuffer(), sizeof(float), PF_R32_UINT /* debug layer errors if not specified correctly */);
			liveParticleCount	= StreamBufferSRVToRHI(&stream_D3D11.StreamSizeBuf(), sizeof(u32), PF_R32_UINT /* debug layer errors if not specified correctly */);
		}
#endif // (PK_GPU_D3D11 == 1)
		if (!PK_VERIFY(simData != null && liveParticleCount != null))
			continue;

		// Set transient compute data for the current draw request
		computeData.m_PerDrParticleCount[iDr]			= dr->InputParticleCount();
		computeData.m_PerDrSimDataSRV[iDr]				= simData;
		computeData.m_PerDrLiveParticleCountSRV[iDr]	= liveParticleCount;

		// Get additional stream offsets for each draw request
		for (u32 iStream = 0; iStream < __SupportedAdditionalStreamCount; ++iStream)
		{
			if (!m_AdditionalStreamOffsets[iStream].m_ValidInputId)
				continue;

			PK_ASSERT(m_AdditionalStreamOffsets[iStream].m_InputId < bbRequest->m_AdditionalInputs.Count());
			const u32	streamId = bbRequest->m_AdditionalInputs[m_AdditionalStreamOffsets[iStream].m_InputId].m_StreamId;

			m_AdditionalStreamOffsets[iStream].m_OffsetInBytes = streamToRender->StreamOffset(streamId);
			m_AdditionalStreamOffsets[iStream].m_ValidOffset = true;
		}

		// Draw call: indirect rendering per submesh
		const u32	LODFirst = matDesc.m_PerParticleLOD ? matDesc.m_BaseLODLevel					: 0;
		const u32	LODCount = matDesc.m_PerParticleLOD ? m_MeshLODCount - matDesc.m_BaseLODLevel	: 1;
		for (u32 iLOD = LODFirst; iLOD < LODCount; ++iLOD)
		{
			const STransientDrawData	drawData = { drCount, m_MeshLODCount, iDr, iLOD, m_PerLODMeshOffset[iLOD], 0, featureLevel, simData};
			_IssueDrawCall_Mesh_Sections(sceneProxy, renderContext, desc, matDesc, drawData, collector);
		}
	}

	return _DispatchComputeShaders(computeData);
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_GPUBB::_DispatchComputeShaders(const STransientComputeData &computeData)
{
	using namespace PopcornFXBillboarder;

	// Make sure necessary data has been set by IssueDrawCall
	if (!computeData.Valid())
		return false;

	FRHICommandListImmediate	&RHICmdList = FRHICommandListImmediate::Get();
	FGlobalShaderMap			*globalShaderMap = GetGlobalShaderMap(computeData.m_FeatureLevel);
	RHICmdList.Transition(FRHITransitionInfo(m_DrawIndirectBuffer->UAV(), ERHIAccess::IndirectArgs, ERHIAccess::IndirectArgs));

	// Compute dispatch: compute particle count per mesh
	for (u32 iDr = 0; iDr < m_DrawRequestCount; ++iDr)
	{
		FShaderResourceViewRHIRef	simData				= computeData.m_PerDrSimDataSRV[iDr];
		FShaderResourceViewRHIRef	liveParticleCount	= computeData.m_PerDrLiveParticleCountSRV[iDr];
		if (simData == null || liveParticleCount == null)
			continue;

		FParticleCountPerMeshCS::FPermutationDomain permutation;
		permutation.Set<CSPermutations::FHasMeshIDs>(m_HasMeshIDs);
		permutation.Set<CSPermutations::FHasMeshLODs>(m_HasMeshLODs);
		TShaderMapRef<FParticleCountPerMeshCS>	particleCountPerMeshCS(globalShaderMap, permutation);
		FParticleCountPerMeshCS::FParameters	particleCountPerMeshParams;

		particleCountPerMeshParams.DrawRequest			= iDr;
		particleCountPerMeshParams.MeshCount			= m_MeshCount;
		particleCountPerMeshParams.LODCount				= m_MeshLODCount;
		particleCountPerMeshParams.InSimData			= simData;
		particleCountPerMeshParams.LiveParticleCount	= liveParticleCount;
		particleCountPerMeshParams.EnabledsOffsets		= m_StreamOffsetsBuffers[StreamOffset_Enableds]->SRV();
		particleCountPerMeshParams.MeshIDsOffsets		= m_HasMeshIDs ? m_StreamOffsetsBuffers[StreamOffset_MeshIDs]->SRV() : nullptr;
		particleCountPerMeshParams.LODsOffsets			= m_HasMeshLODs ? m_StreamOffsetsBuffers[StreamOffset_MeshLODs]->SRV() : nullptr;
		particleCountPerMeshParams.PerLODMeshCounts		= m_LODsConstants_MeshCountsView;
		particleCountPerMeshParams.PerLODMeshOffsets	= m_LODsConstants_MeshOffsetsView;
		particleCountPerMeshParams.OutDrawIndirect		= m_DrawIndirectBuffer->UAV();

		particleCountPerMeshCS->Dispatch(RHICmdList, particleCountPerMeshCS, particleCountPerMeshParams, computeData.m_PerDrParticleCount[iDr]);
	}

	RHICmdList.Transition(FRHITransitionInfo(m_DrawIndirectBuffer->UAV(), ERHIAccess::IndirectArgs, ERHIAccess::IndirectArgs));
	RHICmdList.Transition(FRHITransitionInfo(m_IndirectionOffsetsBuffer->UAV(), ERHIAccess::UAVMask, ERHIAccess::UAVMask));

	// Compute dispatch: init indirection offsets buffer (one dispatch that handles every DR and DC)
	{
		FIndirectionOffsetsBufferCS::FPermutationDomain permutation;
		permutation.Set<CSPermutations::FHasMeshLODsNoIDs>(m_HasMeshLODs && !m_HasMeshIDs);
		TShaderMapRef<FIndirectionOffsetsBufferCS>	indirectionOffsetsBufferCS(globalShaderMap, permutation);
		FIndirectionOffsetsBufferCS::FParameters	indirectionOffsetsBufferParams;

		indirectionOffsetsBufferParams.DrawCallCount			= m_DrawRequestCount * (m_HasMeshIDs ? m_MeshCount : (m_HasMeshLODs ? m_MeshLODCount : 1));
		indirectionOffsetsBufferParams.DrawIndirect				= m_DrawIndirectBuffer->SRV();
		indirectionOffsetsBufferParams.PerLODMeshOffsets		= m_LODsConstants_MeshOffsetsView;
		indirectionOffsetsBufferParams.OutIndirectionOffsets	= m_IndirectionOffsetsBuffer->UAV();

		indirectionOffsetsBufferCS->Dispatch(RHICmdList, indirectionOffsetsBufferCS, indirectionOffsetsBufferParams);
	}

	RHICmdList.Transition(FRHITransitionInfo(m_IndirectionOffsetsBuffer->UAV(), ERHIAccess::UAVMask, ERHIAccess::UAVMask));
	
	// Compute dispatch: mesh indirection buffers
	for (u32 iDr = 0; iDr < m_DrawRequestCount; ++iDr)
	{
		FShaderResourceViewRHIRef	simData				= computeData.m_PerDrSimDataSRV[iDr];
		FShaderResourceViewRHIRef	liveParticleCount	= computeData.m_PerDrLiveParticleCountSRV[iDr];
		if (simData == null || liveParticleCount == null)
			continue;

		FMeshIndirectionBufferCS::FPermutationDomain permutation;
		permutation.Set<CSPermutations::FHasMeshIDs>(m_HasMeshIDs);
		permutation.Set<CSPermutations::FHasMeshLODs>(m_HasMeshLODs);
		TShaderMapRef<FMeshIndirectionBufferCS>	meshIndirectionBufferCS(globalShaderMap, permutation);
		FMeshIndirectionBufferCS::FParameters	meshIndirectionBufferParams;

		meshIndirectionBufferParams.DrawRequest				= iDr;
		meshIndirectionBufferParams.MeshCount				= m_MeshCount;
		meshIndirectionBufferParams.LODCount				= m_MeshLODCount;
		meshIndirectionBufferParams.InSimData				= simData;
		meshIndirectionBufferParams.LiveParticleCount		= liveParticleCount;
		meshIndirectionBufferParams.EnabledsOffsets			= m_StreamOffsetsBuffers[StreamOffset_Enableds]->SRV();
		meshIndirectionBufferParams.MeshIDsOffsets			= m_HasMeshIDs ? m_StreamOffsetsBuffers[StreamOffset_MeshIDs]->SRV() : nullptr;
		meshIndirectionBufferParams.LODsOffsets				= m_HasMeshLODs ? m_StreamOffsetsBuffers[StreamOffset_MeshLODs]->SRV() : nullptr;
		meshIndirectionBufferParams.PerLODMeshCounts		= m_LODsConstants_MeshCountsView;
		meshIndirectionBufferParams.PerLODMeshOffsets		= m_LODsConstants_MeshOffsetsView;
		meshIndirectionBufferParams.OutIndirectionOffsets	= m_IndirectionOffsetsBuffer->UAV();
		meshIndirectionBufferParams.OutIndirection			= m_IndirectionBuffer->UAV();

		meshIndirectionBufferCS->Dispatch(RHICmdList, meshIndirectionBufferCS, meshIndirectionBufferParams, computeData.m_PerDrParticleCount[iDr]);
	}

	RHICmdList.Transition(FRHITransitionInfo(m_IndirectionBuffer->UAV(), ERHIAccess::UAVMask, ERHIAccess::UAVMask));
	
	// Compute dispatch: mesh matrices
	for (u32 iDr = 0; iDr < m_DrawRequestCount; ++iDr)
	{
		FShaderResourceViewRHIRef	simData				= computeData.m_PerDrSimDataSRV[iDr];
		FShaderResourceViewRHIRef	liveParticleCount	= computeData.m_PerDrLiveParticleCountSRV[iDr];
		if (simData == null || liveParticleCount == null)
			continue;

		TShaderMapRef<FMeshMatricesCS>	meshMatricesCS(globalShaderMap);
		FMeshMatricesCS::FParameters	meshMatricesParams;

		meshMatricesParams.DrawRequest			= iDr;
		meshMatricesParams.InSimData			= simData;
		meshMatricesParams.LiveParticleCount	= liveParticleCount;
		meshMatricesParams.PositionsOffsets		= m_StreamOffsetsBuffers[StreamOffset_Positions]->SRV();
		meshMatricesParams.ScalesOffsets		= m_StreamOffsetsBuffers[StreamOffset_Scales]->SRV();
		meshMatricesParams.OrientationsOffsets	= m_StreamOffsetsBuffers[StreamOffset_Orientations]->SRV();
		meshMatricesParams.MatricesOffsets		= m_MatricesOffsetsBuffer->SRV();
		meshMatricesParams.OutMatrices			= m_MatricesBuffer->UAV();
		
		meshMatricesCS->Dispatch(RHICmdList, meshMatricesCS, meshMatricesParams, computeData.m_PerDrParticleCount[iDr]);
	}

	RHICmdList.Transition(FRHITransitionInfo(m_MatricesBuffer->UAV(), ERHIAccess::UAVMask, ERHIAccess::UAVMask));
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_GPUBB::EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SDrawCallDesc &toEmit)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_GPUBB::EmitDrawCall");
	PK_ASSERT(toEmit.m_TotalParticleCount <= m_TotalParticleCount); // <= if slicing is enabled
	PK_ASSERT(m_DrawPass->m_RendererType == toEmit.m_Renderer);
	
	// !Currently, there is no batching for gpu particles!
	// So we'll emit one draw call per draw request
	// Some data are indexed by the draw-requests. So "EmitDrawCall" should be called once, with all draw-requests.
	PK_ASSERT(toEmit.m_DrawRequests.Count() == DrawPass().m_DrawRequests.Count());
	PK_ASSERT(toEmit.m_DrawRequests.First() != null);

	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);

	// !Resolve material proxy and interface for first compatible material
	CRendererCache	*matCache = static_cast<CRendererCache*>(DrawPass().m_RendererCaches.First().Get());
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

#if (PK_HAS_GPU == 1)
	switch (m_DrawPass->m_RendererType)
	{
	case	PopcornFX::ERendererClass::Renderer_Mesh:
		return _IssueDrawCall_Mesh(renderContext, toEmit);
		break;
	default:
		PK_ASSERT_NOT_REACHED();
		return false;
	}
#else
	return false;
#endif // (PK_HAS_GPU == 1)
}

//----------------------------------------------------------------------------
