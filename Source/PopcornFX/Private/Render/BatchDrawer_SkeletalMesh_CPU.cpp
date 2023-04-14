//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "BatchDrawer_SkeletalMesh_CPU.h"
#include "RenderBatchManager.h"
#include "SceneManagement.h"
#include "MaterialDesc.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Engine/SkeletalMesh.h"

#include "Engine/Engine.h"
#include "World/PopcornFXSceneProxy.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "Render/PopcornFXSkeletalMeshVertexFactory.h"
#include "Render/PopcornFXVertexFactoryCommon.h"
#include "PopcornFXStats.h"
#include "PopcornFXPlugin.h"

#include <pk_render_helpers/include/render_features/rh_features_basic.h>
#include <pk_render_helpers/include/render_features/rh_features_vat_skeletal.h>

//----------------------------------------------------------------------------

#if (PK_PREBUILD_BONE_TRANSFORMS != 0)
DECLARE_GPU_STAT_NAMED(PopcornFXComputeBoneTransformsCS, TEXT("PopcornFX Bone transforms update"));
#endif // (PK_PREBUILD_BONE_TRANSFORMS != 0)

//----------------------------------------------------------------------------

void	CBatchDrawer_SkeletalMesh_CPUBB::SAdditionalInput::Clear()
{
	m_ByteSize = 0;
	m_BufferOffset = 0;
	m_AdditionalInputIndex = 0;
}

//----------------------------------------------------------------------------

CBatchDrawer_SkeletalMesh_CPUBB::CBatchDrawer_SkeletalMesh_CPUBB()
{
}

//----------------------------------------------------------------------------

CBatchDrawer_SkeletalMesh_CPUBB::~CBatchDrawer_SkeletalMesh_CPUBB()
{
	_ClearBuffers();

	PK_ASSERT(!m_SimData.Valid());

#if (PK_PREBUILD_BONE_TRANSFORMS != 0)
	// Render resources cannot be released on the game thread.
	// When re-importing an effect, render medium gets destroyed on the main thread (expected scenario)
	// When unloading a scene, render mediums are destroyed on the render thread (RenderBatchManager.cpp::Clean())
	if (IsInGameThread())
	{
		ENQUEUE_RENDER_COMMAND(ReleaseRenderResourcesCommand)(
			[this](FRHICommandListImmediate &RHICmdList)
			{
				m_BoneMatrices.Release();
				m_PreviousBoneMatrices.Release();
			});

		FlushRenderingCommands(); // so we can safely release frames
	}
	else
	{
		m_BoneMatrices.Release();
		m_PreviousBoneMatrices.Release();
	}
#endif // (PK_PREBUILD_BONE_TRANSFORMS != 0)
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_SkeletalMesh_CPUBB::Setup(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleRenderMedium *owner, const PopcornFX::CFrameCollector *fc, const PopcornFX::CStringId &storageClass)
{
	if (!Super::Setup(renderer, owner, fc, storageClass))
		return false;

	CRendererCache	*matCache = static_cast<CRendererCache*>(renderer->m_RendererCache.Get());
	if (!PK_VERIFY(matCache != null))
		return false;
	CMaterialDesc_GameThread	&matDesc = matCache->GameThread_Desc();

	m_MotionBlur = matDesc.m_MotionBlur;
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_SkeletalMesh_CPUBB::AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const
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

	PK_ASSERT(rDesc0.m_RendererClass == PopcornFX::Renderer_Mesh);
	const FPopcornFXSubRendererMaterial	*mat0 = rDesc0.m_RendererMaterial->GetSubMaterial(0);
	const FPopcornFXSubRendererMaterial	*mat1 = rDesc1.m_RendererMaterial->GetSubMaterial(0);
	if (mat0 == null || mat1 == null)
		return false;
	if (mat0 == mat1 ||
		mat0->RenderThread_SameMaterial_Mesh(*mat1))
		return true;
	return false;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_SkeletalMesh_CPUBB::CanRender(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass) const
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

void	CBatchDrawer_SkeletalMesh_CPUBB::BeginFrame(PopcornFX::SRenderContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::BeginFrame");

	Super::BeginFrame(ctx);

	INC_DWORD_STAT_BY(STAT_PopcornFX_BatchesCount, 1);

	_ClearBuffers();
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_SkeletalMesh_CPUBB::_IsAdditionalInputSupported(const PopcornFX::CStringId &fieldName, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets &outStreamOffsetType)
{
	if (type == PopcornFX::BaseType_Float4)
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
		if (fieldName == PopcornFX::BasicRendererProperties::SID_AlphaRemap_Cursor())
			outStreamOffsetType = StreamOffset_AlphaCursors;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_Atlas_TextureID())
			outStreamOffsetType = StreamOffset_TextureIDs;
		else if (fieldName == PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimation_AnimationCursor())
			outStreamOffsetType = StreamOffset_VATCursors;
		else if (fieldName == PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimationInterpolateTracks_NextAnimationCursor())
			outStreamOffsetType = StreamOffset_VATCursorNexts;
		else if (fieldName == PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimationInterpolateTracks_TransitionRatio())
			outStreamOffsetType = StreamOffset_VATTransitionCursors;
		else if (fieldName == PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimation_PrevAnimationCursor())
			outStreamOffsetType = StreamOffset_PrevVATCursors;
		else if (fieldName == PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimationInterpolateTracks_PrevNextAnimationCursor())
			outStreamOffsetType = StreamOffset_PrevVATCursorNexts;
		else if (fieldName == PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimationInterpolateTracks_PrevTransitionRatio())
			outStreamOffsetType = StreamOffset_PrevVATTransitionCursors;
	}
	else if (type == PopcornFX::BaseType_I32)
	{
		if (fieldName == PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimation_CurrentAnimTrack())
			outStreamOffsetType = StreamOffset_VATTracks;
		else if (fieldName == PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimationInterpolateTracks_NextAnimTrack())
			outStreamOffsetType = StreamOffset_VATTrackNexts;
		else if (fieldName == PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimation_PrevCurrentAnimTrack())
			outStreamOffsetType = StreamOffset_PrevVATTracks;
		else if (fieldName == PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimationInterpolateTracks_PrevNextAnimTrack())
			outStreamOffsetType = StreamOffset_PrevVATTrackNexts;
	}
	return outStreamOffsetType != EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_SkeletalMesh_CPUBB::AllocBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::AllocBuffers");
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
	CVertexBufferPool	*particleDataVBPool = &rbManager->VBPool_VertexBB();
	PK_ASSERT(mainVBPool != null);
	PK_ASSERT(particleDataVBPool != null);

	// View independent
	const PopcornFX::SGeneratedInputs	&toGenerate = drawPass.m_ToGenerate;

	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::AllocBuffers_ViewIndependent");

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
	}

	if (drawPass.m_IsNewFrame ||
		resizeBuffers) // m_TotalParticleCount can differ between passes, realloc buffers if that's the case
	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::AllocBuffers_AdditionalInputs");

		CRendererCache	*matCache = static_cast<CRendererCache*>(drawPass.m_RendererCaches.First().Get());
		if (!PK_VERIFY(matCache != null))
			return false;
		CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();

		if (m_MotionBlur)
		{
			m_SimDataBufferSizeInBytes = m_TotalParticleCount * 2 * sizeof(CFloat4x4); // Base size: matrix + prev matrix count
			m_PrevMatricesOffset = m_TotalParticleCount * 16;
		}
		else
		{
			m_SimDataBufferSizeInBytes = m_TotalParticleCount * sizeof(CFloat4x4); // Base size: matrix count
			m_PrevMatricesOffset = 0;
		}

		_ClearStreamOffsets();

#if (PK_PREBUILD_BONE_TRANSFORMS != 0)
		m_TotalBoneCount = matDesc.m_TotalBoneCount;
		m_BoneMatricesBufferStride = m_TotalBoneCount * 3;

		const u32	bufferElementCount = m_TotalParticleCount * m_BoneMatricesBufferStride; // float3x4
		if (m_BoneMatrices.Buffer == null ||
			bufferElementCount > m_BoneMatricesCapacity) // Only grow
		{
			m_BoneMatricesCapacity = bufferElementCount; // One per draw request
			m_BoneMatrices.Release();
			m_PreviousBoneMatrices.Release();

#if (ENGINE_MAJOR_VERSION == 5)
			m_BoneMatrices.Initialize(TEXT("PopcornFXBoneMatrices"), sizeof(PopcornFX::CFloat4), m_BoneMatricesCapacity, EPixelFormat::PF_A32B32G32R32F, BUF_UnorderedAccess | BUF_ShaderResource);
			if (m_MotionBlur)
				m_PreviousBoneMatrices.Initialize(TEXT("PopcornFXPreviousBoneMatrices"), sizeof(PopcornFX::CFloat4), m_BoneMatricesCapacity, EPixelFormat::PF_A32B32G32R32F, BUF_UnorderedAccess | BUF_ShaderResource);
#else
			m_BoneMatrices.Initialize(sizeof(PopcornFX::CFloat4), m_BoneMatricesCapacity, EPixelFormat::PF_A32B32G32R32F, BUF_UnorderedAccess | BUF_ShaderResource, TEXT("PopcornFXBoneMatrices"));
			if (m_MotionBlur)
				m_PreviousBoneMatrices.Initialize(sizeof(PopcornFX::CFloat4), m_BoneMatricesCapacity, EPixelFormat::PF_A32B32G32R32F, BUF_UnorderedAccess | BUF_ShaderResource, TEXT("PopcornFXPreviousBoneMatrices"));
#endif // (ENGINE_MAJOR_VERSION == 5)
		}
#endif // (PK_PREBUILD_BONE_TRANSFORMS != 0)

		{
			PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::Build bone reorder buffer");

			const u32	boneMapSum = matDesc.m_SkeletalMeshBoneIndicesReorder.Count();
			if (!PK_VERIFY(boneMapSum > 0))
				return false;
			if (!particleDataVBPool->AllocateIf(true, m_BoneIndicesReorder, boneMapSum, sizeof(u32), false))
				return false;

			PopcornFX::TMemoryView<u32>	boneIndices(0, boneMapSum);
			if (!m_BoneIndicesReorder->Map(boneIndices, boneMapSum))
				return false;
			PopcornFX::Mem::Copy(boneIndices.Data(), matDesc.m_SkeletalMeshBoneIndicesReorder.Data(), boneMapSum * sizeof(u32));
			m_BoneIndicesReorder.Unmap();
		}

		// Additional inputs sent to shaders (AlphaRemapCursor, Colors, ..)
		const u32	aFieldCount = toGenerate.m_AdditionalGeneratedInputs.Count();

		m_AdditionalInputs.Clear();
		if (!PK_VERIFY(m_AdditionalInputs.Reserve(aFieldCount))) // Max possible additional field count
			return false;

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

			newAdditionalInput.m_BufferOffset = m_SimDataBufferSizeInBytes;
			newAdditionalInput.m_ByteSize = typeSize;
			newAdditionalInput.m_AdditionalInputIndex = iField;
			m_SimDataBufferSizeInBytes += typeSize * m_TotalParticleCount;
		}

		if (m_SimDataBufferSizeInBytes > 0) // m_SimDataBufferSizeInBytes being 0 means no additional inputs ?
		{
			const u32	elementCount = m_SimDataBufferSizeInBytes / sizeof(float);
			if (!particleDataVBPool->Allocate(m_SimData, elementCount, sizeof(float), false))
				return false;
		}

		{
			PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::AllocBuffers (map atlas data)");

			// CPU particles, only map atlas data
			// TODO: move that in the renderer cache instead of doing that each frame
			PopcornFX::TMemoryView<const PopcornFX::Drawers::SMesh_DrawRequest * const>	drawRequests = drawPass.DrawRequests<PopcornFX::Drawers::SMesh_DrawRequest>();
			const PopcornFX::Drawers::SMesh_BillboardingRequest							&compatBr = drawRequests.First()->m_BB;

			if (compatBr.m_Atlas != null)
			{
				if (!PK_VERIFY(m_AtlasRects.LoadRects(compatBr.m_Atlas->m_RectsFp32)))
					return false;
				PK_ASSERT(m_AtlasRects.Loaded());
			}
		}
	}
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_SkeletalMesh_CPUBB::UnmapBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::UnmapBuffers");

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

	return true;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_SkeletalMesh_CPUBB::_ClearBuffers()
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::_ClearBuffers");

	m_FeatureLevel = ERHIFeatureLevel::Num;

	m_SimData.UnmapAndClear();

#if RHI_RAYTRACING
	m_Mapped_RayTracing_Matrices.Clear();
#endif // RHI_RAYTRACING

	m_Mapped_Matrices.Clear();

	const u32	aFieldCount = m_AdditionalInputs.Count();
	for (u32 iField = 0; iField < aFieldCount; ++iField)
		m_AdditionalInputs[iField].Clear();
}

//----------------------------------------------------------------------------

void	CBatchDrawer_SkeletalMesh_CPUBB::_ClearStreamOffsets()
{
	for (u32 iStream = 0; iStream < __SupportedAdditionalStreamCount; ++iStream)
		m_AdditionalStreamOffsets[iStream].Reset();
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_SkeletalMesh_CPUBB::MapBuffers(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::MapBuffers_Meshes");

	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);

	// Map global GPU buffer
	PK_ASSERT(m_SimData.Valid());
	PopcornFX::TMemoryView<float>	simData;

	if (!m_SimData->Map(simData, m_SimDataBufferSizeInBytes / sizeof(float)))
		return false;
	float	*_data = simData.Data();

	m_Mapped_Matrices = TStridedMemoryView<CFloat4x4>(reinterpret_cast<CFloat4x4*>(_data), m_TotalParticleCount);
#if 0
	if (m_MotionBlur)
		m_Mapped_PrevMatrices = TStridedMemoryView<CFloat4x4>(reinterpret_cast<CFloat4x4*>(PopcornFX::Mem::AdvanceRawPointer(_data, m_PrevMatricesOffset * sizeof(float))), m_TotalParticleCount);
#endif

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

	if (!drawPass.m_ToGenerate.m_AdditionalGeneratedInputs.Empty())
	{
		// Additional inputs
		const u32	aFieldCount = m_AdditionalInputs.Count();

		if (!PK_VERIFY(m_MappedAdditionalInputs.Resize(aFieldCount)))
			return false;
		for (u32 iField = 0; iField < aFieldCount; ++iField)
		{
			SAdditionalInput					&field = m_AdditionalInputs[iField];
			PopcornFX::Drawers::SCopyFieldDesc	&desc = m_MappedAdditionalInputs[iField];

			desc.m_Storage.m_Count = m_TotalParticleCount;
			desc.m_Storage.m_RawDataPtr = reinterpret_cast<u8*>(PopcornFX::Mem::AdvanceRawPointer(_data, field.m_BufferOffset));
			desc.m_Storage.m_Stride = field.m_ByteSize;
			desc.m_AdditionalInputIndex = field.m_AdditionalInputIndex;
		}
		m_BBJobs_Mesh.m_Exec_CopyField.m_FieldsToCopy = m_MappedAdditionalInputs.View();
	}

	m_BBJobs_Mesh.m_Exec_Matrices.m_PositionScale = 100.0f;
	m_BBJobs_Mesh.m_Exec_Matrices.m_Matrices = TStridedMemoryView<CFloat4x4>(&m_Mapped_Matrices[0], m_Mapped_Matrices.Count(), sizeof(m_Mapped_Matrices[0]));
#if 0
	if (m_MotionBlur)
		m_BBJobs_Mesh.m_Exec_Matrices.m_PrevMatrices = TStridedMemoryView<CFloat4x4>(&m_Mapped_PrevMatrices[0], m_Mapped_PrevMatrices.Count(), sizeof(m_Mapped_PrevMatrices[0]));
#endif
	return true;
}

//----------------------------------------------------------------------------

class FPopcornFXMeshCollector : public FOneFrameResource
{
public:
	FPopcornFXSkelMeshVertexFactory		*m_VertexFactory = null;

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

void	CBatchDrawer_SkeletalMesh_CPUBB::_PreSetupMeshData(FPopcornFXSkelMeshVertexFactory::FDataType &meshData, const FSkeletalMeshLODRenderData &meshResources)
{
	meshData.bInitialized = false;

	// see Engine/Private/Particles/ParticleSystemRender.cpp -> FDynamicMeshEmitterData::SetupVertexFactory

	// Warning for next UE versions, nothing seems to be done with the in vertex factory, this might change
	meshResources.StaticVertexBuffers.PositionVertexBuffer.BindPositionVertexBuffer(null, meshData);
	meshResources.StaticVertexBuffers.StaticMeshVertexBuffer.BindTangentVertexBuffer(null, meshData);
	meshResources.StaticVertexBuffers.StaticMeshVertexBuffer.BindTexCoordVertexBuffer(null, meshData, MAX_TEXCOORDS);
	meshResources.StaticVertexBuffers.ColorVertexBuffer.BindColorVertexBuffer(null, meshData); // TODO: Override vertex color buffer (vertex-painted in engine ?)
	
	const FSkinWeightVertexBuffer		*weightVertexBuffer = meshResources.GetSkinWeightVertexBuffer(); // TODO: Override skin weights buffer ?
	const FSkinWeightDataVertexBuffer	*weightDataVertexBuffer = weightVertexBuffer->GetDataVertexBuffer();
	const uint32						stride = weightVertexBuffer->GetConstantInfluencesVertexStride();
	const uint32						weightsOffset = weightVertexBuffer->GetConstantInfluencesBoneWeightsOffset();

	PK_ASSERT(weightVertexBuffer->GetBoneInfluenceType() != GPUSkinBoneInfluenceType::UnlimitedBoneInfluence);

	meshData.m_Use16BitBoneIndex = weightVertexBuffer->Use16BitBoneIndex();
	meshData.m_BoneIndices = FVertexStreamComponent(weightDataVertexBuffer, 0, stride, meshData.m_Use16BitBoneIndex ? VET_UShort4 : VET_UByte4);
	meshData.m_BoneWeights = FVertexStreamComponent(weightDataVertexBuffer, weightsOffset, stride, VET_UByte4N);

	// Not yet initialized, particle instance data will be added later
}

//----------------------------------------------------------------------------

void	CBatchDrawer_SkeletalMesh_CPUBB::_FillUniforms(	CMaterialDesc_RenderThread		&matDesc,
														u32								buffersOffset,
														FPopcornFXUniforms				&outUniforms,
														FPopcornFXSkelMeshUniforms		&outUniformsSkelMesh)
{
	outUniforms.InSimData = m_SimData.Buffer()->SRV();
	outUniforms.DynamicParameterMask = matDesc.m_DynamicParameterMask;

	outUniformsSkelMesh.AtlasRectCount = m_AtlasRects.m_AtlasRectsCount;
	if (m_AtlasRects.m_AtlasBufferSRV != null)
		outUniformsSkelMesh.AtlasBuffer = m_AtlasRects.m_AtlasBufferSRV;
	else
		outUniformsSkelMesh.AtlasBuffer = outUniforms.InSimData; // Dummy SRV

	outUniformsSkelMesh.InAlphaCursorsOffset =				m_AdditionalStreamOffsets[StreamOffset_AlphaCursors].Valid() ?				(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_AlphaCursors] / sizeof(float)) + buffersOffset) : -1;
	outUniformsSkelMesh.InTextureIDsOffset =				m_AdditionalStreamOffsets[StreamOffset_TextureIDs].Valid() ?				(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_TextureIDs] / sizeof(float)) + buffersOffset) : -1;
	outUniformsSkelMesh.InVATCursorsOffset =				m_AdditionalStreamOffsets[StreamOffset_VATCursors].Valid() ?				(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_VATCursors] / sizeof(float)) + buffersOffset) : -1;
	outUniformsSkelMesh.InVATCursorNextsOffset =			m_AdditionalStreamOffsets[StreamOffset_VATCursorNexts].Valid() ?			(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_VATCursorNexts] / sizeof(float)) + buffersOffset) : -1;
	outUniformsSkelMesh.InVATTracksOffset =					m_AdditionalStreamOffsets[StreamOffset_VATTracks].Valid() ?					(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_VATTracks] / sizeof(float)) + buffersOffset) : -1;
	outUniformsSkelMesh.InVATTrackNextsOffset =				m_AdditionalStreamOffsets[StreamOffset_VATTrackNexts].Valid() ?				(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_VATTrackNexts] / sizeof(float)) + buffersOffset) : -1;
	outUniformsSkelMesh.InVATTransitionCursorsOffset =		m_AdditionalStreamOffsets[StreamOffset_VATTransitionCursors].Valid() ?		(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_VATTransitionCursors] / sizeof(float)) + buffersOffset) : -1;
	outUniformsSkelMesh.InPrevVATCursorsOffset =			m_AdditionalStreamOffsets[StreamOffset_PrevVATCursors].Valid() ?			(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_PrevVATCursors] / sizeof(float)) + buffersOffset) : outUniformsSkelMesh.InVATCursorsOffset;
	outUniformsSkelMesh.InPrevVATCursorNextsOffset =		m_AdditionalStreamOffsets[StreamOffset_PrevVATCursorNexts].Valid() ?		(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_PrevVATCursorNexts] / sizeof(float)) + buffersOffset) : outUniformsSkelMesh.InVATCursorNextsOffset;
	outUniformsSkelMesh.InPrevVATTracksOffset =				m_AdditionalStreamOffsets[StreamOffset_PrevVATTracks].Valid() ?				(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_PrevVATTracks] / sizeof(float)) + buffersOffset) : outUniformsSkelMesh.InVATTracksOffset;
	outUniformsSkelMesh.InPrevVATTrackNextsOffset =			m_AdditionalStreamOffsets[StreamOffset_PrevVATTrackNexts].Valid() ?			(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_PrevVATTrackNexts] / sizeof(float)) + buffersOffset) : outUniformsSkelMesh.InVATTrackNextsOffset;
	outUniformsSkelMesh.InPrevVATTransitionCursorsOffset =	m_AdditionalStreamOffsets[StreamOffset_PrevVATTransitionCursors].Valid() ?	(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_PrevVATTransitionCursors] / sizeof(float)) + buffersOffset) : outUniformsSkelMesh.InVATTransitionCursorsOffset;

	outUniformsSkelMesh.InEmissiveColorsOffset =			m_AdditionalStreamOffsets[StreamOffset_EmissiveColors].Valid() ?			(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_EmissiveColors] / sizeof(float)) + buffersOffset * 3) : -1;

	outUniformsSkelMesh.InColorsOffset =					m_AdditionalStreamOffsets[StreamOffset_Colors].Valid() ?					(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_Colors] / sizeof(float)) + buffersOffset * 4) : -1;
	outUniformsSkelMesh.InDynamicParameter0sOffset =		m_AdditionalStreamOffsets[StreamOffset_DynParam0s].Valid() ?				(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_DynParam0s] / sizeof(float)) + buffersOffset * 4) : -1;
	outUniformsSkelMesh.InDynamicParameter1sOffset =		m_AdditionalStreamOffsets[StreamOffset_DynParam1s].Valid() ?				(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_DynParam1s] / sizeof(float)) + buffersOffset * 4) : -1;
	outUniformsSkelMesh.InDynamicParameter2sOffset =		m_AdditionalStreamOffsets[StreamOffset_DynParam2s].Valid() ?				(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_DynParam2s] / sizeof(float)) + buffersOffset * 4) : -1;
	outUniformsSkelMesh.InDynamicParameter3sOffset =		m_AdditionalStreamOffsets[StreamOffset_DynParam3s].Valid() ?				(static_cast<s32>(m_AdditionalStreamOffsets[StreamOffset_DynParam3s] / sizeof(float)) + buffersOffset * 4) : -1;

	outUniformsSkelMesh.InMatricesOffset =					buffersOffset * 16;
	outUniformsSkelMesh.InPrevMatricesOffset =				m_PrevMatricesOffset + buffersOffset * 16;

	outUniformsSkelMesh.BoneIndicesReorder = m_BoneIndicesReorder.Buffer()->SRV();

	outUniformsSkelMesh.SkinningBufferStride = m_BoneMatricesBufferStride;
	outUniformsSkelMesh.BoneMatricesOffset = buffersOffset * m_BoneMatricesBufferStride;

	outUniformsSkelMesh.SkeletalAnimationSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp, 0, 0>::GetRHI();
	outUniformsSkelMesh.SkeletalAnimationTexture = matDesc.m_SkeletalAnimationTexture->TextureReference.TextureReferenceRHI;
	outUniformsSkelMesh.SkeletalAnimationPosBoundsMin = matDesc.m_SkeletalAnimationPosBoundsMin;
	outUniformsSkelMesh.SkeletalAnimationPosBoundsMax = matDesc.m_SkeletalAnimationPosBoundsMax;
	outUniformsSkelMesh.LinearInterpolateTransforms = matDesc.m_SkeletalAnimationLinearInterpolate;
	outUniformsSkelMesh.LinearInterpolateTracks = matDesc.m_SkeletalAnimationLinearInterpolateTracks;

	const FVector2f		texDimensions(matDesc.m_SkeletalAnimationTexture->GetSizeX(), matDesc.m_SkeletalAnimationTexture->GetSizeY());
	outUniformsSkelMesh.InvSkeletalAnimationTextureDimensions = FVector2f(1.0f) / texDimensions;
	outUniformsSkelMesh.SkeletalAnimationYOffsetPerTrack = (texDimensions.Y / (float)matDesc.m_SkeletalAnimationCount) * outUniformsSkelMesh.InvSkeletalAnimationTextureDimensions.Y;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_SkeletalMesh_CPUBB::_CreateSkelMeshVertexFactory(	CMaterialDesc_RenderThread		&matDesc,
																		u32								buffersOffset,
																		FMeshElementCollector			*collector,
																		FPopcornFXSkelMeshVertexFactory	*&outFactory,
																		FPopcornFXMeshCollector			*&outCollectorRes)
{
	// Resets m_VFData, that is *copied* per VF
#if (PK_PREBUILD_BONE_TRANSFORMS != 0)
	m_VFData.m_BoneMatrices = m_BoneMatrices.SRV;
	m_VFData.m_PreviousBoneMatrices = m_MotionBlur ? m_PreviousBoneMatrices.SRV : m_BoneMatrices.SRV;
#endif // (PK_PREBUILD_BONE_TRANSFORMS != 0)

	FPopcornFXUniforms			vsUniforms;
	FPopcornFXSkelMeshUniforms	uniformsSkelMesh;
	_FillUniforms(matDesc, buffersOffset, vsUniforms, uniformsSkelMesh);

	m_VFData.bInitialized = true;

	// Create the vertex factory
	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::IssueDrawCall_Mesh CollectorRes");

		outCollectorRes = &(collector->AllocateOneFrameResource<FPopcornFXMeshCollector>());
		check(outCollectorRes != null);

		PK_ASSERT(outCollectorRes->m_VertexFactory == null);
		outCollectorRes->m_VertexFactory = new FPopcornFXSkelMeshVertexFactory(m_FeatureLevel);

		outFactory = outCollectorRes->m_VertexFactory;
		outFactory->SetData(m_VFData);
		outFactory->m_VSUniformBuffer = FPopcornFXUniformsRef::CreateUniformBufferImmediate(vsUniforms, UniformBuffer_SingleFrame);
		outFactory->m_SkelMeshVSUniformBuffer = FPopcornFXSkelMeshUniformsRef::CreateUniformBufferImmediate(uniformsSkelMesh, UniformBuffer_SingleFrame);
		PK_ASSERT(!outFactory->IsInitialized());
		outFactory->InitResource();
	}
}

//----------------------------------------------------------------------------

#if RHI_RAYTRACING
void	CBatchDrawer_SkeletalMesh_CPUBB::_IssueDrawCall_Mesh_AccelStructs(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
#if 0 // TODO
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::IssueDrawCall_Mesh");

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
		const u32	sectionCount = matDesc.m_SkelMeshLODRenderData->RenderSections.Num();
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
			if (!PK_VERIFY(_BuildMeshBatch_Mesh(meshBatch, sceneProxy, view->SceneViews()[0].m_SceneView, matDesc, LODRenderData, iSection, sectionPCount, 0/*TODO*/)))
				return;

			meshBatch.VertexFactory = vertexFactory;
			meshBatch.Elements[0].VertexFactoryUserData = vertexFactory->GetUniformBuffer();
			meshBatch.SegmentIndex = 0;

			INC_DWORD_STAT_BY(STAT_PopcornFX_RayTracing_DrawCallsCount, 1);
			rayTracingInstance.Materials.Add(meshBatch);

			// Temp code, until we know for sure if we can use the gpu buffer matrices directly (untouched)
			rayTracingInstance.InstanceTransforms.Append(reinterpret_cast<FMatrix*>(rayTracing_InstancedMatrices + buffersOffset), sectionPCount);
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 1)
			rayTracingInstance.BuildInstanceMaskAndFlags(m_FeatureLevel);
#else
			rayTracingInstance.BuildInstanceMaskAndFlags();
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)

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

			if (!PK_VERIFY(_BuildMeshBatch_Mesh(meshBatch, sceneProxy, view->SceneViews()[0].m_SceneView, matDesc, LODRenderData, iSection, m_TotalParticleCount, 0/*TODO*/)))
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
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 1)
			rayTracingInstance.BuildInstanceMaskAndFlags(m_FeatureLevel);
#else
			rayTracingInstance.BuildInstanceMaskAndFlags();
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)

			view->OutRayTracingInstances()->Add(rayTracingInstance);
		}
	}
#endif
}
#endif // RHI_RAYTRACING

//----------------------------------------------------------------------------

bool	CBatchDrawer_SkeletalMesh_CPUBB::_BuildMeshBatch_Mesh(	FMeshBatch							&meshBatch,
																const FPopcornFXSceneProxy			*sceneProxy,
																const FSceneView					*view,
																const CMaterialDesc_RenderThread	&matDesc,
																const FSkeletalMeshLODRenderData	&LODRenderData,
																u32									LODLevel,
																u32									iSection,
																u32									sectionParticleCount,
																u32									boneIndicesReorderOffset)
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

	const FSkelMeshRenderSection	&section = LODRenderData.RenderSections[iSection];
	if (!PK_VERIFY(section.NumTriangles > 0))
		return false;
	FMeshBatchElement	&batchElement = meshBatch.Elements[0];

	batchElement.FirstIndex = section.BaseIndex;
	batchElement.MinVertexIndex = section.GetVertexBufferIndex();
	batchElement.MaxVertexIndex = section.GetVertexBufferIndex() + section.GetNumVertices();
	batchElement.NumInstances = sectionParticleCount; // Ignored by raytracing pass
	batchElement.IndexBuffer = LODRenderData.MultiSizeIndexContainer.GetIndexBuffer();
	batchElement.NumPrimitives = section.NumTriangles;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	batchElement.VisualizeElementIndex = iSection;

	meshBatch.LODIndex = LODLevel;
	meshBatch.VisualizeLODIndex = LODLevel;
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	batchElement.UserData = reinterpret_cast<void*>(u64(boneIndicesReorderOffset));

	batchElement.UserIndex = 0;

	return true;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_SkeletalMesh_CPUBB::_IssueDrawCall_Mesh_Sections(	const FPopcornFXSceneProxy				*sceneProxy,
																		const SUERenderContext					&renderContext,
																		const PopcornFX::SDrawCallDesc			&desc,
																		const PopcornFX::TMemoryView<const u32>	&perSectionPCount,
																		CMaterialDesc_RenderThread				&matDesc,
																		u32										&buffersOffset,
																		u32										&boneIndicesReorderOffset,
																		u32										LODLevel,
																		FMeshElementCollector					*collector)
{
	const FSkeletalMeshRenderData	*renderData = matDesc.m_SkeletalMeshRenderData;
	CRendererSubView				*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);
	PK_ASSERT(renderData != null);

	// Render a single LOD level
	PK_ASSERT(LODLevel < (u32)renderData->LODRenderData.Num());
	PK_ASSERT(perSectionPCount.Count() == (u32)renderData->LODRenderData[LODLevel].RenderSections.Num());
	const FSkeletalMeshLODRenderData	&LODRenderData = renderData->LODRenderData[LODLevel];

	// Could we do this elsewhere ? .. Not per draw call ?
	_PreSetupMeshData(m_VFData, LODRenderData);

	// Vertex factory and collector resource are either used to render all sections (if all sections of the mesh are being rendered of m_TotalParticleCount)
	// If there is a mesh atlas, we have to create a vertex factory per mesh section, because we cannot specify per FMeshBatchElement an offset into the instances buffers
	FPopcornFXSkelMeshVertexFactory	*vertexFactory = null;
	FPopcornFXMeshCollector			*collectorRes = null;
	if (!m_HasMeshIDs)
		_CreateSkelMeshVertexFactory(matDesc, 0, collector, vertexFactory, collectorRes);

	PK_ASSERT(LODLevel < (u32)matDesc.m_SkeletalMeshLODInfos.Count() || matDesc.m_SkeletalMeshLODInfos.Empty());
	const FSkeletalMeshLODInfo	*LODInfo = !matDesc.m_SkeletalMeshLODInfos.Empty() ? &matDesc.m_SkeletalMeshLODInfos[LODLevel] : null;

	PK_ASSERT(desc.m_ViewIndex < m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() == m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() <= renderContext.m_RendererSubView->SceneViews().Count());
	for (u32 iView = 0; iView < m_RealViewCount; ++iView)
	{
		if (desc.m_ViewIndex != iView)
			continue;

		const u32	realViewIndex = renderContext.m_RendererSubView->BBViews()[iView].m_ViewIndex;
		const u32	sectionCount = LODRenderData.RenderSections.Num();
		for (u32 iSection = 0; iSection < sectionCount; ++iSection)
		{
			const u32	sectionPCount = (m_HasMeshIDs || matDesc.m_PerParticleLOD) ? perSectionPCount[iSection] : m_TotalParticleCount;
			const u32	renderSectionId = LODInfo != null && LODInfo->LODMaterialMap.Num() > 0 ? LODInfo->LODMaterialMap[iSection] : iSection;

			if (sectionPCount > 0)
			{
				if (m_HasMeshIDs)
					_CreateSkelMeshVertexFactory(matDesc, buffersOffset, collector, vertexFactory, collectorRes);

				PK_ASSERT(view->RenderPass() != CRendererSubView::RenderPass_Shadow || matDesc.m_CastShadows);

				FMeshBatch	&meshBatch = collector->AllocateMesh();

				if (!PK_VERIFY(_BuildMeshBatch_Mesh(meshBatch, sceneProxy, view->SceneViews()[realViewIndex].m_SceneView, matDesc, LODRenderData, LODLevel, renderSectionId, sectionPCount, boneIndicesReorderOffset)))
					return;

				meshBatch.VertexFactory = vertexFactory;
				meshBatch.Elements[0].PrimitiveUniformBuffer = sceneProxy->GetUniformBuffer();

				buffersOffset += m_HasMeshIDs ? sectionPCount : 0;

				{
					INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsCount, 1);
					INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsSkelMeshCount, 1);
					collector->AddMesh(realViewIndex, meshBatch);
				}
			}
			boneIndicesReorderOffset += LODRenderData.RenderSections[renderSectionId].BoneMap.Num();
		} // for (sections)
	} // for (views)
}

//----------------------------------------------------------------------------

void	CBatchDrawer_SkeletalMesh_CPUBB::_IssueDrawCall_Mesh(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::IssueDrawCall_Mesh");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);

	FMeshElementCollector		*collector = view->Collector();
	const FPopcornFXSceneProxy	*sceneProxy = view->SceneProxy();
	PK_ASSERT(collector != null);
	PK_ASSERT(sceneProxy != null);

	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return;
	CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();
	PK_ASSERT(matDesc.ValidForRendering());

	const FSkeletalMeshRenderData	*renderData = matDesc.m_SkeletalMeshRenderData;
	PK_ASSERT(matDesc.m_BaseLODLevel < (u32)renderData->LODRenderData.Num());
	const u32						LODCount = matDesc.m_PerParticleLOD ? renderData->LODRenderData.Num() - matDesc.m_BaseLODLevel : 1;

	PK_ONLY_IF_ASSERTS(
		u32	_sectionCount = 0;
		for (u32 i = 0; i < LODCount; ++i)
			_sectionCount += renderData->LODRenderData[i].RenderSections.Num();
		PK_ASSERT(_sectionCount == m_PerMeshParticleCount.Count());
	);

#if (PK_PREBUILD_BONE_TRANSFORMS != 0)
	{
		FRHICommandListImmediate	&RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

		FPopcornFXUniforms			uniforms;
		FPopcornFXSkelMeshUniforms	uniformsSkelMesh;
		_FillUniforms(matDesc, 0, uniforms, uniformsSkelMesh);

		FPopcornFXComputeBoneTransformsCS_Params	params;
		params.m_TotalParticleCount = m_TotalParticleCount;
		params.m_TotalBoneCount = m_TotalBoneCount;
		params.m_BoneMatrices = m_BoneMatrices.UAV;
		params.m_PrevBoneMatrices = m_MotionBlur ? m_PreviousBoneMatrices.UAV : m_BoneMatrices.UAV;
		params.m_UniformBuffer = FPopcornFXUniformsRef::CreateUniformBufferImmediate(uniforms, UniformBuffer_SingleFrame);
		params.m_MeshUniformBuffer = FPopcornFXSkelMeshUniformsRef::CreateUniformBufferImmediate(uniformsSkelMesh, UniformBuffer_SingleFrame);

		SCOPED_DRAW_EVENT(RHICmdList, PopcornFXComputeBoneTransformsCS);
		SCOPED_GPU_STAT(RHICmdList, PopcornFXComputeBoneTransformsCS);
		if (m_MotionBlur)
		{
			TShaderMapRef< FPopcornFXComputeMBBoneTransformsCS >	computeBoneTransformsCS(GetGlobalShaderMap(m_FeatureLevel));
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
			SetComputePipelineState(RHICmdList, computeBoneTransformsCS.GetComputeShader());
#else
			RHICmdList.SetComputeShader(computeBoneTransformsCS.GetComputeShader());
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
			computeBoneTransformsCS->Dispatch(RHICmdList, params);
		}
		else
		{
			TShaderMapRef< FPopcornFXComputeBoneTransformsCS >	computeBoneTransformsCS(GetGlobalShaderMap(m_FeatureLevel));
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
			SetComputePipelineState(RHICmdList, computeBoneTransformsCS.GetComputeShader());
#else
			RHICmdList.SetComputeShader(computeBoneTransformsCS.GetComputeShader());
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
			computeBoneTransformsCS->Dispatch(RHICmdList, params);
		}
	}
#endif // (PK_PREBUILD_BONE_TRANSFORMS != 0)

	if (matDesc.m_PerParticleLOD)
	{
		u32		sectionsStart = 0;
		u32		buffersOffset = 0;
		u32		boneIndicesReorderOffset = 0;
		for (u32 i = matDesc.m_BaseLODLevel; i < LODCount; ++i)
		{
			const u32	LODSectionCount = renderData->LODRenderData[i].RenderSections.Num();
			_IssueDrawCall_Mesh_Sections(sceneProxy, renderContext, desc, m_PerMeshParticleCount.Slice(sectionsStart, LODSectionCount), matDesc, buffersOffset, boneIndicesReorderOffset, i, collector);
			sectionsStart += LODSectionCount;
		}
	}
	else
	{
		u32		buffersOffset = 0;
		u32		boneIndicesReorderOffset = 0;
		_IssueDrawCall_Mesh_Sections(sceneProxy, renderContext, desc, m_PerMeshParticleCount, matDesc, buffersOffset, boneIndicesReorderOffset, matDesc.m_BaseLODLevel, collector);
	}
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_SkeletalMesh_CPUBB::EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SRendererBatchDrawPass &drawPass, const PopcornFX::SDrawCallDesc &toEmit)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_SkeletalMesh_CPUBB::EmitDrawCall");
	PK_ASSERT(toEmit.m_TotalParticleCount <= m_TotalParticleCount); // <= if slicing is enabled
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
