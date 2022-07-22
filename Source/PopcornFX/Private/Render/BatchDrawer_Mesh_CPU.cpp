//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "BatchDrawer_Mesh_CPU.h"
#include "RenderBatchManager.h"
#include "SceneManagement.h"
#include "MaterialDesc.h"
#include "StaticMeshResources.h"

#include "Engine/Engine.h"
#include "World/PopcornFXSceneProxy.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "Render/PopcornFXMeshVertexFactory.h"
#include "PopcornFXStats.h"

#include <pk_render_helpers/include/basic_renderer_properties/rh_basic_renderer_properties.h>
#include <pk_render_helpers/include/basic_renderer_properties/rh_vertex_animation_renderer_properties.h>

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_CPUBB::SAdditionalInput::UnmapBuffer()
{
	m_VertexBuffer.Unmap();
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_CPUBB::SAdditionalInput::ClearBuffer()
{
	m_VertexBuffer.UnmapAndClear();
	m_ByteSize = 0;
	m_BufferOffset = 0;
	m_AdditionalInputIndex = 0;
}

//----------------------------------------------------------------------------

CBatchDrawer_Mesh_CPUBB::CBatchDrawer_Mesh_CPUBB()
:	m_TotalParticleCount(0)
,	m_TotalVertexCount(0)
,	m_TotalIndexCount(0)
{
}

//----------------------------------------------------------------------------

CBatchDrawer_Mesh_CPUBB::~CBatchDrawer_Mesh_CPUBB()
{
	_ClearBuffers();

	PK_ASSERT(!m_SimData.Valid());
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_CPUBB::Setup(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleRenderMedium *owner, const PopcornFX::CFrameCollector *fc, const PopcornFX::CStringId &storageClass)
{
	if (!Super::Setup(renderer, owner, fc, storageClass))
		return false;
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_CPUBB::AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const
{
	if (!Super::AreRenderersCompatible(rendererA, rendererB))
		return false;
	const CRendererCache	*firstMatCache = static_cast<const CRendererCache*>(rendererA->m_RendererCache.Get());
	const CRendererCache	*secondMatCache = static_cast<const CRendererCache*>(rendererB->m_RendererCache.Get());

	if (firstMatCache == null || secondMatCache == null)
		return false;

	const CMaterialDesc_RenderThread	&rDesc0 = firstMatCache->RenderThread_Desc();
	const CMaterialDesc_RenderThread	&rDesc1 = secondMatCache->RenderThread_Desc();
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

	// If renderer materials are not the same, then they cannot be compatible
	// Otherwise, they should've been batched at effect import time
	switch (rDesc0.m_RendererClass)
	{
	case	PopcornFX::ERendererClass::Renderer_Billboard:
	case	PopcornFX::ERendererClass::Renderer_Ribbon:
	case	PopcornFX::ERendererClass::Renderer_Triangle:
	{
		const FPopcornFXSubRendererMaterial	*mat0 = rDesc0.m_RendererMaterial->GetSubMaterial(0);
		const FPopcornFXSubRendererMaterial	*mat1 = rDesc1.m_RendererMaterial->GetSubMaterial(0);
		if (mat0 == null || mat1 == null)
			return false;
		if (mat0 == mat1 ||
			mat0->RenderThread_SameMaterial_Billboard(*mat1))
			return true;
		break;
	}
	case	PopcornFX::ERendererClass::Renderer_Mesh:
	{
		const FPopcornFXSubRendererMaterial	*mat0 = rDesc0.m_RendererMaterial->GetSubMaterial(0);
		const FPopcornFXSubRendererMaterial	*mat1 = rDesc1.m_RendererMaterial->GetSubMaterial(0);
		if (mat0 == null || mat1 == null)
			return false;
		if (mat0 == mat1 ||
			mat0->RenderThread_SameMaterial_Mesh(*mat1))
			return true;
		break;
	}
	default:
		PK_ASSERT_NOT_REACHED();
		return false;
	}
	return false;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_CPUBB::CanRender(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) const
{
	PK_ASSERT(drawPass.m_RendererCaches.First() != null);

	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);
	CMaterialDesc_RenderThread	&matDesc = static_cast<CRendererCache*>(drawPass.m_RendererCaches.First().Get())->RenderThread_Desc();
	PK_ASSERT(renderContext.m_RendererSubView != null);

	return	renderContext.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Main ||
#if RHI_RAYTRACING
			(renderContext.m_RendererSubView->Pass() == CRendererSubView::RenderPass_RT_AccelStructs && matDesc.m_Raytraced) ||
#endif
			(renderContext.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Shadow && matDesc.m_CastShadows);
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_CPUBB::BeginFrame(PopcornFX::SRenderContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_CPUBB::BeginFrame");

	Super::BeginFrame(ctx);

	INC_DWORD_STAT_BY(STAT_PopcornFX_BatchesCount, 1);

	_ClearBuffers();
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_CPUBB::_IsAdditionalInputSupported(const PopcornFX::CStringId &fieldName, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets &outStreamOffsetType)
{
	if (type == PopcornFX::EBaseTypeID::BaseType_Float4)
	{
		if (fieldName == PopcornFX::BasicRendererProperties::SID_Diffuse_Color() ||
			fieldName == PopcornFX::BasicRendererProperties::SID_Distortion_Color())
			outStreamOffsetType = StreamOffset_Colors;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput0_Input0())
			outStreamOffsetType = StreamOffset_DynParam0s;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput1_Input1())
			outStreamOffsetType = StreamOffset_DynParam1s;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput2_Input2())
			outStreamOffsetType = StreamOffset_DynParam2s;
		//else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput3_Input3())
		//	outStreamOffsetType = StreamOffset_DynParam3s;
	}
	else if (type == PopcornFX::EBaseTypeID::BaseType_Float3)
	{
		if (fieldName == PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveColor())
			outStreamOffsetType = StreamOffset_EmissiveColors;
	}
	else if (type == PopcornFX::EBaseTypeID::BaseType_Float)
	{
		if (fieldName == PopcornFX::BasicRendererProperties::SID_AlphaRemap_Cursor())
			outStreamOffsetType = StreamOffset_AlphaCursors;
		else if (fieldName == PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_Cursor() ||
				 fieldName == PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft_Cursor() ||
				 fieldName == PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid_Cursor())
			outStreamOffsetType = StreamOffset_VATCursors;
	}
	return outStreamOffsetType != EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_CPUBB::AllocBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_CPUBB::AllocBuffers");
	const PopcornFX::SRendererBatchDrawPass_Mesh_CPUBB		&drawPassCPU = static_cast<const PopcornFX::SRendererBatchDrawPass_Mesh_CPUBB&>(drawPass);
	const SUERenderContext									&renderContext = static_cast<SUERenderContext&>(ctx);

	const bool	resizeBuffers = m_TotalParticleCount != drawPassCPU.m_TotalParticleCount;
	m_TotalParticleCount = drawPassCPU.m_TotalParticleCount;
	m_PerMeshParticleCount = drawPassCPU.m_PerMeshParticleCount;
	m_HasMeshIDs = drawPassCPU.m_HasMeshIDs;

	PK_ASSERT(renderContext.m_RendererSubView != null);
	m_FeatureLevel = renderContext.m_RendererSubView->ViewFamily()->GetFeatureLevel();

	m_RealViewCount = drawPass.m_Views.Count();

	CRenderBatchManager	*rbManager = renderContext.m_RenderBatchManager;
	PK_ASSERT(rbManager != null);

	CVertexBufferPool	*mainVBPool = &rbManager->VBPool();
	CIndexBufferPool	*mainIBPool = &rbManager->IBPool();
	CVertexBufferPool	*particleDataVBPool = &rbManager->VBPool_VertexBB();
	PK_ASSERT(mainVBPool != null);
	PK_ASSERT(mainIBPool != null);
	PK_ASSERT(particleDataVBPool != null);

	const u32	totalParticleCount = m_TotalParticleCount;
	const u32	totalVertexCount = m_TotalVertexCount;
	const u32	totalIndexCount = m_TotalIndexCount;

	// View independent
	const PopcornFX::SGeneratedInputs	&toGenerate = drawPass.m_ToGenerate;


	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_CPUBB::AllocBuffers_ViewIndependent");

#if RHI_RAYTRACING
			// Tests: raytracing ? billboard into main memory buffer, then copy to gpu buffer
			const bool	isRayTracingPass = renderContext.m_RendererSubView->RenderPass() == CRendererSubView::RenderPass_RT_AccelStructs;
			if (isRayTracingPass)
			{
				// Allocate our data here
				if (!PK_VERIFY(m_RayTracing_Matrices.GetSome<CFloat4x4>(m_TotalParticleCount) != null))
					return false;
			}
#endif // RHI_RAYTRACING
			if (!mainVBPool->AllocateIf(true, m_Instanced_Matrices, m_TotalParticleCount, sizeof(CFloat4x4), false))
				return false;
	}

	if (drawPass.m_IsNewFrame ||
		resizeBuffers) // m_TotalParticleCount can differ between passes, realloc buffers if that's the case
	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_CPUBB::AllocBuffers_AdditionalInputs");

		_ClearStreamOffsets();

		// Additional inputs sent to shaders (AlphaRemapCursor, Colors, ..)
		const u32	aFieldCount = toGenerate.m_AdditionalGeneratedInputs.Count();

		m_AdditionalInputs.Clear();
		if (!PK_VERIFY(m_AdditionalInputs.Reserve(aFieldCount))) // Max possible additional field count
			return false;

		m_SimDataBufferSizeInBytes = 0;
		for (u32 iField = 0; iField < aFieldCount; ++iField)
		{
			const PopcornFX::SRendererFeatureFieldDefinition	&additionalInput = toGenerate.m_AdditionalGeneratedInputs[iField];

			const PopcornFX::CStringId			&fieldName = additionalInput.m_Name;
			u32									typeSize = PopcornFX::CBaseTypeTraits::Traits(additionalInput.m_Type).Size;
			const u32							fieldID = m_AdditionalInputs.Count();
			EPopcornFXAdditionalStreamOffsets	streamOffsetType = EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount;

			if (!_IsAdditionalInputSupported(fieldName, additionalInput.m_Type, streamOffsetType))
				continue; // Unsupported shader input, discard
			PK_ASSERT(streamOffsetType < EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount);
			m_AdditionalStreamOffsets[streamOffsetType].Setup(m_SimDataBufferSizeInBytes, iField);

			if (!PK_VERIFY(m_AdditionalInputs.PushBack().Valid()))
				return false;
			SAdditionalInput	&newAdditionalInput = m_AdditionalInputs.Last();

			if (!mainVBPool->Allocate(newAdditionalInput.m_VertexBuffer, totalParticleCount, typeSize))
				return false;

			newAdditionalInput.m_BufferOffset = m_SimDataBufferSizeInBytes;
			newAdditionalInput.m_ByteSize = typeSize;
			newAdditionalInput.m_AdditionalInputIndex = iField;
			m_SimDataBufferSizeInBytes += typeSize * totalParticleCount;
		}

		if (m_SimDataBufferSizeInBytes > 0) // m_SimDataBufferSizeInBytes being 0 means no additional inputs ?
		{
			const u32	elementCount = m_SimDataBufferSizeInBytes / sizeof(float);
			if (!particleDataVBPool->Allocate(m_SimData, elementCount, sizeof(float), false))
				return false;
		}
	}
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_CPUBB::UnmapBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_CPUBB::UnmapBuffers");

	m_SimData.Unmap();

#if RHI_RAYTRACING
	// Temp tests for raytracing meshes
	if (!m_Mapped_RayTracing_Matrices.Empty())
	{
		PK_ASSERT(m_Mapped_RayTracing_Matrices.Count() == m_Mapped_Matrices.Count());
		PK_ASSERT(m_Mapped_RayTracing_Matrices.Stride() == m_Mapped_Matrices.Stride());
		PK_ASSERT(m_Mapped_RayTracing_Matrices.Contiguous());
		PK_ASSERT(m_Mapped_Matrices.Contiguous());
		PopcornFX::Mem::Copy_Uncached(m_Mapped_RayTracing_Matrices.Data(), m_Mapped_Matrices.Data(), m_Mapped_Matrices.Count() * sizeof(CFloat4x4));
	}
#endif // RHI_RAYTRACING

	// Meshes
	m_Instanced_Matrices.Unmap();

	const u32	aFieldCount = m_AdditionalInputs.Count();
	for (u32 iField = 0; iField < aFieldCount; ++iField)
		m_AdditionalInputs[iField].UnmapBuffer();

	return true;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_CPUBB::_ClearBuffers()
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_CPUBB::_ClearBuffers");

	m_FeatureLevel = ERHIFeatureLevel::Num;

	m_SimData.UnmapAndClear();

	// Meshes
	m_Instanced_Matrices.UnmapAndClear();

#if RHI_RAYTRACING
	m_Mapped_RayTracing_Matrices.Clear();
#endif // RHI_RAYTRACING

	m_Mapped_Matrices.Clear();
	m_Mapped_Param_Colors.Clear();
	m_Mapped_Param_EmissiveColors.Clear();
	m_Mapped_Param_DynamicParameter0.Clear();
	m_Mapped_Param_DynamicParameter1.Clear();
	m_Mapped_Param_DynamicParameter2.Clear();
	//m_Mapped_Param_DynamicParameter3.Clear();

	const u32	aFieldCount = m_AdditionalInputs.Count();
	for (u32 iField = 0; iField < aFieldCount; ++iField)
		m_AdditionalInputs[iField].ClearBuffer();
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_CPUBB::_ClearStreamOffsets()
{
	for (u32 iStream = 0; iStream < __SupportedAdditionalStreamCount; ++iStream)
		m_AdditionalStreamOffsets[iStream].Reset();
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_CPUBB::MapBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_CPUBB::MapBuffers_Meshes");

	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);

	// Note: all of this verbose code related to additional inputs will be removed in future PK versions when all particle data is mapped in a single GPU buffer.
	// This will allow support for all 4 shader parameters + VATs without vertex attribute limitations and will remove the need for explicit naming.
	const bool		hasColors = m_AdditionalStreamOffsets[StreamOffset_Colors].Valid();
	const bool		hasVATCursors = m_AdditionalStreamOffsets[StreamOffset_VATCursors].Valid();
	const bool		hasDynParam0 = m_AdditionalStreamOffsets[StreamOffset_DynParam0s].Valid();
	const bool		hasDynParam1 = m_AdditionalStreamOffsets[StreamOffset_DynParam1s].Valid();
	const bool		hasDynParam2 = m_AdditionalStreamOffsets[StreamOffset_DynParam2s].Valid();
	const u32		mappedAdditionalInputCount = (u32)hasColors + (u32)hasVATCursors + (u32)hasDynParam0 + (u32)hasDynParam1 + (u32)hasDynParam2;

	PK_ASSERT(m_Instanced_Matrices.Valid());

	m_Mapped_Matrices = TStridedMemoryView<CFloat4x4>(0, m_TotalParticleCount);
	if (!m_Instanced_Matrices->Map(m_Mapped_Matrices))
		return false;
#if RHI_RAYTRACING
	// We need to write into CPU memory for now
	// See with UE4.25 how we could use the gpu buffer directly
	const bool	isRayTracingPass = renderContext.m_RendererSubView->RenderPass() == CRendererSubView::RenderPass_RT_AccelStructs;
	if (isRayTracingPass)
	{
		CFloat4x4	*matricesWb = m_RayTracing_Matrices.GetSome<CFloat4x4>(m_TotalParticleCount);
		if (!PK_VERIFY(matricesWb != null))
			return false;
		m_Mapped_RayTracing_Matrices = m_Mapped_Matrices; // confusing, tmp
		m_Mapped_Matrices = TStridedMemoryView<CFloat4x4>(reinterpret_cast<CFloat4x4*>(matricesWb), m_TotalParticleCount, sizeof(CFloat4x4));
	}
#endif // RHI_RAYTRACING

	// Additional input mapping (named because re-used when issuing draw calls)
	if (hasColors)
	{
		const u32	inputId = m_AdditionalStreamOffsets[StreamOffset_Colors].InputId();
		PK_ASSERT(inputId < m_AdditionalInputs.Count());
		PK_ASSERT(m_AdditionalInputs[inputId].m_VertexBuffer.Valid());

		m_Mapped_Param_Colors = TStridedMemoryView<CFloat4>(0, m_TotalParticleCount);
		if (!m_AdditionalInputs[inputId].m_VertexBuffer->Map(m_Mapped_Param_Colors))
			return false;
	}
	if (hasVATCursors)
	{
		const u32	inputId = m_AdditionalStreamOffsets[StreamOffset_VATCursors].InputId();
		PK_ASSERT(inputId < m_AdditionalInputs.Count());
		PK_ASSERT(m_AdditionalInputs[inputId].m_VertexBuffer.Valid());

		m_Mapped_Param_VATCursors = TStridedMemoryView<float>(0, m_TotalParticleCount);
		if (!m_AdditionalInputs[inputId].m_VertexBuffer->Map(m_Mapped_Param_VATCursors))
			return false;
	}
	if (hasDynParam0)
	{
		const u32	inputId = m_AdditionalStreamOffsets[StreamOffset_DynParam0s].InputId();
		PK_ASSERT(inputId < m_AdditionalInputs.Count());
		PK_ASSERT(m_AdditionalInputs[inputId].m_VertexBuffer.Valid());

		m_Mapped_Param_DynamicParameter0 = TStridedMemoryView<CFloat4>(0, m_TotalParticleCount);
		if (!m_AdditionalInputs[inputId].m_VertexBuffer->Map(m_Mapped_Param_DynamicParameter0))
			return false;
	}
	if (hasDynParam1)
	{
		const u32	inputId = m_AdditionalStreamOffsets[StreamOffset_DynParam1s].InputId();
		PK_ASSERT(inputId < m_AdditionalInputs.Count());
		PK_ASSERT(m_AdditionalInputs[inputId].m_VertexBuffer.Valid());

		m_Mapped_Param_DynamicParameter1 = TStridedMemoryView<CFloat4>(0, m_TotalParticleCount);
		if (!m_AdditionalInputs[inputId].m_VertexBuffer->Map(m_Mapped_Param_DynamicParameter1))
			return false;
	}
	if (hasDynParam2)
	{
		const u32	inputId = m_AdditionalStreamOffsets[StreamOffset_DynParam2s].InputId();
		PK_ASSERT(inputId < m_AdditionalInputs.Count());
		PK_ASSERT(m_AdditionalInputs[inputId].m_VertexBuffer.Valid());

		m_Mapped_Param_DynamicParameter2 = TStridedMemoryView<CFloat4>(0, m_TotalParticleCount);
		if (!m_AdditionalInputs[inputId].m_VertexBuffer->Map(m_Mapped_Param_DynamicParameter2))
			return false;
	}

	if (mappedAdditionalInputCount > 0)
	{
		u32	mappedBufferIndex = 0;
		if (!PK_VERIFY(m_MappedAdditionalInputs.Resize(mappedAdditionalInputCount)))
			return false;
		if (hasColors)
		{
			const u32							inputId = m_AdditionalStreamOffsets[StreamOffset_Colors].InputId();
			PopcornFX::Drawers::SCopyFieldDesc	&desc = m_MappedAdditionalInputs[mappedBufferIndex++];
			desc.m_AdditionalInputIndex = m_AdditionalInputs[inputId].m_AdditionalInputIndex;
			desc.m_Storage.m_Count = m_TotalParticleCount;
			desc.m_Storage.m_RawDataPtr = reinterpret_cast<u8*>(m_Mapped_Param_Colors.Data());
			desc.m_Storage.m_Stride = sizeof(CFloat4);
		}
		if (hasVATCursors)
		{
			const u32							inputId = m_AdditionalStreamOffsets[StreamOffset_VATCursors].InputId();
			PopcornFX::Drawers::SCopyFieldDesc	&desc = m_MappedAdditionalInputs[mappedBufferIndex++];
			desc.m_AdditionalInputIndex = m_AdditionalInputs[inputId].m_AdditionalInputIndex;
			desc.m_Storage.m_Count = m_TotalParticleCount;
			desc.m_Storage.m_RawDataPtr = reinterpret_cast<u8*>(m_Mapped_Param_VATCursors.Data());
			desc.m_Storage.m_Stride = sizeof(float);
		}
		if (hasDynParam0)
		{
			const u32							inputId = m_AdditionalStreamOffsets[StreamOffset_DynParam0s].InputId();
			PopcornFX::Drawers::SCopyFieldDesc	&desc = m_MappedAdditionalInputs[mappedBufferIndex++];
			desc.m_AdditionalInputIndex = m_AdditionalInputs[inputId].m_AdditionalInputIndex;
			desc.m_Storage.m_Count = m_TotalParticleCount;
			desc.m_Storage.m_RawDataPtr = reinterpret_cast<u8*>(m_Mapped_Param_DynamicParameter0.Data());
			desc.m_Storage.m_Stride = sizeof(CFloat4);
		}
		if (hasDynParam1)
		{
			const u32							inputId = m_AdditionalStreamOffsets[StreamOffset_DynParam1s].InputId();
			PopcornFX::Drawers::SCopyFieldDesc	&desc = m_MappedAdditionalInputs[mappedBufferIndex++];
			desc.m_AdditionalInputIndex = m_AdditionalInputs[inputId].m_AdditionalInputIndex;
			desc.m_Storage.m_Count = m_TotalParticleCount;
			desc.m_Storage.m_RawDataPtr = reinterpret_cast<u8*>(m_Mapped_Param_DynamicParameter1.Data());
			desc.m_Storage.m_Stride = sizeof(CFloat4);
		}
		if (hasDynParam2)
		{
			const u32							inputId = m_AdditionalStreamOffsets[StreamOffset_DynParam2s].InputId();
			PopcornFX::Drawers::SCopyFieldDesc	&desc = m_MappedAdditionalInputs[mappedBufferIndex++];
			desc.m_AdditionalInputIndex = m_AdditionalInputs[inputId].m_AdditionalInputIndex;
			desc.m_Storage.m_Count = m_TotalParticleCount;
			desc.m_Storage.m_RawDataPtr = reinterpret_cast<u8*>(m_Mapped_Param_DynamicParameter2.Data());
			desc.m_Storage.m_Stride = sizeof(CFloat4);
		}
		m_BBJobs_Mesh.m_Exec_CopyField.m_FieldsToCopy = m_MappedAdditionalInputs.View();
	}

	m_BBJobs_Mesh.m_Exec_Matrices.m_PositionScale = 100.0f;
	m_BBJobs_Mesh.m_Exec_Matrices.m_Matrices = TStridedMemoryView<CFloat4x4>(&m_Mapped_Matrices[0], m_Mapped_Matrices.Count(), sizeof(m_Mapped_Matrices[0]));
	return true;
}

//----------------------------------------------------------------------------

class FPopcornFXMeshCollector : public FOneFrameResource
{
public:
	FPopcornFXMeshVertexFactory		*m_VertexFactory = null;

	FPopcornFXMeshCollector() { }
	~FPopcornFXMeshCollector()
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

void	CBatchDrawer_Mesh_CPUBB::_PreSetupMeshData(FPopcornFXMeshVertexFactory::FDataType &meshData, const FStaticMeshLODResources &meshResources)
{
	meshData.bInitialized = false;

	// see Engine/Private/Particles/ParticleSystemRender.cpp -> FDynamicMeshEmitterData::SetupVertexFactory

	// Warning for next UE versions, nothing seems to be done with the in vertex factory, this might change
	meshResources.VertexBuffers.PositionVertexBuffer.BindPositionVertexBuffer(null, meshData);
	meshResources.VertexBuffers.StaticMeshVertexBuffer.BindTangentVertexBuffer(null, meshData);
	meshResources.VertexBuffers.StaticMeshVertexBuffer.BindTexCoordVertexBuffer(null, meshData, MAX_TEXCOORDS);
	meshResources.VertexBuffers.ColorVertexBuffer.BindColorVertexBuffer(null, meshData);

	// Not yet initialized, particle instance data will be added later
	// meshData.bInitialized = true;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_CPUBB::_CreateMeshVertexFactory(u32 buffersOffset, FMeshElementCollector *collector, FPopcornFXMeshVertexFactory *&outFactory, FPopcornFXMeshCollector *&outCollectorRes)
{
	// Note: all of this verbose code related to additional inputs will be removed in future PK versions when all particle data is mapped in a single GPU buffer.
	// This will allow support for all 4 shader parameters + VATs without vertex attribute limitations and will remove the need for explicit naming.

	// Should we even allow render without a color stream ? Default value ?
	const bool	hasColors = m_AdditionalStreamOffsets[StreamOffset_Colors].Valid();
	const bool	hasVATCursors = m_AdditionalStreamOffsets[StreamOffset_VATCursors].Valid();
	const bool	hasDynParam0 = m_AdditionalStreamOffsets[StreamOffset_DynParam0s].Valid();
	const bool	hasDynParam1 = m_AdditionalStreamOffsets[StreamOffset_DynParam1s].Valid();
	const bool	hasDynParam2 = m_AdditionalStreamOffsets[StreamOffset_DynParam2s].Valid();

	// Resets m_VFData, that is *copied* per VF
	m_VFData.m_InstancedMatrices.Setup(m_Instanced_Matrices, buffersOffset * sizeof(CFloat4x4));

	if (hasColors)
	{
		const u32	inputId = m_AdditionalStreamOffsets[StreamOffset_Colors].InputId();
		PK_ASSERT(inputId < m_AdditionalInputs.Count());
		PK_ASSERT(m_AdditionalInputs[inputId].m_VertexBuffer.Valid());
		m_VFData.m_InstancedColors.Setup(m_AdditionalInputs[inputId].m_VertexBuffer, buffersOffset * sizeof(CFloat4)); // Warning: Colors Stride with GPU particles might be different
	}
	if (hasVATCursors)
	{
		const u32	inputId = m_AdditionalStreamOffsets[StreamOffset_VATCursors].InputId();
		PK_ASSERT(inputId < m_AdditionalInputs.Count());
		PK_ASSERT(m_AdditionalInputs[inputId].m_VertexBuffer.Valid());
		m_VFData.m_InstancedVATCursors.Setup(m_AdditionalInputs[inputId].m_VertexBuffer, buffersOffset * sizeof(float));
	}
	if (hasDynParam0)
	{
		const u32	inputId = m_AdditionalStreamOffsets[StreamOffset_DynParam0s].InputId();
		PK_ASSERT(inputId < m_AdditionalInputs.Count());
		PK_ASSERT(m_AdditionalInputs[inputId].m_VertexBuffer.Valid());
		m_VFData.m_InstancedDynamicParameters0.Setup(m_AdditionalInputs[inputId].m_VertexBuffer, buffersOffset * sizeof(CFloat4));
	}
	if (hasDynParam1)
	{
		const u32	inputId = m_AdditionalStreamOffsets[StreamOffset_DynParam1s].InputId();
		PK_ASSERT(inputId < m_AdditionalInputs.Count());
		PK_ASSERT(m_AdditionalInputs[inputId].m_VertexBuffer.Valid());
		m_VFData.m_InstancedDynamicParameters1.Setup(m_AdditionalInputs[inputId].m_VertexBuffer, buffersOffset * sizeof(CFloat4));
	}
	if (hasDynParam2)
	{
		const u32	inputId = m_AdditionalStreamOffsets[StreamOffset_DynParam2s].InputId();
		PK_ASSERT(inputId < m_AdditionalInputs.Count());
		PK_ASSERT(m_AdditionalInputs[inputId].m_VertexBuffer.Valid());
		m_VFData.m_InstancedDynamicParameters2.Setup(m_AdditionalInputs[inputId].m_VertexBuffer, buffersOffset * sizeof(CFloat4));
	}
	m_VFData.bInitialized = true;

	// Create the vertex factory
	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_CPUBB::IssueDrawCall_Mesh CollectorRes");

		outCollectorRes = &(collector->AllocateOneFrameResource<FPopcornFXMeshCollector>());
		check(outCollectorRes != null);

		PK_ASSERT(outCollectorRes->m_VertexFactory == null);
		outCollectorRes->m_VertexFactory = new FPopcornFXMeshVertexFactory(m_FeatureLevel);

		outFactory = outCollectorRes->m_VertexFactory;
		outFactory->SetData(m_VFData);
		PK_ASSERT(!outFactory->IsInitialized());
		outFactory->InitResource();
	}
}

//----------------------------------------------------------------------------

#if RHI_RAYTRACING
void	CBatchDrawer_Mesh_CPUBB::_IssueDrawCall_Mesh_AccelStructs(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_CPUBB::IssueDrawCall_Mesh");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);

	const bool				gpuStorage = desc.m_DrawRequests.First()->GPUStorage();
	FMeshElementCollector	*collector = view->RTCollector();

	if (gpuStorage) // not supported yet
		return;
	if (desc.m_ViewIndex != 0) // submit once
		return;

	const FPopcornFXSceneProxy	*sceneProxy = view->SceneProxy();
	PK_ASSERT(collector != null);
	PK_ASSERT(sceneProxy != null);

	const u32	totalParticleCount = desc.m_TotalParticleCount;
	PK_ASSERT(totalParticleCount == m_TotalParticleCount);

	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return;
	CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();
	PK_ASSERT(matDesc.ValidForRendering());

	CFloat4x4	*rayTracing_InstancedMatrices = m_Mapped_Matrices.Data();
	if (!PK_VERIFY(rayTracing_InstancedMatrices != null))
		return;

	PK_ASSERT(desc.m_ViewIndex < m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() == m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() <= renderContext.m_RendererSubView->SceneViews().Count());

	// We can't (yet?) use our own vertex factory for the mesh raytracing passes
	// It requires manual vertex fetch, already done by the FLocalVertexFactory
	PK_ASSERT(matDesc.m_LODVertexFactories != null);
	const FLocalVertexFactory	*vertexFactory = &matDesc.m_LODVertexFactories->VertexFactory;
	if (!PK_VERIFY(vertexFactory != null))
		return;
	if (m_HasMeshIDs)
	{
		// Single geom per active section
		u32			buffersOffset = 0;
		const u32	sectionCount = matDesc.m_LODResources->Sections.Num();
		for (u32 iSection = 0; iSection < sectionCount; ++iSection)
		{
			const u32	sectionPCount = m_PerMeshParticleCount[iSection];
			if (sectionPCount == 0)
				continue;

			PK_ASSERT(iSection < (u32)matDesc.m_RayTracingGeometries.Num());
			FRayTracingGeometry	&rayTracingGeometry = matDesc.m_RayTracingGeometries[iSection];
			FRayTracingInstance	rayTracingInstance;
			rayTracingInstance.Geometry = &rayTracingGeometry;

			PK_ASSERT(view->RenderPass() != CRendererSubView::RenderPass_Shadow || matDesc.m_CastShadows);

			FMeshBatch	meshBatch;
			if (!PK_VERIFY(_BuildMeshBatch_Mesh(meshBatch, sceneProxy, view->SceneViews()[0].m_SceneView, matDesc, iSection, sectionPCount)))
				return;

			meshBatch.VertexFactory = vertexFactory;
			meshBatch.Elements[0].VertexFactoryUserData = vertexFactory->GetUniformBuffer();
			meshBatch.SegmentIndex = 0;

			INC_DWORD_STAT_BY(STAT_PopcornFX_RayTracing_DrawCallsCount, 1);
			rayTracingInstance.Materials.Add(meshBatch);

			// Temp code, until we know for sure if we can use the gpu buffer matrices directly (untouched)
			rayTracingInstance.InstanceTransforms.Append(reinterpret_cast<FMatrix*>(rayTracing_InstancedMatrices + buffersOffset), sectionPCount);
#if (ENGINE_MAJOR_VERSION == 5)
			rayTracingInstance.BuildInstanceMaskAndFlags(m_FeatureLevel);
#else
			rayTracingInstance.BuildInstanceMaskAndFlags();
#endif // (ENGINE_MAJOR_VERSION == 5)

			view->OutRayTracingInstances()->Add(rayTracingInstance);

			buffersOffset += sectionPCount;
		}
	}
	else
	{
		// Single geom, withall sections
		FRayTracingGeometry	&rayTracingGeometry = matDesc.m_LODResources->RayTracingGeometry;
		FRayTracingInstance	rayTracingInstance;
		rayTracingInstance.Geometry = &rayTracingGeometry;

		const u32	sectionCount = matDesc.m_LODResources->Sections.Num();
		for (u32 iSection = 0; iSection < sectionCount; ++iSection)
		{
			PK_ASSERT(view->RenderPass() != CRendererSubView::RenderPass_Shadow || matDesc.m_CastShadows);

			FMeshBatch	meshBatch;

			if (!PK_VERIFY(_BuildMeshBatch_Mesh(meshBatch, sceneProxy, view->SceneViews()[0].m_SceneView, matDesc, iSection, m_TotalParticleCount)))
				return;

			meshBatch.VertexFactory = vertexFactory;
			meshBatch.Elements[0].VertexFactoryUserData = vertexFactory->GetUniformBuffer();

			{
				INC_DWORD_STAT_BY(STAT_PopcornFX_RayTracing_DrawCallsCount, 1);
				rayTracingInstance.Materials.Add(meshBatch);
			}
		} // for (sections)

		{
			// Temp code, until we know for sure if we can use the gpu buffer matrices directly (untouched)
			rayTracingInstance.InstanceTransforms.Append(reinterpret_cast<FMatrix*>(rayTracing_InstancedMatrices), m_TotalParticleCount);
#if (ENGINE_MAJOR_VERSION == 5)
			rayTracingInstance.BuildInstanceMaskAndFlags(m_FeatureLevel);
#else
			rayTracingInstance.BuildInstanceMaskAndFlags();
#endif // (ENGINE_MAJOR_VERSION == 5)

			view->OutRayTracingInstances()->Add(rayTracingInstance);
		}
	}
}
#endif // RHI_RAYTRACING

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_CPUBB::_BuildMeshBatch_Mesh(	FMeshBatch &meshBatch,
														const FPopcornFXSceneProxy *sceneProxy,
														const FSceneView *view,
														const CMaterialDesc_RenderThread &matDesc,
														u32 iSection,
														u32 sectionParticleCount)
{
	PK_ASSERT(sceneProxy != null);

	meshBatch.LCI = NULL;
	meshBatch.bUseAsOccluder = false;
	meshBatch.ReverseCulling = sceneProxy->IsLocalToWorldDeterminantNegative();
	meshBatch.CastShadow = matDesc.m_CastShadows;

#if RHI_RAYTRACING
	meshBatch.CastRayTracedShadow = matDesc.m_CastShadows;
	meshBatch.SegmentIndex = iSection;
#endif // RHI_RAYTRACING

	meshBatch.DepthPriorityGroup = sceneProxy->GetDepthPriorityGroup(view);
	meshBatch.Type = PT_TriangleList;
	meshBatch.MaterialRenderProxy = matDesc.RenderProxy(); // TODO: Check with v1 impl.
	meshBatch.bCanApplyViewModeOverrides = true;

	const FStaticMeshSection	&section = matDesc.m_LODResources->Sections[iSection];
	if (!PK_VERIFY(section.NumTriangles > 0))
		return false;
	FMeshBatchElement	&batchElement = meshBatch.Elements[0];

	batchElement.FirstIndex = section.FirstIndex;
	batchElement.MinVertexIndex = section.MinVertexIndex;
	batchElement.MaxVertexIndex = section.MaxVertexIndex;
	batchElement.NumInstances = sectionParticleCount; // Ignored by raytracing pass

	batchElement.IndexBuffer = &matDesc.m_LODResources->IndexBuffer;
	batchElement.NumPrimitives = section.NumTriangles;

	batchElement.UserIndex = 0;

	return true;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Mesh_CPUBB::_IssueDrawCall_Mesh(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_CPUBB::IssueDrawCall_Mesh");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);

	FMeshElementCollector		*collector = view->Collector();
	const FPopcornFXSceneProxy	*sceneProxy = view->SceneProxy();
	PK_ASSERT(collector != null);
	PK_ASSERT(sceneProxy != null);

	const u32	totalParticleCount = desc.m_TotalParticleCount;
	PK_ASSERT(totalParticleCount == m_TotalParticleCount);

	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return;
	CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();
	PK_ASSERT(matDesc.ValidForRendering());

	// Could we do this elsewhere ? .. Not per draw call ?
	_PreSetupMeshData(m_VFData, *matDesc.m_LODResources);

	// Vertex factory and collector resource are either used to render all sections (if all sections of the mesh are being rendered of m_TotalParticleCount)
	// If there is a mesh atlas, we have to create a vertex factory per mesh section, because we cannot specify per FMeshBatchElement an offset into the instances buffers
	FPopcornFXMeshVertexFactory		*vertexFactory = null;
	FPopcornFXMeshCollector			*collectorRes = null;
	if (!m_HasMeshIDs)
		_CreateMeshVertexFactory(0, collector, vertexFactory, collectorRes);

	PK_ASSERT(desc.m_ViewIndex < m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() == m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() <= renderContext.m_RendererSubView->SceneViews().Count());
	for (u32 iView = 0; iView < m_RealViewCount; ++iView)
	{
		if (desc.m_ViewIndex != iView)
			continue;

		u32			buffersOffset = 0;
		const u32	realViewIndex = renderContext.m_RendererSubView->BBViews()[iView].m_ViewIndex;
		const u32	sectionCount = matDesc.m_LODResources->Sections.Num();
		for (u32 iSection = 0; iSection < sectionCount; ++iSection)
		{
			const u32	sectionPCount = m_HasMeshIDs ? m_PerMeshParticleCount[iSection] : m_TotalParticleCount;
			if (sectionPCount == 0)
				continue;

			if (m_HasMeshIDs)
				_CreateMeshVertexFactory(buffersOffset, collector, vertexFactory, collectorRes);

			PK_ASSERT(view->RenderPass() != CRendererSubView::RenderPass_Shadow || matDesc.m_CastShadows);

			FMeshBatch	&meshBatch = collector->AllocateMesh();

			if (!PK_VERIFY(_BuildMeshBatch_Mesh(meshBatch, sceneProxy, view->SceneViews()[realViewIndex].m_SceneView, matDesc, iSection, sectionPCount)))
				return;

			meshBatch.VertexFactory = vertexFactory;
			meshBatch.Elements[0].PrimitiveUniformBuffer = sceneProxy->GetUniformBuffer();

			buffersOffset += m_HasMeshIDs ? sectionPCount : 0;

			{
				INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsCount, 1);
				collector->AddMesh(realViewIndex, meshBatch);
			}
		} // for (sections)
	} // for (views)
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Mesh_CPUBB::EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass, const PopcornFX::SDrawCallDesc &toEmit)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Mesh_CPUBB::EmitDrawCall");
	PK_ASSERT(toEmit.m_TotalParticleCount <= m_TotalParticleCount); // <= if slicing is enabled
	PK_ASSERT(toEmit.m_TotalIndexCount <= m_TotalIndexCount);
	PK_ASSERT(toEmit.m_TotalVertexCount <= m_TotalIndexCount);
	PK_ASSERT(toEmit.m_DrawRequests.First() != null);

	// !Resolve material proxy and interface for first compatible material
	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);
	CRendererCache				*matCache = static_cast<CRendererCache*>(toEmit.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return true;

	CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();

	if (!matDesc.ResolveMaterial(PopcornFX::Drawers::BillboardingLocation_CPU)) // CPU. Same checks as ComputeShader, and ComputeShader will be deprecated soon
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

	_IssueDrawCall_Mesh(renderContext, toEmit);
	return true;
}

//----------------------------------------------------------------------------
