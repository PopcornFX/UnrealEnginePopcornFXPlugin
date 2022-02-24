//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "RenderPolicies.h"
#include "RenderBatchManager.h"
#include "SceneManagement.h"
#include "MaterialDesc.h"
#include "StaticMeshResources.h"

#if (PLATFORM_PS4)
//	Since 4.19.1/2.. can't include AudioDevice.h anymore because of AudioDeviceManager.h
//	raising:
//		Runtime/Engine/Public/AudioDeviceManager.h(135,2): error : declaration does not declare anything [-Werror,-Wmissing-declarations]
//		          void TrackResource(USoundWave* SoundWave, FSoundBuffer* Buffer);
//		          ^~~~
//	So we now use GameplayStatics directly

#	include "Kismet/GameplayStatics.h"
#else
#	include "AudioDevice.h"
#endif // PLATFORM_PS4

#include "Engine/Engine.h"
#include "World/PopcornFXSceneProxy.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "Render/PopcornFXVertexFactory.h"
#include "Render/PopcornFXMeshVertexFactory.h"
#include "GPUSim/PopcornFXBillboarderCS.h"
#include "GPUSim/PopcornFXGPUSim.h"
#include "PopcornFXStats.h"

#include <pk_render_helpers/include/basic_renderer_properties/rh_basic_renderer_properties.h>
#include <pk_render_helpers/include/basic_renderer_properties/rh_vertex_animation_renderer_properties.h>

DECLARE_GPU_STAT_NAMED(PopcornFXBillboardBillboarding, TEXT("PopcornFX Billboards Billboarding"));
DECLARE_GPU_STAT_NAMED(PopcornFXMeshBillboarding, TEXT("PopcornFX Meshes Billboarding"));
DECLARE_GPU_STAT_NAMED(PopcornFXBillboardSort, TEXT("PopcornFX Billboards Sorting"));

//----------------------------------------------------------------------------
//
// FPopcornFXAtlasRectsVertexBuffer
//
//----------------------------------------------------------------------------

bool	FPopcornFXAtlasRectsVertexBuffer::LoadRects(const SRenderContext &renderContext, const TMemoryView<const CFloat4> &rects)
{
	if (rects.Empty())
	{
		Clear();
		return false;
	}
	if (!_LoadRects(renderContext, rects))
	{
		Clear();
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	FPopcornFXAtlasRectsVertexBuffer::_LoadRects(const SRenderContext &renderContext, const TMemoryView<const CFloat4> &rects)
{
	const u32		bytes = rects.CoveredBytes();
	const bool		empty = m_AtlasBuffer_Raw == null;

	if (empty || bytes > m_AtlasBufferCapacity)
	{
		m_AtlasBufferCapacity = PopcornFX::Mem::Align<0x100>(bytes + 0x10);

		FRHIResourceCreateInfo	info;
		const uint32	usage = BUF_Static | BUF_ShaderResource;

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

	void	*map = RHILockVertexBuffer(m_AtlasBuffer_Raw, 0, bytes, RLM_WriteOnly);
	if (!PK_VERIFY(map != null))
		return false;
	PopcornFX::Mem::Copy(map, rects.Data(), bytes);
	RHIUnlockVertexBuffer(m_AtlasBuffer_Raw);

	m_AtlasRectsCount = rects.Count();
	return true;
}

//----------------------------------------------------------------------------

void	CRenderBatchPolicy::SAdditionalInput::UnmapBuffer()
{
	m_VertexBuffer.Unmap();
}

//----------------------------------------------------------------------------

void	CRenderBatchPolicy::SAdditionalInput::ClearBuffer()
{
	m_VertexBuffer.UnmapAndClear();
	m_ByteSize = 0;
}

//----------------------------------------------------------------------------

void	CRenderBatchPolicy::SViewDependent::UnmapBuffers()
{
	m_Indices.Unmap();
	m_Positions.Unmap();
	m_Normals.Unmap();
	m_Tangents.Unmap();
	m_UVFactors.Unmap();
}

//----------------------------------------------------------------------------

void	CRenderBatchPolicy::SViewDependent::ClearBuffers()
{
	m_Indices.UnmapAndClear();
	m_Positions.UnmapAndClear();
	m_Normals.UnmapAndClear();
	m_Tangents.UnmapAndClear();
	m_UVFactors.UnmapAndClear();
}

//----------------------------------------------------------------------------

CRenderBatchPolicy::CRenderBatchPolicy()
:	m_TotalParticleCount(0)
,	m_TotalVertexCount(0)
,	m_TotalIndexCount(0)
,	m_RendererType(0)
{
}

//----------------------------------------------------------------------------

CRenderBatchPolicy::~CRenderBatchPolicy()
{
	_ClearBuffers();

	PK_ASSERT(!m_Indices.Valid());
	PK_ASSERT(!m_Positions.Valid());
	PK_ASSERT(!m_Normals.Valid());
	PK_ASSERT(!m_Tangents.Valid());
	PK_ASSERT(!m_Texcoords.Valid());
	PK_ASSERT(!m_Texcoord2s.Valid());
	PK_ASSERT(!m_AtlasIDs.Valid());
	PK_ASSERT(!m_UVRemaps.Valid());
	PK_ASSERT(!m_UVFactors.Valid());
	PK_ASSERT(!m_Instanced_Matrices.Valid());

	PK_ASSERT(!m_SimData.Valid());

#if RHI_RAYTRACING
	PK_ASSERT(m_Mapped_RayTracing_Matrices.Empty());
#endif // RHI_RAYTRACING

	PK_ASSERT(m_Mapped_Matrices.Empty());
	PK_ASSERT(m_Mapped_Param_Colors.Empty());
	PK_ASSERT(m_Mapped_Param_EmissiveColors.Empty());
	PK_ASSERT(m_Mapped_Param_DynamicParameter0.Empty());
	PK_ASSERT(m_Mapped_Param_DynamicParameter1.Empty());
	PK_ASSERT(m_Mapped_Param_DynamicParameter2.Empty());
	//PK_ASSERT(m_Mapped_Param_DynamicParameter3.Empty());

	for (u32 iField = 0; iField < m_AdditionalInputs.Count(); ++iField)
		PK_ASSERT(!m_AdditionalInputs[iField].m_VertexBuffer.Valid());

	for (u32 iView = 0; iView < m_ViewDependents.Count(); ++iView)
	{
		PK_ASSERT(!m_ViewDependents[iView].m_Indices.Valid());
		PK_ASSERT(!m_ViewDependents[iView].m_Positions.Valid());
		PK_ASSERT(!m_ViewDependents[iView].m_Normals.Valid());
		PK_ASSERT(!m_ViewDependents[iView].m_Tangents.Valid());
		PK_ASSERT(!m_ViewDependents[iView].m_UVFactors.Valid());
	}
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::CanRender(const PopcornFX::Drawers::SBillboard_DrawRequest *request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext &ctx)
{
	PK_ASSERT(request != null);
	PK_ASSERT(rendererCache != null);
	PK_ASSERT(ctx.m_RendererSubView != null);

	CMaterialDesc_RenderThread	&matDesc = static_cast<CRendererCache*>(rendererCache.Get())->RenderThread_Desc();

	return ctx.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Main ||
		(ctx.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Shadow && matDesc.m_CastShadows);
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::CanRender(const PopcornFX::Drawers::SRibbon_DrawRequest *request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext &ctx)
{
	PK_ASSERT(request != null);
	PK_ASSERT(rendererCache != null);
	PK_ASSERT(ctx.m_RendererSubView != null);

	CMaterialDesc_RenderThread	&matDesc = static_cast<CRendererCache*>(rendererCache.Get())->RenderThread_Desc();

	return ctx.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Main ||
		(ctx.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Shadow && matDesc.m_CastShadows);
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::CanRender(const PopcornFX::Drawers::STriangle_DrawRequest *request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext &ctx)
{
	PK_ASSERT(request != null);
	PK_ASSERT(rendererCache != null);
	PK_ASSERT(ctx.m_RendererSubView != null);

	CMaterialDesc_RenderThread	&matDesc = static_cast<CRendererCache*>(rendererCache.Get())->RenderThread_Desc();

	return ctx.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Main ||
		(ctx.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Shadow && matDesc.m_CastShadows);
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::CanRender(const PopcornFX::Drawers::SMesh_DrawRequest *request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext &ctx)
{
	PK_ASSERT(request != null);
	PK_ASSERT(rendererCache != null);
	PK_ASSERT(ctx.m_RendererSubView != null);

	CMaterialDesc_RenderThread	&matDesc = static_cast<CRendererCache*>(rendererCache.Get())->RenderThread_Desc();

	return ctx.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Main ||
#if RHI_RAYTRACING
		(ctx.m_RendererSubView->Pass() == CRendererSubView::RenderPass_RT_AccelStructs && matDesc.m_Raytraced) ||
#endif
		(ctx.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Shadow && matDesc.m_CastShadows);
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::CanRender(const PopcornFX::Drawers::SLight_DrawRequest *request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext &ctx)
{
	PK_ASSERT(request != null);
	PK_ASSERT(rendererCache != null);
	PK_ASSERT(ctx.m_RendererSubView != null);
	return ctx.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Main;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::CanRender(const PopcornFX::Drawers::SSound_DrawRequest *request, const PopcornFX::PRendererCacheBase rendererCache, const SRenderContext &ctx)
{
	PK_ASSERT(request != null);
	PK_ASSERT(rendererCache != null);
	PK_ASSERT(ctx.m_RendererSubView != null);
	return ctx.m_RendererSubView->Pass() == CRendererSubView::UpdatePass_PostUpdate;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::Tick(const SRenderContext &renderContext, const TMemoryView<SBBView> &views)
{
	if (m_UnusedCounter++ > 10)
		return false;
	INC_DWORD_STAT_BY(STAT_PopcornFX_BatchesCount, 1);
	return true;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::AllocBuffers(const SRenderContext &renderContext, const PopcornFX::SBuffersToAlloc &allocBuffers, const TMemoryView<SBBView> &views, PopcornFX::ERendererClass rendererType)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::AllocBuffers");

	m_UnusedCounter = 0;

	m_TotalParticleCount = allocBuffers.m_TotalParticleCount;
	m_TotalVertexCount = allocBuffers.m_TotalVertexCount;
	m_TotalIndexCount = allocBuffers.m_TotalIndexCount;
	m_PerMeshParticleCount = allocBuffers.m_PerMeshParticleCount;

	m_RendererType = rendererType;
	if (rendererType == PopcornFX::ERendererClass::Renderer_Light ||
		rendererType == PopcornFX::ERendererClass::Renderer_Sound)
	{
		PK_ASSERT(allocBuffers.m_TotalVertexCount == 0 && allocBuffers.m_TotalIndexCount == 0);
		PK_ASSERT(allocBuffers.m_TotalParticleCount > 0);
		return true;
	}

	m_HasMeshIDs = allocBuffers.m_HasMeshIDs;
	PK_ASSERT(renderContext.m_RendererSubView != null);
	m_FeatureLevel = renderContext.m_RendererSubView->ViewFamily()->GetFeatureLevel();

	PK_ASSERT(allocBuffers.m_DrawRequests.First() != null);
	const bool	gpuStorage = allocBuffers.m_DrawRequests.First()->GPUStorage();
	if (gpuStorage)
	{
		if (rendererType != PopcornFX::ERendererClass::Renderer_Billboard &&
			rendererType != PopcornFX::ERendererClass::Renderer_Mesh)
		{
			PK_ASSERT_NOT_REACHED();
			return false;
		}
	}

	PK_ASSERT((allocBuffers.m_TotalVertexCount > 0 && allocBuffers.m_TotalIndexCount > 0) ||
			  allocBuffers.m_TotalParticleCount > 0);

	m_RealViewCount = views.Count();

	CRenderBatchManager	*rbManager = renderContext.m_RenderBatchManager;
	PK_ASSERT(rbManager != null);

	CVertexBufferPool	*mainVBPool = gpuStorage ? &rbManager->VBPool_GPU() : &rbManager->VBPool();
	CIndexBufferPool	*mainIBPool = gpuStorage ? &rbManager->IBPool_GPU() : &rbManager->IBPool();
	CVertexBufferPool	*particleDataVBPool = &rbManager->VBPool_VertexBB();
	PK_ASSERT(mainVBPool != null);
	PK_ASSERT(mainIBPool != null);
	PK_ASSERT(particleDataVBPool != null);

	const u32	totalParticleCount = m_TotalParticleCount;
	const u32	totalVertexCount = m_TotalVertexCount;
	const u32	totalIndexCount = m_TotalIndexCount;

	// View independent
	const PopcornFX::SGeneratedInputs	&toGenerate = allocBuffers.m_ToGenerate;

	const bool	needsIndices = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Indices) != 0;
	const bool	needsPositions = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Position) != 0;
	const bool	needsNormals = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Normal) != 0;
	const bool	needsTangents = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Tangent) != 0;

	// Ribbon complex
	const bool	needsUVRemaps = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UVRemap) != 0;
	const bool	needsUVFactors = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UVFactors) != 0;

	PK_ASSERT(!needsUVRemaps || rendererType == PopcornFX::ERendererClass::Renderer_Ribbon);
	PK_ASSERT(!needsUVFactors || rendererType == PopcornFX::ERendererClass::Renderer_Ribbon);

	// Billboard View independent
	const bool	needsUV0 = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UV0) != 0;
	const bool	needsUV1 = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UV1) != 0;
	const bool	needsAtlasIDs = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_AtlasId) != 0;

	// Meshes
	const bool	needsMatrices = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Matrices) != 0;

	const u32	texcoordsStride = gpuStorage && PK_BILLBOARDER_CS_OUTPUT_PACK_TEXCOORD ? sizeof(float) : sizeof(CFloat2);
	const u32	ptnStride = gpuStorage && PK_BILLBOARDER_CS_OUTPUT_PACK_PTN ? sizeof(CFloat3) : sizeof(CFloat4);
	const u32	colorsStride = gpuStorage && PK_BILLBOARDER_CS_OUTPUT_PACK_COLOR_F16 ? sizeof(CFloat2) : sizeof(CFloat4);
	const u32	emissiveColorsStride = sizeof(CFloat3);
	bool		largeIndices = false;
	bool		hasNormals = needsNormals;
	bool		hasUVFactors = needsUVFactors || needsUVRemaps;

	const bool		meshRenderer = rendererType == PopcornFX::ERendererClass::Renderer_Mesh;
	const bool		billboardRenderer = rendererType == PopcornFX::ERendererClass::Renderer_Billboard;
	{
		PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::AllocBuffers_ViewIndependent");

		if (!meshRenderer)
		{
			if (!mainIBPool->AllocateIf(needsIndices, m_Indices, largeIndices, totalIndexCount, false))
				return false;
			if (!mainVBPool->AllocateIf(needsPositions, m_Positions, totalVertexCount, ptnStride, false))
				return false;
			if (!mainVBPool->AllocateIf(needsNormals, m_Normals, totalVertexCount, ptnStride, false))
				return false;
			if (!mainVBPool->AllocateIf(needsTangents, m_Tangents, totalVertexCount, ptnStride, false))
				return false;
			if (!mainVBPool->AllocateIf(needsUV0, m_Texcoords, totalVertexCount, texcoordsStride, false))
				return false;
			if (!mainVBPool->AllocateIf(needsUV1, m_Texcoord2s, totalVertexCount, texcoordsStride, false))
				return false;
			if (!mainVBPool->AllocateIf(needsAtlasIDs, m_AtlasIDs, totalVertexCount, sizeof(float), false))
				return false;
			if (!mainVBPool->AllocateIf(needsUVRemaps, m_UVRemaps, totalVertexCount, sizeof(CFloat4), false))
				return false;
			if (!mainVBPool->AllocateIf(needsUVFactors, m_UVFactors, totalVertexCount, sizeof(CFloat4), false))
				return false;
		}
		else
		{
#if (ENGINE_MINOR_VERSION < 25)
			m_Instanced = GRHISupportsInstancing; // Supported everywhere since 4.25
#endif // (ENGINE_MINOR_VERSION < 25)

#if (ENGINE_MINOR_VERSION < 25)
			if (m_Instanced)
			{
#endif // (ENGINE_MINOR_VERSION < 25)
#if RHI_RAYTRACING
				// Tests: raytracing ? billboard into main memory buffer, then copy to gpu buffer
				const bool	isRayTracingPass = renderContext.m_RendererSubView->RenderPass() == CRendererSubView::RenderPass_RT_AccelStructs;
				if (isRayTracingPass)
				{
					if (gpuStorage)
						return false; // not supported yet

					// Allocate our data here
					if (!PK_VERIFY(m_RayTracing_Matrices.GetSome<CFloat4x4>(m_TotalParticleCount) != null))
						return false;
				}
#endif // RHI_RAYTRACING
				if (!mainVBPool->AllocateIf(needsMatrices, m_Instanced_Matrices, m_TotalParticleCount, sizeof(CFloat4x4), false))
					return false;
#if (ENGINE_MINOR_VERSION < 25)
			}
			else
			{
				// No Vertex buffer to allocate, we use a CWorkingBuffer
			}
#endif // (ENGINE_MINOR_VERSION < 25)
		}
	}

	if (allocBuffers.m_IsNewFrame)
	{
		PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::AllocBuffers_AdditionalInputs");

		// Additional inputs sent to shaders (AlphaRemapCursor, Colors, ..)
		const u32	instanceCount = meshRenderer ? totalParticleCount : totalVertexCount;
		const u32	aFieldCount = toGenerate.m_AdditionalGeneratedInputs.Count();

		m_AdditionalInputs.Clear();
		if (!PK_VERIFY(m_AdditionalInputs.Reserve(aFieldCount))) // Max possible additional field count
			return false;

		m_SimDataBufferSizeInBytes = 0;
		for (u32 iField = 0; iField < aFieldCount; ++iField)
		{
			const PopcornFX::Drawers::SBase_BillboardingRequest::SAdditionalInputInfo	&additionalInput = toGenerate.m_AdditionalGeneratedInputs[iField];

			const PopcornFX::CStringId			&fieldName = additionalInput.m_Name;
			u32									typeSize = PopcornFX::CBaseTypeTraits::Traits(additionalInput.m_Type).Size;
			const u32							fieldID = m_AdditionalInputs.Count();
			EPopcornFXAdditionalStreamOffsets	streamOffsetType = EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount;

			if (!_IsAdditionalInputSupported(fieldName, meshRenderer, additionalInput.m_Type, streamOffsetType))
				continue; // Unsupported shader input, discard
			PK_ASSERT(streamOffsetType < EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount);
			m_AdditionalStreamOffsets[streamOffsetType].Setup(m_SimDataBufferSizeInBytes, iField);

			if (!PK_VERIFY(m_AdditionalInputs.PushBack().Valid()))
				return false;
			SAdditionalInput	&newAdditionalInput = m_AdditionalInputs.Last();

#if (ENGINE_MINOR_VERSION < 25)
			if (m_Instanced)
			{
				// Will be modified in v2.10
#endif // (ENGINE_MINOR_VERSION < 25)
				if (!mainVBPool->Allocate(newAdditionalInput.m_VertexBuffer, instanceCount, typeSize))
					return false;
#if (ENGINE_MINOR_VERSION < 25)
			}
#endif // (ENGINE_MINOR_VERSION < 25)

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

	const u32	activeViewCount = toGenerate.m_PerViewGeneratedInputs.Count();
	if (!PK_VERIFY(m_ViewDependents.Resize(activeViewCount)))
		return false;
	if (activeViewCount > 0)
	{
		PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::AllocBuffers_ViewDependents");

		// View deps
		for (u32 iView = 0; iView < activeViewCount; ++iView)
		{
			const u32	inputFlags = toGenerate.m_PerViewGeneratedInputs[iView].m_GeneratedInputs;
			if (inputFlags == 0)
				continue;

			const bool	viewNeedsIndices = (inputFlags & PopcornFX::Drawers::GenInput_Indices) != 0;
			const bool	viewNeedsPositions = (inputFlags & PopcornFX::Drawers::GenInput_Position) != 0;
			const bool	viewNeedsNormals = (inputFlags & PopcornFX::Drawers::GenInput_Normal) != 0;
			const bool	viewNeedsTangents = (inputFlags & PopcornFX::Drawers::GenInput_Tangent) != 0;
			const bool	viewNeedsUVFactors = (inputFlags & PopcornFX::Drawers::GenInput_UVFactors) != 0;

			SViewDependent	&viewDep = m_ViewDependents[iView];

			PK_ASSERT(viewNeedsNormals == viewNeedsTangents);
			PK_ASSERT(!viewNeedsUVFactors || rendererType == PopcornFX::ERendererClass::Renderer_Ribbon);
			viewDep.m_ViewIndex = toGenerate.m_PerViewGeneratedInputs[iView].m_ViewIndex;
			viewDep.m_BBMatrix = views[viewDep.m_ViewIndex].m_InvViewMatrix; // Avoid copy if not necessary ?
			if (!mainIBPool->AllocateIf(viewNeedsIndices, viewDep.m_Indices, largeIndices, totalIndexCount, false))
				return false;
			if (!mainVBPool->AllocateIf(viewNeedsPositions, viewDep.m_Positions, totalVertexCount, ptnStride, false))
				return false;
			if (!mainVBPool->AllocateIf(viewNeedsNormals, viewDep.m_Normals, totalVertexCount, ptnStride, false))
				return false;
			if (!mainVBPool->AllocateIf(viewNeedsTangents, viewDep.m_Tangents, totalVertexCount, ptnStride, false))
				return false;
			if (!mainVBPool->AllocateIf(viewNeedsUVFactors, viewDep.m_UVFactors, totalVertexCount, sizeof(CFloat4), false))
				return false;

			hasNormals |= viewNeedsNormals;
			hasUVFactors |= viewNeedsUVFactors;
		}
	}
	else
	{
		PK_ASSERT(views.Count() == 0 || !allocBuffers.m_IsNewFrame || needsIndices || rendererType == PopcornFX::ERendererClass::Renderer_Mesh);
	}

	// !Store this during main pass!
	// Subsequent passes (ie. shadows) will not contain view independent flags
	// so for example 'needsAtlasIDs' will be false, but it fact it's needed
	if (allocBuffers.m_IsNewFrame)
	{
		m_NeedsBTN = hasNormals;
		m_CorrectRibbonDeformation = hasUVFactors;
		m_SecondUVSet = needsAtlasIDs;
		m_FlipUVs = false;
	}

	if (rendererType == PopcornFX::ERendererClass::Renderer_Ribbon)
	{
		const PopcornFX::Drawers::SRibbon_DrawRequest			*dr = static_cast<const PopcornFX::Drawers::SRibbon_DrawRequest*>(allocBuffers.m_DrawRequests.First());
		const PopcornFX::Drawers::SRibbon_BillboardingRequest	&bb = dr->m_BB;

		m_FlipUVs = bb.m_Flags.m_RotateTexture;
	}

	return true;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::_IsAdditionalInputSupported(const PopcornFX::CStringId &fieldName, bool meshRenderer, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets &outStreamOffsetType)
{
	if (type == PopcornFX::EBaseTypeID::BaseType_Float4)
	{
		if (fieldName == PopcornFX::BasicRendererProperties::SID_Diffuse_Color() ||
			fieldName == PopcornFX::BasicRendererProperties::SID_Distortion_Color() ||
			fieldName == PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_Color())
			outStreamOffsetType = StreamOffset_Colors;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput1_Input1())
			outStreamOffsetType = StreamOffset_DynParam1s;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput2_Input2())
			outStreamOffsetType = StreamOffset_DynParam2s;
		else if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput3_Input3() && !meshRenderer /* This will get removed in 2.10 */)
			outStreamOffsetType = StreamOffset_DynParam3s;
		if (meshRenderer)
		{
			if (fieldName == PopcornFX::BasicRendererProperties::SID_ShaderInput0_Input0())
				outStreamOffsetType = StreamOffset_DynParam0s;
		}
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
		if (meshRenderer)
		{
			if (fieldName == PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_Cursor() ||
				fieldName == PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft_Cursor() ||
				fieldName == PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid_Cursor())
				outStreamOffsetType = StreamOffset_VATCursors;
		}
	}
	return outStreamOffsetType != EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::UnmapBuffers(const SRenderContext &renderContext)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::UnmapBuffers");

	m_Indices.Unmap();
	m_Positions.Unmap();
	m_Normals.Unmap();
	m_Tangents.Unmap();
	m_Texcoords.Unmap();
	m_Texcoord2s.Unmap();
	m_AtlasIDs.Unmap();
	m_UVRemaps.Unmap();
	m_UVFactors.Unmap();

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

	for (u32 iView = 0; iView < m_ViewDependents.Count(); ++iView)
		m_ViewDependents[iView].UnmapBuffers();

	return true;
}

//----------------------------------------------------------------------------

void	CRenderBatchPolicy::ClearBuffers(const SRenderContext &renderContext)
{
	_ClearBuffers();
}

//----------------------------------------------------------------------------

void	CRenderBatchPolicy::_ClearBuffers()
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::_ClearBuffers");

	m_FeatureLevel = ERHIFeatureLevel::Num;
#if (ENGINE_MINOR_VERSION < 25)
	m_Instanced = false;
#endif // (ENGINE_MINOR_VERSION < 25)

	m_Indices.UnmapAndClear();
	m_Positions.UnmapAndClear();
	m_Normals.UnmapAndClear();
	m_Tangents.UnmapAndClear();
	m_Texcoords.UnmapAndClear();
	m_Texcoord2s.UnmapAndClear();
	m_AtlasIDs.UnmapAndClear();
	m_UVRemaps.UnmapAndClear();
	m_UVFactors.UnmapAndClear();

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

	for (u32 iView = 0; iView < m_ViewDependents.Count(); ++iView)
		m_ViewDependents[iView].ClearBuffers();
}

//----------------------------------------------------------------------------

void	CRenderBatchPolicy::_ClearStreamOffsets()
{
	for (u32 iStream = 0; iStream < __SupportedAdditionalStreamCount; ++iStream)
		m_AdditionalStreamOffsets[iStream].Reset();
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::AreBillboardingBatchable(const PopcornFX::PCRendererCacheBase &firstMat, const PopcornFX::PCRendererCacheBase &secondMat) const
{
	// Nothing special to do
	const CRendererCache	*firstMatCache = static_cast<const CRendererCache*>(firstMat.Get());
	const CRendererCache	*secondMatCache = static_cast<const CRendererCache*>(secondMat.Get());

	if (firstMatCache == null || secondMatCache == null)
		return false;

	const CMaterialDesc_GameThread		&gDesc0 = firstMatCache->GameThread_Desc();
	const CMaterialDesc_GameThread		&gDesc1 = secondMatCache->GameThread_Desc();
	if (gDesc0.m_RendererClass == gDesc1.m_RendererClass)
	{
		if (gDesc0.m_RendererClass == PopcornFX::ERendererClass::Renderer_Light)
			return true; // Single batch for all lights

		// we MUST return false,
		// because we only use SelfID, which is uniq only inside ParticleMediums
		// @FIXME: use SelfID+"MediumID?" to make AreRenderersCompatible return rendererA->CompatibleWith(rendererB))
		// @FIXME2: Unless batches are cleared each frame, this will cause issues
		if (gDesc0.m_RendererClass == PopcornFX::ERendererClass::Renderer_Sound)
			return false;
	}

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

bool	CRenderBatchPolicy::EmitDrawCall(const SRenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc, SCollectedDrawCalls &output)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::EmitDrawCall");
	PK_ASSERT(desc.m_TotalParticleCount <= m_TotalParticleCount); // <= if slicing is enabled
	PK_ASSERT(desc.m_TotalIndexCount <= m_TotalIndexCount);
	PK_ASSERT(desc.m_TotalVertexCount <= m_TotalIndexCount);
	PK_ASSERT(m_RendererType == desc.m_Renderer);
	PK_ASSERT(desc.m_DrawRequests.First() != null);

	// !Resolve material proxy and interface for first compatible material
	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return true;

	if (m_RendererType != PopcornFX::ERendererClass::Renderer_Light &&
		m_RendererType != PopcornFX::ERendererClass::Renderer_Sound) // don't try to resolve and validate material for lights and sounds
	{
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
	}

	switch (m_RendererType)
	{
	case	PopcornFX::ERendererClass::Renderer_Billboard:
		_IssueDrawCall_Billboard(renderContext, desc);
		break;
	case	PopcornFX::ERendererClass::Renderer_Mesh:
#if RHI_RAYTRACING
		if (renderContext.m_RendererSubView->RenderPass() == CRendererSubView::RenderPass_RT_AccelStructs)
			_IssueDrawCall_Mesh_AccelStructs(renderContext, desc);
		else
#endif // RHI_RAYTRACING
			_IssueDrawCall_Mesh(renderContext, desc);
		break;
	case	PopcornFX::ERendererClass::Renderer_Ribbon:
		_IssueDrawCall_Ribbon(renderContext, desc);
		break;
	case	PopcornFX::ERendererClass::Renderer_Triangle:
		_IssueDrawCall_Triangle(renderContext, desc);
		break;
	case	PopcornFX::ERendererClass::Renderer_Light:
		_IssueDrawCall_Light(renderContext, desc, output);
		break;
	case	PopcornFX::ERendererClass::Renderer_Sound:
		_IssueDrawCall_Sound(renderContext, desc);
		break;
	default:
		PK_ASSERT_NOT_REACHED();
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------
//
// Billboards (CPU)
//
//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::MapBuffers(const SRenderContext &renderContext, const TMemoryView<SBBView> &views, PopcornFX::SBillboardBatchJobs *billboardBatch, const PopcornFX::SGeneratedInputs &toMap)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::MapBuffers_Billboard");

	if (billboardBatch == null)
	{
		// GPU or Light particles
		return true;
	}

	// All quads are billboarded first, then capsules. We need this info in the VS to compute the particle ID from the vertex ID
	PK_ASSERT(billboardBatch->m_Owner != null);
	m_CapsulesOffset = billboardBatch->m_Owner->VPP4_ParticleCount();

	const u32	totalIndexCount = m_TotalIndexCount;
	const u32	totalVertexCount = m_TotalVertexCount;
	const u32	totalParticleCount = m_TotalParticleCount;

	// View independent
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Indices)
	{
		PK_ASSERT(m_Indices.Valid());
		void	*indices = null;
		bool	largeIndices = false;

		if (!m_Indices->Map(indices, largeIndices, totalIndexCount))
			return false;
		if (!billboardBatch->m_Exec_Indices.m_IndexStream.Setup(indices, totalIndexCount, largeIndices))
			return false;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Position)
	{
		PK_ASSERT(m_Positions.Valid());
		TStridedMemoryView<CFloat3, 0x10>	positions(null, totalVertexCount, 0x10);
		if (!m_Positions->Map(positions))
			return false;
		billboardBatch->m_Exec_PNT.m_Positions = positions;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Normal)
	{
		PK_ASSERT(m_Normals.Valid());
		TStridedMemoryView<CFloat3, 0x10>	normals(null, totalVertexCount, 0x10);
		if (!m_Normals->Map(normals))
			return false;
		billboardBatch->m_Exec_PNT.m_Normals = normals;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Tangent)
	{
		PK_ASSERT(m_Tangents.Valid());
		TStridedMemoryView<CFloat4, 0x10>	tangents(null, totalVertexCount, 0x10);
		if (!m_Tangents->Map(tangents))
			return false;
		billboardBatch->m_Exec_PNT.m_Tangents = tangents;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UV0)
	{
		PK_ASSERT(m_Texcoords.Valid());
		TStridedMemoryView<CFloat2>	uv0s(null, totalVertexCount);
		if (!m_Texcoords->Map(uv0s))
			return false;
		billboardBatch->m_Exec_Texcoords.m_Texcoords = uv0s;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UV1)
	{
		PK_ASSERT(m_Texcoords.Valid());
		PK_ASSERT(m_Texcoord2s.Valid());
		TStridedMemoryView<CFloat2>	uv1s(null, totalVertexCount);
		if (!m_Texcoord2s->Map(uv1s))
			return false;
		billboardBatch->m_Exec_Texcoords.m_Texcoords2 = uv1s;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_AtlasId)
	{
		PK_ASSERT(m_AtlasIDs.Valid());
		TStridedMemoryView<float>	atlasIDs(null, totalVertexCount);
		if (!m_AtlasIDs->Map(atlasIDs))
			return false;
		billboardBatch->m_Exec_Texcoords.m_AtlasIds = atlasIDs.ToMemoryViewIFP();
	}
	if (!toMap.m_AdditionalGeneratedInputs.Empty())
	{
		PK_ASSERT(m_SimData.Valid());
		TMemoryView<float>	simData;
		{
			PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::MapBuffers_Billboard (particle data buffer)");
			const u32	elementCount = m_SimDataBufferSizeInBytes / sizeof(float);
			if (!m_SimData->Map(simData, elementCount))
				return false;
		}
		float	*_data = simData.Data();

		// Additional inputs
		const u32	aFieldCount = m_AdditionalInputs.Count();

		if (!PK_VERIFY(m_MappedAdditionalInputs.Resize(aFieldCount)))
			return false;
		for (u32 iField = 0; iField < aFieldCount; ++iField)
		{
			SAdditionalInput					&field = m_AdditionalInputs[iField];
			PopcornFX::Drawers::SCopyFieldDesc	&desc = m_MappedAdditionalInputs[iField];

			desc.m_Storage.m_Count = totalParticleCount;
			desc.m_Storage.m_RawDataPtr = reinterpret_cast<u8*>(PopcornFX::Mem::AdvanceRawPointer(_data, field.m_BufferOffset));
			desc.m_Storage.m_Stride = field.m_ByteSize;
			desc.m_AdditionalInputIndex = field.m_AdditionalInputIndex;
		}

		billboardBatch->m_Exec_CopyField.m_FieldsToCopy = m_MappedAdditionalInputs.View();
		billboardBatch->m_Exec_CopyField.m_PerVertex = false;
	}

	// View deps
	const u32	activeViewCount = m_ViewDependents.Count();
	PK_ASSERT(activeViewCount == billboardBatch->m_PerView.Count());
	for (u32 iView = 0; iView < activeViewCount; ++iView)
	{
		const u32									viewGeneratedInputs = toMap.m_PerViewGeneratedInputs[iView].m_GeneratedInputs;
		SViewDependent								&viewDep = m_ViewDependents[iView];
		PopcornFX::SBillboardBatchJobs::SPerView	&dstView = billboardBatch->m_PerView[iView];

		if (viewGeneratedInputs & PopcornFX::Drawers::GenInput_Indices)
		{
			PK_ASSERT(viewDep.m_Indices.Valid());
			void	*indices = null;
			bool	largeIndices = false;

			if (!viewDep.m_Indices->Map(indices, largeIndices, totalIndexCount))
				return false;
			if (!dstView.m_Exec_Indices.m_IndexStream.Setup(indices, totalIndexCount, largeIndices))
				return false;
		}
		if (viewGeneratedInputs & PopcornFX::Drawers::GenInput_Position)
		{
			PK_ASSERT(viewDep.m_Positions.Valid());
			TStridedMemoryView<CFloat3, 0x10>	positions(null, totalVertexCount, 0x10);
			if (!viewDep.m_Positions->Map(positions))
				return false;
			dstView.m_Exec_PNT.m_Positions = positions;
		}
		if (viewGeneratedInputs & PopcornFX::Drawers::GenInput_Normal)
		{
			PK_ASSERT(viewDep.m_Normals.Valid());
			TStridedMemoryView<CFloat3, 0x10>	normals(null, totalVertexCount, 0x10);
			if (!viewDep.m_Normals->Map(normals))
				return false;
			dstView.m_Exec_PNT.m_Normals = normals;
		}
		if (viewGeneratedInputs & PopcornFX::Drawers::GenInput_Tangent)
		{
			PK_ASSERT(viewDep.m_Tangents.Valid());
			TStridedMemoryView<CFloat4, 0x10>	tangents(null, totalVertexCount, 0x10);
			if (!viewDep.m_Tangents->Map(tangents))
				return false;
			dstView.m_Exec_PNT.m_Tangents = tangents;
		}
	}
	return true;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::LaunchCustomTasks(const SRenderContext &renderContext, const TMemoryView<const PopcornFX::Drawers::SBillboard_DrawRequest * const> &drawRequests, PopcornFX::Drawers::CBillboard_CPU *billboardBatch)
{
	if (billboardBatch == null)
	{
		PK_ASSERT_NOT_REACHED(); // GPU
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

class FPopcornFXBillboardCollector : public FOneFrameResource
{
public:
	FPopcornFXVertexFactory		*m_VertexFactory = null;
	CPooledIndexBuffer			m_IndexBufferForRefCount;

	FPopcornFXBillboardCollector() { }
	~FPopcornFXBillboardCollector()
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

void	CRenderBatchPolicy::_IssueDrawCall_Billboard(const SRenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Billboard (CPU)");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);

	FMeshElementCollector		*collector = view->Collector();
	const FPopcornFXSceneProxy	*sceneProxy = view->SceneProxy();
	PK_ASSERT(collector != null);
	PK_ASSERT(sceneProxy != null);

	const u32	indexCount = desc.m_TotalIndexCount;
	const u32	indexOffset = desc.m_IndexOffset;

	PK_ASSERT(indexOffset + indexCount <= m_TotalIndexCount);

	const bool	reverseCulling = sceneProxy->IsLocalToWorldDeterminantNegative();
	const bool	isSelected = sceneProxy->IsSelected();
	const bool	drawsVelocity = sceneProxy->DrawsVelocity();

	const FBoxSphereBounds	&bounds = sceneProxy->GetBounds();
	const FBoxSphereBounds	&localBounds = sceneProxy->GetLocalBounds();

	bool	hasPrecomputedVolumetricLightmap;
	FMatrix	previousLocalToWorld;
	int32	singleCaptureIndex;

	bool	outputVelocity;
	sceneProxy->GetScene().GetPrimitiveUniformShaderParameters_RenderThread(sceneProxy->GetPrimitiveSceneInfo(), hasPrecomputedVolumetricLightmap, previousLocalToWorld, singleCaptureIndex, outputVelocity);

	FMatrix	localToWorld = FMatrix::Identity;
	if (view->GlobalScale() != 1.0f)
		localToWorld *= view->GlobalScale();

	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return;
	CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();
	PK_ASSERT(matDesc.ValidForRendering());

	PK_ASSERT(m_RealViewCount >= m_ViewDependents.Count());
	PK_ASSERT(desc.m_ViewIndex < m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() == m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() <= renderContext.m_RendererSubView->SceneViews().Count());
	for (u32 iView = 0; iView < m_RealViewCount; ++iView)
	{
		if (desc.m_ViewIndex != iView)
			continue;

		const u32	realViewIndex = renderContext.m_RendererSubView->BBViews()[iView].m_ViewIndex;

		// Try and resolve from active views
		SViewDependent	*viewDep = null;
		for (u32 iViewDep = 0; iViewDep < m_ViewDependents.Count(); ++iViewDep)
		{
			if (m_ViewDependents[iViewDep].m_ViewIndex == iView)
			{
				viewDep = &m_ViewDependents[iViewDep];
				break;
			}
		}

		const bool	fullViewIndependent = viewDep == null;
		const bool	viewIndependentIndices = (fullViewIndependent && m_Indices.Valid()) || (!fullViewIndependent && !viewDep->m_Indices.Valid());
		const bool	viewIndependentPNT = (fullViewIndependent && m_Positions.Valid()) || (!fullViewIndependent && !viewDep->m_Positions.Valid());

		// This should never happen
		if (!viewIndependentIndices && (viewDep == null || !viewDep->m_Indices.Valid()))
			return;

		// Assert cannot have viewdep normals/tangents and indep pos + vice versa

		FPopcornFXVertexFactory			*vertexFactory = null;

		PK_ASSERT(m_Indices.Valid() || (viewDep != null && viewDep->m_Indices.Valid()));
		{
			PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Billboard CollectorRes");

			FPopcornFXBillboardCollector	*collectorRes = &(collector->AllocateOneFrameResource<FPopcornFXBillboardCollector>());
			collectorRes->m_IndexBufferForRefCount = viewIndependentIndices ? m_Indices : viewDep->m_Indices;

			PK_ASSERT(collectorRes->m_VertexFactory == null);
			collectorRes->m_VertexFactory = new FPopcornFXVertexFactory(m_FeatureLevel);

			// !! if the vertexFactory is CACHED:
			// be carefull streams could change Strides and/or formats on the fly !
			vertexFactory = collectorRes->m_VertexFactory;
			vertexFactory->m_Texcoords.Setup(m_Texcoords);
			vertexFactory->m_Texcoord2s.Setup(m_Texcoord2s);
			vertexFactory->m_AtlasIDs.Setup(m_AtlasIDs);

			vertexFactory->m_Positions.Setup(viewIndependentPNT ? m_Positions : viewDep->m_Positions);
			vertexFactory->m_Normals.Setup(viewIndependentPNT ? m_Normals : viewDep->m_Normals);
			vertexFactory->m_Tangents.Setup(viewIndependentPNT ? m_Tangents : viewDep->m_Tangents);


			FPopcornFXBillboardVSUniforms		vsUniforms;
			FPopcornFXBillboardCommonUniforms	commonUniforms;

			vsUniforms.SimData = m_SimData.Buffer()->SRV();
			vsUniforms.RendererType = static_cast<u32>(m_RendererType);
			vsUniforms.CapsulesOffset = m_CapsulesOffset;
			vsUniforms.InColorsOffset = m_AdditionalStreamOffsets[StreamOffset_Colors].OffsetForShaderConstant();
			vsUniforms.InEmissiveColorsOffset = m_AdditionalStreamOffsets[StreamOffset_EmissiveColors].OffsetForShaderConstant();
			vsUniforms.InAlphaCursorsOffset = m_AdditionalStreamOffsets[StreamOffset_AlphaCursors].OffsetForShaderConstant();
			vsUniforms.InDynamicParameter1sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam1s].OffsetForShaderConstant();
			vsUniforms.InDynamicParameter2sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam2s].OffsetForShaderConstant();
			vsUniforms.InDynamicParameter3sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam3s].OffsetForShaderConstant();

			commonUniforms.HasSecondUVSet = m_SecondUVSet;
			commonUniforms.FlipUVs = m_FlipUVs;
			commonUniforms.NeedsBTN = m_NeedsBTN;
			commonUniforms.CorrectRibbonDeformation = m_CorrectRibbonDeformation;

			vertexFactory->m_VSUniformBuffer = FPopcornFXBillboardVSUniformsRef::CreateUniformBufferImmediate(vsUniforms, UniformBuffer_SingleDraw);
			vertexFactory->m_CommonUniformBuffer = FPopcornFXBillboardCommonUniformsRef::CreateUniformBufferImmediate(commonUniforms, UniformBuffer_SingleDraw);

			PK_ASSERT(!vertexFactory->IsInitialized());
			vertexFactory->InitResource();
		}

		if (!PK_VERIFY(vertexFactory->IsInitialized()))
			return;

		FMeshBatch		&meshBatch = collector->AllocateMesh();

		meshBatch.VertexFactory = vertexFactory;
		meshBatch.CastShadow = matDesc.m_CastShadows;
		meshBatch.bUseAsOccluder = false;
		meshBatch.ReverseCulling = reverseCulling;
		meshBatch.Type = PT_TriangleList;
		meshBatch.DepthPriorityGroup = sceneProxy->GetDepthPriorityGroup(view->SceneViews()[realViewIndex].m_SceneView);
		meshBatch.bUseWireframeSelectionColoring = isSelected;
		meshBatch.bCanApplyViewModeOverrides = true;
		meshBatch.MaterialRenderProxy = matDesc.RenderProxy();

		FMeshBatchElement	&meshElement = meshBatch.Elements[0];

		meshElement.IndexBuffer = viewIndependentIndices ? m_Indices.Buffer() : viewDep->m_Indices.Buffer();
		meshElement.FirstIndex = indexOffset;
		meshElement.NumPrimitives = indexCount / 3;
		meshElement.MinVertexIndex = 0;
		meshElement.MaxVertexIndex = m_TotalVertexCount - 1;

		FDynamicPrimitiveUniformBuffer	&dynamicPrimitiveUniformBuffer = collector->AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
		dynamicPrimitiveUniformBuffer.Set(localToWorld, previousLocalToWorld, bounds, localBounds, true, hasPrecomputedVolumetricLightmap, drawsVelocity, outputVelocity);
		meshElement.PrimitiveUniformBuffer = dynamicPrimitiveUniformBuffer.UniformBuffer.GetUniformBufferRHI();

		PK_ASSERT(meshElement.NumPrimitives > 0);

		{
			INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsCount, 1);

			collector->AddMesh(realViewIndex, meshBatch);
		}
	}
}

//----------------------------------------------------------------------------
//
// Billboards (GPU)
//
//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::MapBuffers(const SRenderContext &renderContext, const TMemoryView<SBBView> &views, PopcornFX::SGPUBillboardBatchJobs *geomBBBatch, const PopcornFX::SGeneratedInputs &toMap)
{
	if (geomBBBatch != null)
	{
		PK_ASSERT_NOT_REACHED();
		return false;
	}
	// No CPU tasks, nothing to map
	return true;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::LaunchCustomTasks(const SRenderContext &renderContext, const TMemoryView<const PopcornFX::Drawers::SBillboard_DrawRequest * const> &drawRequests, PopcornFX::Drawers::CCopyStream_CPU *billboardBatch)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy:LaunchCustom Jobs (Billboards)");
	if (billboardBatch != null)
	{
		PK_ASSERT_NOT_REACHED();
		return false;
	}
	PK_ASSERT(!drawRequests.Empty());
	PK_ASSERT(drawRequests.First() != null);
	PK_ASSERT(drawRequests.First()->GPUStorage());

	const PopcornFX::Drawers::SBillboard_BillboardingRequest	&compatBr = drawRequests.First()->m_BB;

#if (PK_HAS_GPU != 0)
	FRHICommandListImmediate	&RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

	// PK_TODO("hot reload atlas");
	if (compatBr.m_Atlas != null)
	{
		//PK_FIXME("Atlas access should be Update thread !");
		if (!PK_VERIFY(m_AtlasRects.LoadRects(renderContext, compatBr.m_Atlas->m_RectsFp32)))
			return false;
		PK_ASSERT(m_AtlasRects.Loaded());
	}

	using namespace		PopcornFXBillboarder;

	const ERHIFeatureLevel::Type					featureLevel = m_FeatureLevel;
	TShaderMapRef< FBillboarderBillboardCS_Std >	billboarderCS(GetGlobalShaderMap(featureLevel));
	TShaderMapRef< FCopyStreamCS >					copyStreamCS(GetGlobalShaderMap(featureLevel));

	CRenderBatchManager	*rbManager = renderContext.m_RenderBatchManager;
	PK_ASSERT(rbManager != null);

	// TODO : Handle sorted indices per views
	const bool	validIndices = m_Indices.Valid() || m_ViewDependents[0].m_Indices.Valid();
	bool		sortSupported = false; // Disabled for now
#if (PK_GPU_D3D12 == 1)
	if (renderContext.m_API == SRenderContext::D3D12)
		sortSupported = false;
#endif // (PK_GPU_D3D12 == 1)

	// Force disable sort for now
	bool		_needSort = validIndices && drawRequests.First()->m_BB.m_Flags.m_NeedSort && sortSupported;
#if (PK_HAS_GPU == 1)
	if (_needSort && !m_Sorter.Prepare(rbManager->VBPool_GPU(), m_TotalParticleCount))
#endif // (PK_HAS_GPU == 1)
		_needSort = false;
	const bool	needSort = _needSort;

	u32	nextParticlesOffset = 0;
	u32	nextVerticesOffset = 0;
	u32	nextIndicesOffset = 0;
	if (needSort)
	{
		SCOPED_GPU_STAT(RHICmdList, PopcornFXBillboardSort);

		PK_ASSERT(m_Sorter.Ready());

		FPopcornFXSortComputeShader_GenKeys_Params	genKeysParams;
		PK_ASSERT(m_RealViewCount > 0);
		genKeysParams.m_SortOrigin = m_ViewDependents[0].m_BBMatrix.WAxis();

		const u32	drCount = drawRequests.Count();
		for (u32 iDr = 0; iDr < drCount; ++iDr)
		{
			const PopcornFX::Drawers::SBillboard_DrawRequest			&dr = *drawRequests[iDr];
			const PopcornFX::Drawers::SBillboard_BillboardingRequest	&br = dr.m_BB;

			const u32	pCount = dr.InputParticleCount();
			PK_ASSERT(pCount > 0);

			const u32	vpp = PopcornFX::Drawers::BBModeVPP(br.m_Mode);
			const u32	ipp = PopcornFX::Drawers::BBModeIPP(br.m_Mode);
			PK_ASSERT(vpp > 0);

			const u32	bbType = PopcornFXBillboarder::BillboarderModeToType(br.m_Mode);
			if (!PK_VERIFY(ipp > 0) ||
				!PK_VERIFY(bbType < EBillboarder::_Count))
				continue;

			const u32	verticesOffset = nextVerticesOffset;
			const u32	indicesOffset = nextIndicesOffset;
			const u32	particlesOffset = nextParticlesOffset;
			nextVerticesOffset += pCount * vpp;
			nextIndicesOffset += pCount * ipp;
			nextParticlesOffset += pCount;

			PK_ASSERT(dr.GPUStorage());
			const PopcornFX::CParticleStreamToRender	&stream = dr.StreamToRender();

			PK_ASSERT(br.m_PositionStreamId.Valid());
			PopcornFX::CGuid				posOffset = PopcornFX::CGuid::INVALID;
			const FShaderResourceViewRHIRef	inPositions = StreamBufferSRVToRHI<CFloat3, 12>(stream, br.m_PositionStreamId, posOffset);

			// @TODO: does each bbView needs it's own sorted indices ????

			if (PK_VERIFY(posOffset.Valid()))
			{
				genKeysParams.m_Count = pCount; // will fill the rest with infinity
				genKeysParams.m_IndexStart = verticesOffset;
				genKeysParams.m_IndexStep = vpp;
				genKeysParams.m_InputOffset = 0; // FUTURE stream pages
				genKeysParams.m_InputPositions = inPositions;
				genKeysParams.m_InputPositionsOffset = posOffset;

				m_Sorter.DispatchGenIndiceBatch(RHICmdList, genKeysParams);
			}
		}

		m_Sorter.DispatchSort(RHICmdList);

		PK_ASSERT(nextVerticesOffset == m_TotalVertexCount);
		PK_ASSERT(nextIndicesOffset == m_TotalIndexCount);
		PK_ASSERT(nextParticlesOffset == m_TotalParticleCount);
		nextParticlesOffset = 0;
		nextVerticesOffset = 0;
		nextIndicesOffset = 0;
	}

	FBillboarderBillboardCS_Params	_bbParams;
	FBillboarderBillboardCS_Params	*bbParams = &_bbParams;
	FBillboardCS_Params				_copyParams;
	FBillboardCS_Params				*copyParams = &_copyParams;

#if (PK_GPU_D3D11 != 0) || (PK_GPU_D3D12 != 0)
	if (!needSort)
	{
		TShaderMapRef< FUAVsClearCS >	UAVsClearCS(GetGlobalShaderMap(featureLevel));

		// Index buffer clear

		FClearCS_Params					_clearParams;
		FClearCS_Params					*clearParams = &_clearParams;

		SCOPED_DRAW_EVENT(RHICmdList, PopcornFXClearUAVs);

		clearParams->SetUAV(EOutput::OutIndices, m_Indices);

#if (ENGINE_MINOR_VERSION >= 25)
		RHICmdList.SetComputeShader(UAVsClearCS.GetComputeShader());
#endif //(ENGINE_MINOR_VERSION >= 25)

		UAVsClearCS->Dispatch(renderContext, RHICmdList, *clearParams);
		clearParams->SetUAV(EOutput::OutIndices, null);

		PK_ASSERT(m_RealViewCount >= m_ViewDependents.Count());
		for (u32 iView = 0; iView < m_RealViewCount; ++iView)
		{
			// Try and resolve from active views
			SViewDependent	*viewDep = null;
			for (u32 iViewDep = 0; iViewDep < m_ViewDependents.Count(); ++iViewDep)
			{
				if (m_ViewDependents[iViewDep].m_ViewIndex == iView)
				{
					viewDep = &m_ViewDependents[iViewDep];
					break;
				}
			}
			const bool	fullViewIndependent = viewDep == null;
			const bool	viewIndependentIndices = (fullViewIndependent && m_Indices.Valid()) || (!fullViewIndependent && !viewDep->m_Indices.Valid());
			if (viewIndependentIndices)
				continue;

			PK_ASSERT(viewDep != null && viewDep->m_Indices.Valid());
			clearParams->SetUAV(EOutput::OutIndices, viewDep->m_Indices);
			UAVsClearCS->Dispatch(renderContext, RHICmdList, *clearParams);
			clearParams->SetUAV(EOutput::OutIndices, null);
		}
	}
#endif // (PK_GPU_D3D11 != 0) || (PK_GPU_D3D12 != 0)

	bbParams->m_RendererAtlasRectCount = m_AtlasRects.m_AtlasRectsCount;
	bbParams->m_RendererAtlasBuffer = m_AtlasRects.m_AtlasBufferSRV;

	{
		SCOPED_DRAW_EVENT(RHICmdList, PopcornFXBillboarder_Billboard);

		SCOPED_GPU_STAT(RHICmdList, PopcornFXBillboardBillboarding);

		const u32	baseParticlesOffset = 0;
		const u32	baseVerticesOffset = 0;
		const u32	baseIndicesOffset = 0;

		const u32	drCount = drawRequests.Count();
		for (u32 iDr = 0; iDr < drCount; ++iDr)
		{
			const PopcornFX::Drawers::SBillboard_DrawRequest			&dr = *drawRequests[iDr];
			const PopcornFX::Drawers::SBillboard_BillboardingRequest	&br = dr.m_BB;
			PK_ASSERT(dr.Valid());

			const u32	pCount = dr.InputParticleCount();
			PK_ASSERT(pCount > 0);

			const u32	vpp = PopcornFX::Drawers::BBModeVPP(br.m_Mode);
			const u32	ipp = PopcornFX::Drawers::BBModeIPP(br.m_Mode);
			PK_ASSERT(vpp > 0);

			const u32	bbType = PopcornFXBillboarder::BillboarderModeToType(br.m_Mode);
			if (!PK_VERIFY(ipp > 0) ||
				!PK_VERIFY(bbType < EBillboarder::_Count))
				continue;

			const u32	verticesOffset = nextVerticesOffset;
			const u32	indicesOffset = nextIndicesOffset;
			const u32	particlesOffset = nextParticlesOffset;
			nextVerticesOffset += pCount * vpp;
			nextIndicesOffset += pCount * ipp;
			nextParticlesOffset += pCount;

			const PopcornFX::CParticleStreamToRender_GPU	*stream = dr.StreamToRender_GPU();
			if (!PK_VERIFY(stream != null))
				return false;

			PK_ASSERT(br.m_PositionStreamId.Valid());

			// Inputs (uniforms)
			bbParams->m_BillboarderType = bbType;
			bbParams->m_InIndicesOffset = baseParticlesOffset + particlesOffset;
			bbParams->m_InputOffset = 0; // FUTURE stream pages
			bbParams->m_OutputVertexOffset = baseVerticesOffset + verticesOffset;
			bbParams->m_OutputIndexOffset = baseIndicesOffset + indicesOffset;
			bbParams->m_ParticleCount = pCount;
#if (PK_GPU_D3D11 == 1)
			if (renderContext.m_API == SRenderContext::D3D11)
			{
				const PopcornFX::CParticleStreamToRender_D3D11	&stream_D3D11 = static_cast<const PopcornFX::CParticleStreamToRender_D3D11&>(*stream);

				bbParams->m_LiveParticleCount = StreamBufferSRVToRHI(&stream_D3D11.StreamSizeBuf(), sizeof(u32) * 4, sizeof(u32));
				copyParams->m_LiveParticleCount = StreamBufferSRVToRHI(&stream_D3D11.StreamSizeBuf(), sizeof(u32) * 4, sizeof(u32));
				bbParams->m_InSimData = StreamBufferSRVToRHI(&stream_D3D11.StreamBuffer(), stream_D3D11.StreamSizeEst(), sizeof(float));
				copyParams->m_InSimData = StreamBufferSRVToRHI(&stream_D3D11.StreamBuffer(), stream_D3D11.StreamSizeEst(), sizeof(float));
				bbParams->SetOutput(EOutput::OutIndices, m_Indices);
			}
#endif // (PK_GPU_D3D11 == 1)
#if (PK_GPU_D3D12 == 1)
			if (renderContext.m_API == SRenderContext::D3D12)
			{
				const PopcornFX::CParticleStreamToRender_D3D12	&stream_D3D12 = static_cast<const PopcornFX::CParticleStreamToRender_D3D12&>(*stream);

				bbParams->m_LiveParticleCount = StreamBufferSRVToRHI(&stream_D3D12.StreamSizeBuf(), sizeof(u32));
				copyParams->m_LiveParticleCount = StreamBufferSRVToRHI(&stream_D3D12.StreamSizeBuf(), sizeof(u32));

				const PopcornFX::SParticleStreamBuffer_D3D12	&simData = stream_D3D12.StreamBuffer();
				bbParams->m_InSimData = StreamBufferSRVToRHI(&simData, 16);
				copyParams->m_InSimData = StreamBufferSRVToRHI(&simData, 16);
				bbParams->SetOutput(EOutput::OutIndicesRaw, m_Indices);
			}
#endif // (PK_GPU_D3D12 == 1)

			copyParams->m_BillboarderType = bbParams->m_BillboarderType;
			copyParams->m_ParticleCount = pCount;
			copyParams->m_InputOffset = bbParams->m_InputOffset;
			copyParams->m_OutputVertexOffset = bbParams->m_OutputVertexOffset;

			bbParams->m_RendererFlags = 0;
			bbParams->m_RendererFlags |= ((br.m_Flags.m_FlipU && br.m_Flags.m_FlipV) ? (1 << ERendererFlag::FlipV) : 0);
			bbParams->m_RendererFlags |= (br.m_Flags.m_HasAtlasBlending ? (1 << ERendererFlag::SoftAnimationBlending) : 0);
			bbParams->m_RendererNormalsBendingFactor = PopcornFX::PKMin(br.m_NormalsBendingFactor, 0.99f);	// don't generate fully flat normals, this will interpolate crappyly on the GPU

			// Outputs
			bbParams->SetOutput(EOutput::OutPositions, m_Positions);
			bbParams->SetOutput(EOutput::OutNormals, m_Normals);
			bbParams->SetOutput(EOutput::OutTangents, m_Tangents);
			bbParams->SetOutput(EOutput::OutTexcoords, m_Texcoords);
			bbParams->SetOutput(EOutput::OutTexcoord2s, m_Texcoord2s);
			bbParams->SetOutput(EOutput::OutAtlasIDs, m_AtlasIDs);
			if (m_AdditionalStreamOffsets[StreamOffset_Colors].Valid())
				copyParams->SetOutput(EOutput::OutColors, m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_Colors].InputId()].m_VertexBuffer);
			if (m_AdditionalStreamOffsets[StreamOffset_VATCursors].Valid())
				copyParams->SetOutput(EOutput::OutAlphaCursors, m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_VATCursors].InputId()].m_VertexBuffer);
			if (m_AdditionalStreamOffsets[StreamOffset_DynParam1s].Valid())
				copyParams->SetOutput(EOutput::OutDynamicParameter1s, m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_DynParam1s].InputId()].m_VertexBuffer);
			if (m_AdditionalStreamOffsets[StreamOffset_DynParam2s].Valid())
				copyParams->SetOutput(EOutput::OutDynamicParameter2s, m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_DynParam2s].InputId()].m_VertexBuffer);
			if (m_AdditionalStreamOffsets[StreamOffset_DynParam3s].Valid())
				copyParams->SetOutput(EOutput::OutDynamicParameter3s, m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_DynParam3s].InputId()].m_VertexBuffer);

			// Inputs
			PK_ASSERT(m_Sorter.Ready() == needSort);
			bbParams->m_InIndices = m_Sorter.Ready() ? m_Sorter.SortedValuesSRV() : null;
			bbParams->SetInput(EInput::InIndices, IsValidRef(bbParams->m_InIndices), 0);

			PK_ASSERT(br.m_PositionStreamId.Valid());
			bbParams->SetInputOffsetIFP<CFloat3, 12>(EInput::InPositions, *stream, br.m_PositionStreamId, CFloat4(0.f));
			bbParams->SetInputOffsetIFP<float, 4>(EInput::InRotations, *stream, br.m_RotationStreamId, CFloat4(0.f));
			bbParams->SetInputOffsetIFP<CFloat3, 12>(EInput::InAxis0s, *stream, br.m_Axis0StreamId, CFloat4(0, 0, 1, 0));
			bbParams->SetInputOffsetIFP<CFloat3, 12>(EInput::InAxis1s, *stream, br.m_Axis1StreamId, CFloat4(0, 0, 1, 0));
			bbParams->SetInputOffsetIFP<float, 4>(EInput::InTextureIds, *stream, br.m_TextureIdStreamId, CFloat4(0.f));
			if (m_AdditionalStreamOffsets[StreamOffset_Colors].Valid())
				copyParams->SetInputOffsetIFP<CFloat4, 16>(EInput::InColors, *stream, br.m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_Colors].InputId()].m_StreamId, CFloat4(1.0f));
			if (m_AdditionalStreamOffsets[StreamOffset_VATCursors].Valid())
				copyParams->SetInputOffsetIFP<float, 4>(EInput::InAlphaCursors, *stream, br.m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_VATCursors].InputId()].m_StreamId, CFloat4(0.0f));
			if (m_AdditionalStreamOffsets[StreamOffset_DynParam1s].Valid())
				copyParams->SetInputOffsetIFP<CFloat4, 16>(EInput::InDynamicParameter1s, *stream, br.m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_DynParam1s].InputId()].m_StreamId, CFloat4(0.0f));
			if (m_AdditionalStreamOffsets[StreamOffset_DynParam2s].Valid())
				copyParams->SetInputOffsetIFP<CFloat4, 16>(EInput::InDynamicParameter2s, *stream, br.m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_DynParam2s].InputId()].m_StreamId, CFloat4(0.0f));
			if (m_AdditionalStreamOffsets[StreamOffset_DynParam3s].Valid())
				copyParams->SetInputOffsetIFP<CFloat4, 16>(EInput::InDynamicParameter3s, *stream, br.m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_DynParam3s].InputId()].m_StreamId, CFloat4(0.0f));

			if (!br.m_SizeFloat2)
			{
				bbParams->SetInputOffsetIFP<float, 4>(EInput::InSizes, *stream, br.m_SizeStreamId, CFloat4(0.0f));
				bbParams->SetInput(EInput::In2Sizes, null, 0);
			}
			else
			{
				bbParams->SetInputOffsetIFP<CFloat2, 8>(EInput::In2Sizes, *stream, br.m_SizeStreamId, CFloat4(0.0f));
				bbParams->SetInput(EInput::InSizes, null, 0);
			}

			// TODO: Don't dispatch when not necessary: lots of views, geom view indenpendent
			// In this case only dispatch once as they will all write in the same vertex buffers
			bool	firstViewExecuted = false;
			PK_ASSERT(m_RealViewCount >= m_ViewDependents.Count());
			for (u32 iView = 0; iView < m_RealViewCount; ++iView)
			{
				// Try and resolve from active views
				SViewDependent	*viewDep = null;
				for (u32 iViewDep = 0; iViewDep < m_ViewDependents.Count(); ++iViewDep)
				{
					if (m_ViewDependents[iViewDep].m_ViewIndex == iView)
					{
						viewDep = &m_ViewDependents[iViewDep];
						break;
					}
				}
				const bool	fullViewIndependent = viewDep == null;
				const bool	viewIndependentIndices = (fullViewIndependent && m_Indices.Valid()) || (!fullViewIndependent && !viewDep->m_Indices.Valid());
				const bool	viewIndependentPNT = (fullViewIndependent && m_Positions.Valid()) || (!fullViewIndependent && !viewDep->m_Positions.Valid());

				PK_ASSERT(m_Indices.Valid() || (viewDep != null && viewDep->m_Indices.Valid()));
				if (!fullViewIndependent)
					bbParams->m_BillboardingMatrix = viewDep->m_BBMatrix;

				if (!viewIndependentPNT)
				{
					bbParams->SetOutput(EOutput::OutPositions, viewDep->m_Positions);
					bbParams->SetOutput(EOutput::OutNormals, viewDep->m_Normals);
					bbParams->SetOutput(EOutput::OutTangents, viewDep->m_Tangents);
				}
				if (!viewIndependentIndices)
				{
#if (PK_GPU_D3D11 == 1)
					if (renderContext.m_API == SRenderContext::D3D11)
						bbParams->SetOutput(EOutput::OutIndices, viewDep->m_Indices);
#endif // (PK_GPU_D3D11 == 1)
#if (PK_GPU_D3D12 == 1)
					if (renderContext.m_API == SRenderContext::D3D12)
						bbParams->SetOutput(EOutput::OutIndicesRaw, viewDep->m_Indices);
#endif // (PK_GPU_D3D12 == 1)
				}
#if (ENGINE_MINOR_VERSION >= 25)
				RHICmdList.SetComputeShader(billboarderCS.GetComputeShader());
#endif //(ENGINE_MINOR_VERSION >= 25)
				billboarderCS->Dispatch(RHICmdList, *bbParams);

				//bbParams->SetOutput(EOutput::OutPositions, null);
				//bbParams->SetOutput(EOutput::OutNormals, null);
				//bbParams->SetOutput(EOutput::OutTangents, null);

				// Unset common params: only do them once
				if (!firstViewExecuted)
				{
					bbParams->SetOutput(EOutput::OutIndicesRaw, null);
					bbParams->SetOutput(EOutput::OutIndices, null);
					bbParams->SetOutput(EOutput::OutColors, null);
					bbParams->SetOutput(EOutput::OutTexcoords, null);
					bbParams->SetOutput(EOutput::OutTexcoord2s, null);
					bbParams->SetOutput(EOutput::OutAtlasIDs, null);
					if (viewIndependentPNT)
					{
						bbParams->SetOutput(EOutput::OutPositions, null);
						bbParams->SetOutput(EOutput::OutNormals, null);
						bbParams->SetOutput(EOutput::OutTangents, null);
					}
					if (viewIndependentIndices)
					{
						bbParams->SetOutput(EOutput::OutIndicesRaw, null);
						bbParams->SetOutput(EOutput::OutIndices, null);
					}
					firstViewExecuted = true;
				}
			}

#if (ENGINE_MINOR_VERSION >= 25)
			RHICmdList.SetComputeShader(copyStreamCS.GetComputeShader());
#endif //(ENGINE_MINOR_VERSION >= 25)
			// Late: CopyStream - view independent
			copyStreamCS->Dispatch(RHICmdList, *copyParams);
		}

		PK_ASSERT(nextParticlesOffset == m_TotalParticleCount);
		PK_ASSERT(nextVerticesOffset == m_TotalVertexCount);
		PK_ASSERT(nextIndicesOffset == m_TotalIndexCount);
	}

#else
	PK_ASSERT_NOT_REACHED();
	return false;
#endif // (PK_HAS_GPU != 0)
	return true;
}

//----------------------------------------------------------------------------
//
// Ribbons
//
//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::MapBuffers(const SRenderContext &renderContext, const TMemoryView<SBBView> &views, PopcornFX::SRibbonBatchJobs *ribbonBatch, const PopcornFX::SGeneratedInputs &toMap)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::MapBuffers_Ribbons");

	if (ribbonBatch == null)
	{
		PK_ASSERT_NOT_REACHED(); // GPU
		return false;
	}

	const u32	totalIndexCount = m_TotalIndexCount;
	const u32	totalVertexCount = m_TotalVertexCount;
	const u32	totalParticleCount = m_TotalParticleCount;

	// View independent
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Indices)
	{
		PK_ASSERT(m_Indices.Valid());
		void	*indices = null;
		bool	largeIndices = false;

		if (!m_Indices->Map(indices, largeIndices, totalIndexCount))
			return false;
		if (!ribbonBatch->m_Exec_Indices.m_IndexStream.Setup(indices, totalIndexCount, largeIndices))
			return false;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Position)
	{
		PK_ASSERT(m_Positions.Valid());
		TStridedMemoryView<CFloat3, 0x10>	positions(null, totalVertexCount, 0x10);
		if (!m_Positions->Map(positions))
			return false;
		ribbonBatch->m_Exec_PNT.m_Positions = positions;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Normal)
	{
		PK_ASSERT(m_Normals.Valid());
		TStridedMemoryView<CFloat3, 0x10>	normals(null, totalVertexCount, 0x10);
		if (!m_Normals->Map(normals))
			return false;
		ribbonBatch->m_Exec_PNT.m_Normals = normals;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Tangent)
	{
		PK_ASSERT(m_Tangents.Valid());
		TStridedMemoryView<CFloat4, 0x10>	tangents(null, totalVertexCount, 0x10);
		if (!m_Tangents->Map(tangents))
			return false;
		ribbonBatch->m_Exec_PNT.m_Tangents = tangents;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UV0)
	{
		PK_ASSERT(m_Texcoords.Valid());
		TStridedMemoryView<CFloat2>	uv0s(null, totalVertexCount);
		if (!m_Texcoords->Map(uv0s))
			return false;
		ribbonBatch->m_Exec_Texcoords.m_Texcoords = uv0s;
	}
	bool	hasUVFactors = false;
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UVRemap)
	{
		PK_ASSERT(m_UVRemaps.Valid());
		TStridedMemoryView<CFloat4>	uvRemap(null, totalVertexCount);
		if (!m_UVRemaps->Map(uvRemap))
			return false;
		ribbonBatch->m_Exec_UVRemap.m_UVRemap = uvRemap;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UVFactors)
	{
		PK_ASSERT(m_UVFactors.Valid());
		TStridedMemoryView<CFloat4>	uvFactors(null, totalVertexCount);
		if (!m_UVFactors->Map(uvFactors))
			return false;
		hasUVFactors = true;
		ribbonBatch->m_Exec_PNT.m_UVFactors4 = uvFactors;
	}
	PK_ASSERT(!m_Texcoord2s.Valid() && !m_AtlasIDs.Valid());

	if (!toMap.m_AdditionalGeneratedInputs.Empty())
	{
		PK_ASSERT(m_SimData.Valid());
		TMemoryView<float>	simData;
		{
			PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::MapBuffers_Ribbon (particle data buffer)");
			const u32	elementCount = m_SimDataBufferSizeInBytes / sizeof(float);
			if (!m_SimData->Map(simData, elementCount))
				return false;
		}
		float	*_data = simData.Data();

		// Additional inputs
		const u32	aFieldCount = m_AdditionalInputs.Count();

		if (!PK_VERIFY(m_MappedAdditionalInputs.Resize(aFieldCount)))
			return false;
		for (u32 iField = 0; iField < aFieldCount; ++iField)
		{
			SAdditionalInput					&field = m_AdditionalInputs[iField];
			PopcornFX::Drawers::SCopyFieldDesc	&desc = m_MappedAdditionalInputs[iField];

			desc.m_Storage.m_Count = totalParticleCount;
			desc.m_Storage.m_RawDataPtr = reinterpret_cast<u8*>(PopcornFX::Mem::AdvanceRawPointer(_data, field.m_BufferOffset));
			desc.m_Storage.m_Stride = field.m_ByteSize;
			desc.m_AdditionalInputIndex = field.m_AdditionalInputIndex;
		}

		ribbonBatch->m_Exec_CopyField.m_FieldsToCopy = m_MappedAdditionalInputs.View();
		ribbonBatch->m_Exec_CopyField.m_PerVertex = false;
	}

	// View deps
	const u32	activeViewCount = m_ViewDependents.Count();
	PK_ASSERT(activeViewCount == ribbonBatch->m_PerView.Count());
	for (u32 iView = 0; iView < activeViewCount; ++iView)
	{
		const u32								viewGeneratedInputs = toMap.m_PerViewGeneratedInputs[iView].m_GeneratedInputs;
		SViewDependent								&viewDep = m_ViewDependents[iView];
		PopcornFX::SRibbonBatchJobs::SPerView	&dstView = ribbonBatch->m_PerView[iView];

		if (viewGeneratedInputs & PopcornFX::Drawers::GenInput_Indices)
		{
			PK_ASSERT(viewDep.m_Indices.Valid());
			void	*indices = null;
			bool	largeIndices = false;

			if (!viewDep.m_Indices->Map(indices, largeIndices, totalIndexCount))
				return false;
			if (!dstView.m_Exec_Indices.m_IndexStream.Setup(indices, totalIndexCount, largeIndices))
				return false;
		}
		if (viewGeneratedInputs & PopcornFX::Drawers::GenInput_Position)
		{
			PK_ASSERT(viewDep.m_Positions.Valid());
			TStridedMemoryView<CFloat3, 0x10>	positions(null, totalVertexCount, 0x10);
			if (!viewDep.m_Positions->Map(positions))
				return false;
			dstView.m_Exec_PNT.m_Positions = positions;
		}
		if (viewGeneratedInputs & PopcornFX::Drawers::GenInput_Normal)
		{
			PK_ASSERT(viewDep.m_Normals.Valid());
			TStridedMemoryView<CFloat3, 0x10>	normals(null, totalVertexCount, 0x10);
			if (!viewDep.m_Normals->Map(normals))
				return false;
			dstView.m_Exec_PNT.m_Normals = normals;
		}
		if (viewGeneratedInputs & PopcornFX::Drawers::GenInput_Tangent)
		{
			PK_ASSERT(viewDep.m_Tangents.Valid());
			TStridedMemoryView<CFloat4, 0x10>	tangents(null, totalVertexCount, 0x10);
			if (!viewDep.m_Tangents->Map(tangents))
				return false;
			dstView.m_Exec_PNT.m_Tangents = tangents;
		}
		if (viewGeneratedInputs & PopcornFX::Drawers::GenInput_UVFactors)
		{
			PK_ASSERT(viewDep.m_UVFactors.Valid());
			TStridedMemoryView<CFloat4>	uvFactors(null, totalVertexCount);
			if (!viewDep.m_UVFactors->Map(uvFactors))
				return false;
			hasUVFactors = true;
			dstView.m_Exec_PNT.m_UVFactors4 = uvFactors;
		}
	}
	ribbonBatch->m_Exec_Texcoords.m_ForUVFactor = hasUVFactors;
	return true;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::MapBuffers(const SRenderContext &renderContext, const TMemoryView<SBBView> &views, PopcornFX::SGPURibbonBatchJobs *ribbonBatch, const PopcornFX::SGeneratedInputs &toMap)
{
	PK_ASSERT_NOT_REACHED();
	return false;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::LaunchCustomTasks(const SRenderContext &renderContext, const TMemoryView<const PopcornFX::Drawers::SRibbon_DrawRequest * const> &drawRequests, PopcornFX::Drawers::CRibbon_CPU *ribbonBatch)
{
	if (ribbonBatch == null)
	{
		PK_ASSERT_NOT_REACHED(); // GPU
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::LaunchCustomTasks(const SRenderContext &renderContext, const TMemoryView<const PopcornFX::Drawers::SRibbon_DrawRequest * const> &drawRequests, PopcornFX::Drawers::CCopyStream_CPU *ribbonBatch)
{
	PK_ASSERT_NOT_REACHED();
	return false;
}

//----------------------------------------------------------------------------

class FPopcornFXRibbonCollector : public FOneFrameResource
{
public:
	FPopcornFXVertexFactory		*m_VertexFactory = null;
	CPooledIndexBuffer			m_IndexBufferForRefCount;

	FPopcornFXRibbonCollector() { }
	~FPopcornFXRibbonCollector()
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

void	CRenderBatchPolicy::_IssueDrawCall_Ribbon(const SRenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Ribbon");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);

	FMeshElementCollector		*collector = view->Collector();
	const FPopcornFXSceneProxy	*sceneProxy = view->SceneProxy();
	PK_ASSERT(collector != null);
	PK_ASSERT(sceneProxy != null);

	const u32	indexCount = desc.m_TotalIndexCount;
	const u32	indexOffset = desc.m_IndexOffset;

	PK_ASSERT(indexOffset + indexCount <= m_TotalIndexCount);

	const bool	reverseCulling = sceneProxy->IsLocalToWorldDeterminantNegative();
	const bool	isSelected = sceneProxy->IsSelected();
	const bool	drawsVelocity = sceneProxy->DrawsVelocity();

	const FBoxSphereBounds	&bounds = sceneProxy->GetBounds();
	const FBoxSphereBounds	&localBounds = sceneProxy->GetLocalBounds();

	bool	hasPrecomputedVolumetricLightmap;
	FMatrix	previousLocalToWorld;
	int32	singleCaptureIndex;

	bool	outputVelocity;
	sceneProxy->GetScene().GetPrimitiveUniformShaderParameters_RenderThread(sceneProxy->GetPrimitiveSceneInfo(), hasPrecomputedVolumetricLightmap, previousLocalToWorld, singleCaptureIndex, outputVelocity);

	FMatrix	localToWorld = FMatrix::Identity;
	if (view->GlobalScale() != 1.0f)
		localToWorld *= view->GlobalScale();

	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return;
	CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();
	PK_ASSERT(matDesc.ValidForRendering());

	PK_ASSERT(m_RealViewCount >= m_ViewDependents.Count());
	PK_ASSERT(desc.m_ViewIndex < m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() == m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() <= renderContext.m_RendererSubView->SceneViews().Count());
	for (u32 iView = 0; iView < m_RealViewCount; ++iView)
	{
		if (desc.m_ViewIndex != iView)
			continue;

		const u32	realViewIndex = renderContext.m_RendererSubView->BBViews()[iView].m_ViewIndex;

		// Try and resolve from active views
		SViewDependent	*viewDep = null;
		for (u32 iViewDep = 0; iViewDep < m_ViewDependents.Count(); ++iViewDep)
		{
			if (m_ViewDependents[iViewDep].m_ViewIndex == iView)
			{
				viewDep = &m_ViewDependents[iViewDep];
				break;
			}
		}

		const bool	fullViewIndependent = viewDep == null;
		const bool	viewIndependentIndices = (fullViewIndependent && m_Indices.Valid()) || (!fullViewIndependent && !viewDep->m_Indices.Valid());
		const bool	viewIndependentPNT = (fullViewIndependent && m_Positions.Valid()) || (!fullViewIndependent && !viewDep->m_Positions.Valid());

		// Assert cannot have viewdep normals/tangents and indep pos + vice versa

		FPopcornFXVertexFactory			*vertexFactory = null;

		{
			PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Ribbon CollectorRes");

			FPopcornFXRibbonCollector	*collectorRes = &(collector->AllocateOneFrameResource<FPopcornFXRibbonCollector>());
			collectorRes->m_IndexBufferForRefCount = viewIndependentIndices ? m_Indices : viewDep->m_Indices;

			PK_ASSERT(collectorRes->m_VertexFactory == null);
			collectorRes->m_VertexFactory = new FPopcornFXVertexFactory(m_FeatureLevel);

			// !! if the vertexFactory is CACHED:
			// be carefull streams could change Strides and/or formats on the fly !
			vertexFactory = collectorRes->m_VertexFactory;
			vertexFactory->m_Positions.Setup(viewIndependentPNT ? m_Positions : viewDep->m_Positions);
			vertexFactory->m_Normals.Setup(viewIndependentPNT ? m_Normals : viewDep->m_Normals);
			vertexFactory->m_Tangents.Setup(viewIndependentPNT ? m_Tangents : viewDep->m_Tangents);
			vertexFactory->m_UVFactors.Setup(viewIndependentPNT ? m_UVFactors : viewDep->m_UVFactors);
			vertexFactory->m_UVScalesAndOffsets.Setup(m_UVRemaps);
			vertexFactory->m_Texcoords.Setup(m_Texcoords);
			vertexFactory->m_Texcoord2s.Setup(m_Texcoord2s);
			vertexFactory->m_AtlasIDs.Setup(m_AtlasIDs);

			FPopcornFXBillboardVSUniforms		vsUniforms;
			FPopcornFXBillboardCommonUniforms	commonUniforms;

			vsUniforms.SimData = m_SimData.Buffer()->SRV();
			vsUniforms.RendererType = static_cast<u32>(m_RendererType);
			vsUniforms.TotalParticleCount = m_TotalParticleCount;
			vsUniforms.InColorsOffset = m_AdditionalStreamOffsets[StreamOffset_Colors].OffsetForShaderConstant();
			vsUniforms.InEmissiveColorsOffset = m_AdditionalStreamOffsets[StreamOffset_EmissiveColors].OffsetForShaderConstant();
			vsUniforms.InAlphaCursorsOffset = m_AdditionalStreamOffsets[StreamOffset_AlphaCursors].OffsetForShaderConstant();
			vsUniforms.InDynamicParameter1sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam1s].OffsetForShaderConstant();
			vsUniforms.InDynamicParameter2sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam2s].OffsetForShaderConstant();
			vsUniforms.InDynamicParameter3sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam3s].OffsetForShaderConstant();

			commonUniforms.HasSecondUVSet = m_SecondUVSet;
			commonUniforms.FlipUVs = m_FlipUVs;
			commonUniforms.NeedsBTN = m_NeedsBTN;
			commonUniforms.CorrectRibbonDeformation = m_CorrectRibbonDeformation;

			vertexFactory->m_VSUniformBuffer = FPopcornFXBillboardVSUniformsRef::CreateUniformBufferImmediate(vsUniforms, UniformBuffer_SingleDraw);
			vertexFactory->m_CommonUniformBuffer = FPopcornFXBillboardCommonUniformsRef::CreateUniformBufferImmediate(commonUniforms, UniformBuffer_SingleDraw);

			PK_ASSERT(!vertexFactory->IsInitialized());
			vertexFactory->InitResource();
		}

		if (!PK_VERIFY(vertexFactory->IsInitialized()))
			return;

		FMeshBatch		&meshBatch = collector->AllocateMesh();

		meshBatch.VertexFactory = vertexFactory;
		meshBatch.CastShadow = matDesc.m_CastShadows;
		meshBatch.bUseAsOccluder = false;
		meshBatch.ReverseCulling = reverseCulling;
		meshBatch.Type = PT_TriangleList;
		meshBatch.DepthPriorityGroup = renderContext.m_RendererSubView->SceneProxy()->GetDepthPriorityGroup(view->SceneViews()[realViewIndex].m_SceneView);
		meshBatch.bUseWireframeSelectionColoring = isSelected;
		meshBatch.bCanApplyViewModeOverrides = true;
		meshBatch.MaterialRenderProxy = matDesc.RenderProxy();

		FMeshBatchElement	&meshElement = meshBatch.Elements[0];

		meshElement.IndexBuffer = viewIndependentIndices ? m_Indices.Buffer() : viewDep->m_Indices.Buffer();
		meshElement.FirstIndex = indexOffset;
		meshElement.NumPrimitives = indexCount / 3;
		meshElement.MinVertexIndex = 0;
		meshElement.MaxVertexIndex = m_TotalVertexCount - 1;
		FDynamicPrimitiveUniformBuffer	&dynamicPrimitiveUniformBuffer = collector->AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
		dynamicPrimitiveUniformBuffer.Set(localToWorld, previousLocalToWorld, bounds, localBounds, true, hasPrecomputedVolumetricLightmap, drawsVelocity, outputVelocity);
		meshElement.PrimitiveUniformBuffer = dynamicPrimitiveUniformBuffer.UniformBuffer.GetUniformBufferRHI();

		PK_ASSERT(meshElement.NumPrimitives > 0);

		{
			INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsCount, 1);

			collector->AddMesh(realViewIndex, meshBatch);
		}
	}
}

//----------------------------------------------------------------------------
//
// Triangles
//
//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::MapBuffers(const SRenderContext &renderContext, const TMemoryView<SBBView> &views, PopcornFX::STriangleBatchJobs *triangleBatch, const PopcornFX::SGeneratedInputs &toMap)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::MapBuffers_Triangles");

	if (triangleBatch == null)
	{
		PK_ASSERT_NOT_REACHED(); // GPU
		return false;
	}

	const u32	totalIndexCount = m_TotalIndexCount;
	const u32	totalVertexCount = m_TotalVertexCount;
	const u32	totalParticleCount = m_TotalParticleCount;

	// View independent
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Indices)
	{
		PK_ASSERT(m_Indices.Valid());
		void	*indices = null;
		bool	largeIndices = false;

		if (!m_Indices->Map(indices, largeIndices, totalIndexCount))
			return false;
		if (!triangleBatch->m_Exec_Indices.m_IndexStream.Setup(indices, totalIndexCount, largeIndices))
			return false;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Position)
	{
		PK_ASSERT(m_Positions.Valid());
		TStridedMemoryView<CFloat3, 0x10>	positions(null, totalVertexCount, 0x10);
		if (!m_Positions->Map(positions))
			return false;
		triangleBatch->m_Exec_PNT.m_Positions = positions;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Normal)
	{
		PK_ASSERT(m_Normals.Valid());
		TStridedMemoryView<CFloat3, 0x10>	normals(null, totalVertexCount, 0x10);
		if (!m_Normals->Map(normals))
			return false;
		triangleBatch->m_Exec_PNT.m_Normals = normals;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Tangent)
	{
		PK_ASSERT(m_Tangents.Valid());
		TStridedMemoryView<CFloat4, 0x10>	tangents(null, totalVertexCount, 0x10);
		if (!m_Tangents->Map(tangents))
			return false;
		triangleBatch->m_Exec_PNT.m_Tangents = tangents;
	}
	if (toMap.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UV0)
	{
		PK_ASSERT(m_Texcoords.Valid());
		TStridedMemoryView<CFloat2>	uv0s(null, totalVertexCount);
		if (!m_Texcoords->Map(uv0s))
			return false;
		triangleBatch->m_Exec_PNT.m_Texcoords = uv0s;
	}

	if (!toMap.m_AdditionalGeneratedInputs.Empty())
	{
		PK_ASSERT(m_SimData.Valid());
		TMemoryView<float>	simData;
		{
			PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::MapBuffers_Triangles (particle data buffer)");
			const u32	elementCount = m_SimDataBufferSizeInBytes / sizeof(float);
			if (!m_SimData->Map(simData, elementCount))
				return false;
		}
		float	*_data = simData.Data();

		// Additional inputs
		const u32	aFieldCount = m_AdditionalInputs.Count();

		if (!PK_VERIFY(m_MappedAdditionalInputs.Resize(aFieldCount)))
			return false;
		for (u32 iField = 0; iField < aFieldCount; ++iField)
		{
			SAdditionalInput					&field = m_AdditionalInputs[iField];
			PopcornFX::Drawers::SCopyFieldDesc	&desc = m_MappedAdditionalInputs[iField];

			desc.m_Storage.m_Count = totalParticleCount;
			desc.m_Storage.m_RawDataPtr = reinterpret_cast<u8*>(PopcornFX::Mem::AdvanceRawPointer(_data, field.m_BufferOffset));
			desc.m_Storage.m_Stride = field.m_ByteSize;
			desc.m_AdditionalInputIndex = field.m_AdditionalInputIndex;
		}

		triangleBatch->m_Exec_CopyField.m_FieldsToCopy = m_MappedAdditionalInputs.View();
		triangleBatch->m_Exec_CopyField.m_PerVertex = false;
	}

	// View deps
	const u32	activeViewCount = m_ViewDependents.Count();
	PK_ASSERT(activeViewCount == triangleBatch->m_PerView.Count());
	for (u32 iView = 0; iView < activeViewCount; ++iView)
	{
		const u32								viewGeneratedInputs = toMap.m_PerViewGeneratedInputs[iView].m_GeneratedInputs;
		SViewDependent							&viewDep = m_ViewDependents[iView];
		PopcornFX::STriangleBatchJobs::SPerView	&dstView = triangleBatch->m_PerView[iView];

		if (viewGeneratedInputs & PopcornFX::Drawers::GenInput_Indices)
		{
			PK_ASSERT(viewDep.m_Indices.Valid());
			void	*indices = null;
			bool	largeIndices = false;

			if (!viewDep.m_Indices->Map(indices, largeIndices, totalIndexCount))
				return false;
			if (!dstView.m_Exec_Indices.m_IndexStream.Setup(indices, totalIndexCount, largeIndices))
				return false;
		}
	}
	return true;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::MapBuffers(const SRenderContext& renderContext, const TMemoryView<SBBView>& views, PopcornFX::SGPUTriangleBatchJobs* triangleBatch, const PopcornFX::SGeneratedInputs& toMap)
{
	PK_ASSERT_NOT_REACHED();
	return false;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::LaunchCustomTasks(const SRenderContext &renderContext, const TMemoryView<const PopcornFX::Drawers::STriangle_DrawRequest * const> &drawRequests, PopcornFX::Drawers::CTriangle_CPU *triangleBatch)
{
	if (triangleBatch == null)
	{
		PK_ASSERT_NOT_REACHED(); // GPU
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::LaunchCustomTasks(const SRenderContext& renderContext, const TMemoryView<const PopcornFX::Drawers::STriangle_DrawRequest* const>& drawRequests, PopcornFX::Drawers::CCopyStream_CPU* triangleBatch)
{
	PK_ASSERT_NOT_REACHED();
	return false;
}

//----------------------------------------------------------------------------

class FPopcornFXTriangleCollector : public FOneFrameResource
{
public:
	FPopcornFXVertexFactory		*m_VertexFactory = null;
	CPooledIndexBuffer			m_IndexBufferForRefCount;

	FPopcornFXTriangleCollector() { }
	~FPopcornFXTriangleCollector()
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

void	CRenderBatchPolicy::_IssueDrawCall_Triangle(const SRenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Triangle");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);

	FMeshElementCollector		*collector = view->Collector();
	const FPopcornFXSceneProxy	*sceneProxy = view->SceneProxy();
	PK_ASSERT(collector != null);
	PK_ASSERT(sceneProxy != null);

	const u32	indexCount = desc.m_TotalIndexCount;
	const u32	indexOffset = desc.m_IndexOffset;

	PK_ASSERT(indexOffset + indexCount <= m_TotalIndexCount);

	const bool	reverseCulling = sceneProxy->IsLocalToWorldDeterminantNegative();
	const bool	isSelected = sceneProxy->IsSelected();
	const bool	drawsVelocity = sceneProxy->DrawsVelocity();

	const FBoxSphereBounds	&bounds = sceneProxy->GetBounds();
	const FBoxSphereBounds	&localBounds = sceneProxy->GetLocalBounds();

	bool	hasPrecomputedVolumetricLightmap;
	FMatrix	previousLocalToWorld;
	int32	singleCaptureIndex;

	bool	outputVelocity;
	sceneProxy->GetScene().GetPrimitiveUniformShaderParameters_RenderThread(sceneProxy->GetPrimitiveSceneInfo(), hasPrecomputedVolumetricLightmap, previousLocalToWorld, singleCaptureIndex, outputVelocity);

	FMatrix	localToWorld = FMatrix::Identity;
	if (view->GlobalScale() != 1.0f)
		localToWorld *= view->GlobalScale();

	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return;
	CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();
	PK_ASSERT(matDesc.ValidForRendering());

	PK_ASSERT(desc.m_ViewIndex < m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() == m_RealViewCount);
	PK_ASSERT(renderContext.m_RendererSubView->BBViews().Count() <= renderContext.m_RendererSubView->SceneViews().Count());
	for (u32 iView = 0; iView < m_RealViewCount; ++iView)
	{
		if (desc.m_ViewIndex != iView)
			continue;

		const u32	realViewIndex = renderContext.m_RendererSubView->BBViews()[iView].m_ViewIndex;

		// Try and resolve from active views
		SViewDependent* viewDep = null;
		for (u32 iViewDep = 0; iViewDep < m_ViewDependents.Count(); ++iViewDep)
		{
			if (m_ViewDependents[iViewDep].m_ViewIndex == iView)
			{
				viewDep = &m_ViewDependents[iViewDep];
				break;
			}
		}

		const bool	fullViewIndependent = viewDep == null;
		const bool	viewIndependentIndices = (fullViewIndependent && m_Indices.Valid()) || (!fullViewIndependent && !viewDep->m_Indices.Valid());

		// This should never happen
		if (!viewIndependentIndices && (viewDep == null || !viewDep->m_Indices.Valid()))
			return;

		FPopcornFXVertexFactory			*vertexFactory = null;

		{
			PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Triangle CollectorRes");

			FPopcornFXTriangleCollector	*collectorRes = &(collector->AllocateOneFrameResource<FPopcornFXTriangleCollector>());
			collectorRes->m_IndexBufferForRefCount = viewIndependentIndices ? m_Indices : viewDep->m_Indices;

			PK_ASSERT(collectorRes->m_VertexFactory == null);
			collectorRes->m_VertexFactory = new FPopcornFXVertexFactory(m_FeatureLevel);

			// !! if the vertexFactory is CACHED:
			// be carefull streams could change Strides and/or formats on the fly !
			vertexFactory = collectorRes->m_VertexFactory;
			vertexFactory->m_Positions.Setup(m_Positions);
			vertexFactory->m_Normals.Setup(m_Normals);
			vertexFactory->m_Tangents.Setup(m_Tangents);
			vertexFactory->m_Texcoords.Setup(m_Texcoords);
			//vertexFactory->m_AtlasIDs.Setup(m_AtlasIDs);

			FPopcornFXBillboardVSUniforms		vsUniforms;
			FPopcornFXBillboardCommonUniforms	commonUniforms;

			vsUniforms.SimData = m_SimData.Buffer()->SRV();
			vsUniforms.RendererType = static_cast<u32>(m_RendererType);
			vsUniforms.InColorsOffset = m_AdditionalStreamOffsets[StreamOffset_Colors].OffsetForShaderConstant();
			vsUniforms.InEmissiveColorsOffset = m_AdditionalStreamOffsets[StreamOffset_EmissiveColors].OffsetForShaderConstant();
			vsUniforms.InAlphaCursorsOffset = m_AdditionalStreamOffsets[StreamOffset_AlphaCursors].OffsetForShaderConstant();
			vsUniforms.InDynamicParameter1sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam1s].OffsetForShaderConstant();
			vsUniforms.InDynamicParameter2sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam2s].OffsetForShaderConstant();
			vsUniforms.InDynamicParameter3sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam3s].OffsetForShaderConstant();

			commonUniforms.HasSecondUVSet = m_SecondUVSet;
			commonUniforms.FlipUVs = m_FlipUVs;
			commonUniforms.NeedsBTN = m_NeedsBTN;
			commonUniforms.CorrectRibbonDeformation = m_CorrectRibbonDeformation;

			vertexFactory->m_VSUniformBuffer = FPopcornFXBillboardVSUniformsRef::CreateUniformBufferImmediate(vsUniforms, UniformBuffer_SingleDraw);
			vertexFactory->m_CommonUniformBuffer = FPopcornFXBillboardCommonUniformsRef::CreateUniformBufferImmediate(commonUniforms, UniformBuffer_SingleDraw);

			PK_ASSERT(!vertexFactory->IsInitialized());
			vertexFactory->InitResource();
		}

		if (!PK_VERIFY(vertexFactory->IsInitialized()))
			return;

		FMeshBatch		&meshBatch = collector->AllocateMesh();

		meshBatch.VertexFactory = vertexFactory;
		meshBatch.CastShadow = matDesc.m_CastShadows;
		meshBatch.bUseAsOccluder = false;
		meshBatch.ReverseCulling = reverseCulling;
		meshBatch.Type = PT_TriangleList;
		meshBatch.DepthPriorityGroup = renderContext.m_RendererSubView->SceneProxy()->GetDepthPriorityGroup(view->SceneViews()[realViewIndex].m_SceneView);
		meshBatch.bUseWireframeSelectionColoring = isSelected;
		meshBatch.bCanApplyViewModeOverrides = true;
		meshBatch.MaterialRenderProxy = matDesc.RenderProxy();

		FMeshBatchElement	&meshElement = meshBatch.Elements[0];

		meshElement.IndexBuffer = viewIndependentIndices ? m_Indices.Buffer() : viewDep->m_Indices.Buffer();
		meshElement.FirstIndex = indexOffset;
		meshElement.NumPrimitives = indexCount / 3;
		meshElement.MinVertexIndex = 0;
		meshElement.MaxVertexIndex = m_TotalVertexCount - 1;
		FDynamicPrimitiveUniformBuffer	&dynamicPrimitiveUniformBuffer = collector->AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
		dynamicPrimitiveUniformBuffer.Set(localToWorld, previousLocalToWorld, bounds, localBounds, true, hasPrecomputedVolumetricLightmap, drawsVelocity, outputVelocity);
		meshElement.PrimitiveUniformBuffer = dynamicPrimitiveUniformBuffer.UniformBuffer.GetUniformBufferRHI();

		PK_ASSERT(meshElement.NumPrimitives > 0);

		{
			INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsCount, 1);

			collector->AddMesh(realViewIndex, meshBatch);
		}
	}
}

//----------------------------------------------------------------------------
//
// Meshes
//
//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::MapBuffers(const SRenderContext &renderContext, const TMemoryView<SBBView> &views, PopcornFX::SMeshBatchJobs *meshBatch, const PopcornFX::SGeneratedInputs &toMap)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::MapBuffers_Meshes");

	if (meshBatch == null)
	{
		// GPU
		return true;
	}
	PK_ASSERT(m_ViewDependents.Empty());

	// Note: all of this verbose code related to additional inputs will be removed in future PK versions when all particle data is mapped in a single GPU buffer.
	// This will allow support for all 4 shader parameters + VATs without vertex attribute limitations and will remove the need for explicit naming.
	const bool		hasColors = m_AdditionalStreamOffsets[StreamOffset_Colors].Valid();
	const bool		hasVATCursors = m_AdditionalStreamOffsets[StreamOffset_VATCursors].Valid();
	const bool		hasDynParam0 = m_AdditionalStreamOffsets[StreamOffset_DynParam0s].Valid();
	const bool		hasDynParam1 = m_AdditionalStreamOffsets[StreamOffset_DynParam1s].Valid();
	const bool		hasDynParam2 = m_AdditionalStreamOffsets[StreamOffset_DynParam2s].Valid();
	const u32		mappedAdditionalInputCount = (u32)hasColors + (u32)hasVATCursors + (u32)hasDynParam0 + (u32)hasDynParam1 + (u32)hasDynParam2;

#if (ENGINE_MINOR_VERSION < 25)
	const u32		alignedCount = PopcornFX::Mem::Align<0x10>(m_TotalParticleCount + 0x10);
	u32			wbSizeInBytes = alignedCount * sizeof(CFloat4x4)
		+ mappedAdditionalInputCount * alignedCount * sizeof(CFloat4); // Because we know all accepted additional inputs for meshes are CFloat4

	u8	*wb = null;
	if (!m_Instanced)
	{
		wb = m_NonInstanced_Wb.GetSome<u8>(wbSizeInBytes);
		if (!PK_VERIFY(wb != null))
			return false;
	}
#endif // (ENGINE_MINOR_VERSION < 25)

	// We shouldn't have anything else than matrices to generate
	if (toMap.m_GeneratedInputs == PopcornFX::Drawers::GenInput_Matrices)
	{
#if (ENGINE_MINOR_VERSION < 25)
		if (m_Instanced)
		{
#endif // (ENGINE_MINOR_VERSION < 25)
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
#if (ENGINE_MINOR_VERSION < 25)
		}
		else
		{
			m_Mapped_Matrices = TStridedMemoryView<CFloat4x4>(reinterpret_cast<CFloat4x4*>(wb), m_TotalParticleCount, sizeof(CFloat4x4));
			wb += alignedCount * m_Mapped_Matrices.Stride();
		}
#endif // (ENGINE_MINOR_VERSION < 25)
	}

	// Additional input mapping (named because re-used when issuing draw calls)
#if (ENGINE_MINOR_VERSION < 25)
	if (m_Instanced)
	{
#endif // (ENGINE_MINOR_VERSION < 25)
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
#if (ENGINE_MINOR_VERSION < 25)
	}
	else
	{
		if (hasColors)
		{
			m_Mapped_Param_Colors = TStridedMemoryView<CFloat4>(reinterpret_cast<CFloat4*>(wb), m_TotalParticleCount, sizeof(CFloat4));
			wb += alignedCount * m_Mapped_Param_Colors.Stride();
		}
		if (hasVATCursors)
		{
			m_Mapped_Param_VATCursors = TStridedMemoryView<float>(reinterpret_cast<float*>(wb), m_TotalParticleCount, sizeof(float));
			wb += alignedCount * m_Mapped_Param_VATCursors.Stride();
		}
		if (hasDynParam0)
		{
			m_Mapped_Param_DynamicParameter0 = TStridedMemoryView<CFloat4>(reinterpret_cast<CFloat4*>(wb), m_TotalParticleCount, sizeof(CFloat4));
			wb += alignedCount * m_Mapped_Param_DynamicParameter0.Stride();
		}
		if (hasDynParam1)
		{
			m_Mapped_Param_DynamicParameter1 = TStridedMemoryView<CFloat4>(reinterpret_cast<CFloat4*>(wb), m_TotalParticleCount, sizeof(CFloat4));
			wb += alignedCount * m_Mapped_Param_DynamicParameter1.Stride();
		}
		if (hasDynParam2)
		{
			m_Mapped_Param_DynamicParameter2 = TStridedMemoryView<CFloat4>(reinterpret_cast<CFloat4*>(wb), m_TotalParticleCount, sizeof(CFloat4));
			wb += alignedCount * m_Mapped_Param_DynamicParameter2.Stride();
		}
	}
#endif // (ENGINE_MINOR_VERSION < 25)

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
		meshBatch->m_Exec_CopyField.m_FieldsToCopy = m_MappedAdditionalInputs.View();
	}

	meshBatch->m_Exec_Matrices.m_PositionScale = 100.0f;
	meshBatch->m_Exec_Matrices.m_Matrices = TStridedMemoryView<CFloat4x4>(&m_Mapped_Matrices[0], m_Mapped_Matrices.Count(), sizeof(m_Mapped_Matrices[0]));
	return true;
}

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::LaunchCustomTasks(const SRenderContext &renderContext, const TMemoryView<const PopcornFX::Drawers::SMesh_DrawRequest * const> &drawRequests, PopcornFX::Drawers::CMesh_CPU *meshBatch)
{
	if (meshBatch == null)
	{
#if (ENGINE_MINOR_VERSION < 25)
		if (!m_Instanced)
		{
			PK_ASSERT_NOT_REACHED();
			return false;
		}
#endif // (ENGINE_MINOR_VERSION < 25)

		PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::LaunchCustom Jobs (Meshes)");
		PK_ASSERT(!drawRequests.Empty());
		PK_ASSERT(drawRequests.First() != null);
		PK_ASSERT(drawRequests.First()->GPUStorage());

		const PopcornFX::Drawers::SMesh_BillboardingRequest	&compatBr = drawRequests.First()->m_BB;

#if (PK_HAS_GPU != 0)
		FRHICommandListImmediate	&RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

		using namespace		PopcornFXBillboarder;

		const ERHIFeatureLevel::Type		featureLevel = m_FeatureLevel;
		TShaderMapRef< FBillboarderMeshCS >	billboarderCS(GetGlobalShaderMap(featureLevel));

		CRenderBatchManager	*rbManager = renderContext.m_RenderBatchManager;
		PK_ASSERT(rbManager != null);

		FMeshCS_Params	_bbParams;
		FMeshCS_Params	*bbParams = &_bbParams;

		{
			SCOPED_GPU_STAT(RHICmdList, PopcornFXMeshBillboarding);

			u32			nextParticlesOffset = 0;
			const u32	baseParticlesOffset = 0;
			const u32	drCount = drawRequests.Count();
			for (u32 iDr = 0; iDr < drCount; ++iDr)
			{
				const PopcornFX::Drawers::SMesh_DrawRequest			&dr = *drawRequests[iDr];
				const PopcornFX::Drawers::SMesh_BillboardingRequest	&br = dr.m_BB;
				PK_ASSERT(dr.Valid());

				const u32	pCount = dr.InputParticleCount();
				PK_ASSERT(pCount > 0);

				const u32	particlesOffset = nextParticlesOffset;
				nextParticlesOffset += pCount;

				const PopcornFX::CParticleStreamToRender_GPU	*stream = dr.StreamToRender_GPU();
				if (!PK_VERIFY(stream != null))
					return false;

				// Inputs (uniforms)
				bbParams->m_ParticleCount = pCount;
				bbParams->m_InputOffset = 0; // FUTURE stream pages
				bbParams->m_OutputVertexOffset = baseParticlesOffset + particlesOffset;
				bbParams->m_PositionsScale = 100.0f;
#if (PK_GPU_D3D11 == 1)
				if (renderContext.m_API == SRenderContext::D3D11)
				{
					const PopcornFX::CParticleStreamToRender_D3D11	&stream_D3D11 = static_cast<const PopcornFX::CParticleStreamToRender_D3D11&>(*stream);

					bbParams->m_LiveParticleCount = StreamBufferSRVToRHI(&stream_D3D11.StreamSizeBuf(), sizeof(u32) * 4, sizeof(u32));
					bbParams->m_InSimData = StreamBufferSRVToRHI(&stream_D3D11.StreamBuffer(), stream_D3D11.StreamSizeEst(), sizeof(float));
				}
#endif // (PK_GPU_D3D11 == 1)
#if (PK_GPU_D3D12 == 1)
				if (renderContext.m_API == SRenderContext::D3D12)
				{
					const PopcornFX::CParticleStreamToRender_D3D12	&stream_D3D12 = static_cast<const PopcornFX::CParticleStreamToRender_D3D12&>(*stream);

					bbParams->m_LiveParticleCount = StreamBufferSRVToRHI(&stream_D3D12.StreamSizeBuf(), 16);
					bbParams->m_InSimData = StreamBufferSRVToRHI(&stream_D3D12.StreamBuffer(), 16);
				}
#endif // (PK_GPU_D3D12 == 1)

				// Outputs
				bbParams->SetOutput(EOutput::OutMatrices, m_Instanced_Matrices);
				if (m_AdditionalStreamOffsets[StreamOffset_Colors].Valid())
					bbParams->SetOutput(EOutput::OutColors, m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_Colors].InputId()].m_VertexBuffer);
				if (m_AdditionalStreamOffsets[StreamOffset_VATCursors].Valid())
					bbParams->SetOutput(EOutput::OutVATCursors, m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_VATCursors].InputId()].m_VertexBuffer);
				if (m_AdditionalStreamOffsets[StreamOffset_DynParam0s].Valid())
					bbParams->SetOutput(EOutput::OutDynamicParameter0s, m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_DynParam0s].InputId()].m_VertexBuffer);
				if (m_AdditionalStreamOffsets[StreamOffset_DynParam1s].Valid())
					bbParams->SetOutput(EOutput::OutDynamicParameter1s, m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_DynParam1s].InputId()].m_VertexBuffer);
				if (m_AdditionalStreamOffsets[StreamOffset_DynParam2s].Valid())
					bbParams->SetOutput(EOutput::OutDynamicParameter2s, m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_DynParam2s].InputId()].m_VertexBuffer);

				// Inputs
				bbParams->SetInputOffsetIFP<CFloat3, 12>(EInput::InPositions, *stream, br.m_PositionStreamId, CFloat4(0.f));
				bbParams->SetInputOffsetIFP<CQuaternion, 16>(EInput::InOrientations, *stream, br.m_OrientationStreamId, CFloat4(0.0f, 0.0f, 0.0f, 1.0f));
				bbParams->SetInputOffsetIFP<CFloat3, 12>(EInput::InScales, *stream, br.m_ScaleStreamId, CFloat4(1.0f));
				if (m_AdditionalStreamOffsets[StreamOffset_Colors].Valid())
					bbParams->SetInputOffsetIFP<CFloat4, 16>(EInput::InColors, *stream, br.m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_Colors].InputId()].m_StreamId, CFloat4(1.0f));
				if (m_AdditionalStreamOffsets[StreamOffset_VATCursors].Valid())
					bbParams->SetInputOffsetIFP<float, 4>(EInput::InVATCursors, *stream, br.m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_VATCursors].InputId()].m_StreamId, CFloat4(0.0f));
				if (m_AdditionalStreamOffsets[StreamOffset_DynParam0s].Valid())
					bbParams->SetInputOffsetIFP<CFloat4, 16>(EInput::InDynamicParameter0s, *stream, br.m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_DynParam0s].InputId()].m_StreamId, CFloat4(0.0f));
				if (m_AdditionalStreamOffsets[StreamOffset_DynParam1s].Valid())
					bbParams->SetInputOffsetIFP<CFloat4, 16>(EInput::InDynamicParameter1s, *stream, br.m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_DynParam1s].InputId()].m_StreamId, CFloat4(0.0f));
				if (m_AdditionalStreamOffsets[StreamOffset_DynParam2s].Valid())
					bbParams->SetInputOffsetIFP<CFloat4, 16>(EInput::InDynamicParameter2s, *stream, br.m_AdditionalInputs[m_AdditionalStreamOffsets[StreamOffset_DynParam2s].InputId()].m_StreamId, CFloat4(0.0f));

#if (ENGINE_MINOR_VERSION >= 25)
				RHICmdList.SetComputeShader(billboarderCS.GetComputeShader());
#endif //(ENGINE_MINOR_VERSION >= 25)
				billboarderCS->Dispatch(RHICmdList, *bbParams);
			}

			PK_ASSERT(nextParticlesOffset == m_TotalParticleCount);
		}
#else
		PK_ASSERT_NOT_REACHED();
		return false;
#endif // (PK_HAS_GPU != 0)
		return true;
	}
	return true;
}

//----------------------------------------------------------------------------

class FPopcornFXMeshCollector : public FOneFrameResource
{
public:
	FPopcornFXMeshVertexFactory		*m_VertexFactory = null;
	FPopcornFXMeshUserData			m_UserData; // We can get rid of that after UE4.25

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

void	CRenderBatchPolicy::_PreSetupMeshData(FPopcornFXMeshVertexFactory::FDataType &meshData, const FStaticMeshLODResources &meshResources)
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

void	CRenderBatchPolicy::_CreateMeshVertexFactory(u32 buffersOffset, FMeshElementCollector *collector, FPopcornFXMeshVertexFactory *&outFactory, FPopcornFXMeshCollector *&outCollectorRes)
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
#if (ENGINE_MINOR_VERSION < 25)
	if (m_Instanced)
	{
#endif // (ENGINE_MINOR_VERSION < 25)
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
#if (ENGINE_MINOR_VERSION < 25)
	}
#endif // (ENGINE_MINOR_VERSION < 25)
	m_VFData.bInitialized = true;

	// Create the vertex factory
	{
		PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Mesh CollectorRes");

		outCollectorRes = &(collector->AllocateOneFrameResource<FPopcornFXMeshCollector>());
		check(outCollectorRes != null);

		PK_ASSERT(outCollectorRes->m_VertexFactory == null);
		outCollectorRes->m_VertexFactory = new FPopcornFXMeshVertexFactory(m_FeatureLevel);

		outFactory = outCollectorRes->m_VertexFactory;
		outFactory->SetData(m_VFData);
		PK_ASSERT(!outFactory->IsInitialized());
		outFactory->InitResource();

#if (ENGINE_MINOR_VERSION < 25)
		outCollectorRes->m_UserData.m_Instanced = m_Instanced;
		if (!m_Instanced)
		{
			PK_ASSERT(!m_Mapped_Matrices.Empty());
			outCollectorRes->m_UserData.m_Instance_Matrices = m_Mapped_Matrices;

			if (hasColors)
			{
				PK_ASSERT(!m_Mapped_Param_Colors.Empty());
				outCollectorRes->m_UserData.m_Instance_Param_DiffuseColors = m_Mapped_Param_Colors;
			}
			if (hasVATCursors)
			{
				PK_ASSERT(!m_Mapped_Param_VATCursors.Empty());
				outCollectorRes->m_UserData.m_Instance_Param_VATCursors = m_Mapped_Param_VATCursors;
			}
			if (hasDynParam0)
			{
				PK_ASSERT(!m_Mapped_Param_DynamicParameter0.Empty());
				outCollectorRes->m_UserData.m_Instance_Param_DynamicParameter0 = m_Mapped_Param_DynamicParameter0;
			}
			if (hasDynParam1)
			{
				PK_ASSERT(!m_Mapped_Param_DynamicParameter1.Empty());
				outCollectorRes->m_UserData.m_Instance_Param_DynamicParameter1 = m_Mapped_Param_DynamicParameter1;
			}
			if (hasDynParam2)
			{
				PK_ASSERT(!m_Mapped_Param_DynamicParameter2.Empty());
				outCollectorRes->m_UserData.m_Instance_Param_DynamicParameter2 = m_Mapped_Param_DynamicParameter2;
			}
		}
#else
		outCollectorRes->m_UserData.m_Instanced = true; // TODO: Remove this entirely
#endif //(ENGINE_MINOR_VERSION < 25)
	}
}

//----------------------------------------------------------------------------

#if RHI_RAYTRACING
void	CRenderBatchPolicy::_IssueDrawCall_Mesh_AccelStructs(const SRenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Mesh");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);

	const bool				gpuStorage = desc.m_DrawRequests.First()->GPUStorage();
	FMeshElementCollector	*collector = view->RTCollector();

	if (gpuStorage) // not supported yet
		return;
	if (desc.m_ViewIndex != 0) // submit once
		return;

#if (ENGINE_MINOR_VERSION < 25)
	if (!m_Instanced)
		return;
#endif // (ENGINE_MINOR_VERSION < 25)

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
			rayTracingInstance.BuildInstanceMaskAndFlags();

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
			rayTracingInstance.BuildInstanceMaskAndFlags();

			view->OutRayTracingInstances()->Add(rayTracingInstance);
		}
	}
}
#endif // RHI_RAYTRACING

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::_BuildMeshBatch_Mesh(	FMeshBatch &meshBatch,
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
#if (ENGINE_MINOR_VERSION >= 25)
	batchElement.NumInstances = sectionParticleCount; // Ignored by raytracing pass
#else
	batchElement.NumInstances = m_Instanced ? sectionParticleCount : 1;
#endif // (ENGINE_MINOR_VERSION >= 25)

	batchElement.IndexBuffer = &matDesc.m_LODResources->IndexBuffer;
	batchElement.NumPrimitives = section.NumTriangles;

	batchElement.UserIndex = 0;
#if (ENGINE_MINOR_VERSION < 25)
	batchElement.bIsInstancedMesh = m_Instanced;
#endif // (ENGINE_MINOR_VERSION < 25)

	return true;
}

//----------------------------------------------------------------------------

void	CRenderBatchPolicy::_IssueDrawCall_Mesh(const SRenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Mesh");

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
			meshBatch.Elements[0].UserData = &collectorRes->m_UserData;
			meshBatch.Elements[0].PrimitiveUniformBuffer = sceneProxy->GetUniformBuffer();

#if (ENGINE_MINOR_VERSION < 25)
			if (!m_Instanced)
			{
				meshBatch.Elements.Reserve(sectionPCount);
				for (u32 batchi = 1; batchi < sectionPCount; ++batchi)
				{
					FMeshBatchElement* NextElement = new(meshBatch.Elements) FMeshBatchElement();
					*NextElement = meshBatch.Elements[0];
					NextElement->UserIndex = batchi + buffersOffset;
				}
			}
#endif // (ENGINE_MINOR_VERSION < 25)

			buffersOffset += m_HasMeshIDs ? sectionPCount : 0;

			{
				INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsCount, 1);
				collector->AddMesh(realViewIndex, meshBatch);
			}
		} // for (sections)
	} // for (views)
}

//----------------------------------------------------------------------------
//
// Lights
//
//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::LaunchCustomTasks(const SRenderContext &renderContext, const TMemoryView<const PopcornFX::Drawers::SLight_DrawRequest * const> &drawRequests, PopcornFX::Drawers::CBillboard_CPU *emptyBatch)
{
	PK_ASSERT(emptyBatch == null);
	return true;
}

//----------------------------------------------------------------------------

void	CRenderBatchPolicy::_IssueDrawCall_Light(const SRenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc, SCollectedDrawCalls &output)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Light");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);
	const float				globalScale = view->GlobalScale();

	PK_ASSERT(!desc.m_DrawRequests.Empty());
	PK_ASSERT(desc.m_TotalParticleCount > 0);
	const u32		totalParticleCount = desc.m_TotalParticleCount;
	PK_ASSERT(totalParticleCount == m_TotalParticleCount);

	PopcornFX::TArray<FSimpleLightPerViewEntry>		&lightPositions = output.m_LightPositions;
	PopcornFX::TArray<FSimpleLightEntry>			&lightDatas = output.m_LightDatas;
	PK_ASSERT(lightPositions.Count() == lightDatas.Count());

	if (!lightPositions.Reserve(lightPositions.Count() + totalParticleCount) ||
		!lightDatas.Reserve(lightDatas.Count() + totalParticleCount))
	{
		PK_ASSERT_NOT_REACHED();
		return;
	}

	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return;
	CMaterialDesc_RenderThread	&matDesc = matCache->RenderThread_Desc();

	const bool		forceLitTranslucentGeometry = renderContext.m_RenderBatchManager->ForceLightsTranslucent();

	const u32	drCount = desc.m_DrawRequests.Count();
	for (u32 iDr = 0; iDr < drCount; ++iDr)
	{
		const PopcornFX::Drawers::SLight_DrawRequest			&drawRequest = *static_cast<const PopcornFX::Drawers::SLight_DrawRequest*>(desc.m_DrawRequests[iDr]);
		const PopcornFX::Drawers::SLight_BillboardingRequest	&bbRequest = static_cast<const PopcornFX::Drawers::SLight_BillboardingRequest&>(drawRequest.BaseBillboardingRequest());
		if (drawRequest.StorageClass() != PopcornFX::CParticleStorageManager_MainMemory::DefaultStorageClass())
		{
			PK_ASSERT_NOT_REACHED();
			return;
		}

		PK_TODO("renderManager->CullTest");

		const PopcornFX::CParticleStreamToRender_MainMemory	*lockedStream = drawRequest.StreamToRender_MainMemory();
		if (!PK_VERIFY(lockedStream != null)) // Light particles shouldn't handle GPU streams for now
			return;

		const PopcornFX::CGuid	volScatteringIntensityStreamId = bbRequest.StreamId(PopcornFX::CStringId("AffectsVolumetricFog.VolumetricScatteringIntensity")); // tmp

		static const float		kMinLightSize = 1e-3f;
		static const float		kExponent = 16.f; // simulate unlinearizeFallOff function
												  //------------------------------
												  // float A = (1.0f) - (1.0f / ((k + 1.0f) * (k + 1.0f)));
												  // float B = (1.0f / ((k + 1.0f) * (k + 1.0f)));
												  // float C = (1.0f / ((x * k + 1.0f) * (x * k + 1.0f)));
												  // return (C - B) / A;
												  //------------------------------
		static const float		kAttenuationSteepness = bbRequest.m_AttenuationSteepness;
		static const float		kAttenuationMultiplier = 4.32f; // represent the function f(x) = 4.32f x + 1.07f
		static const float		kAttenuationMinimumCoeff = 1.07f; // that simulate UE light intensity compared to PK FX unit-less light intensity

		const float				kColorMultiplier = (kAttenuationSteepness > 0.f) ? kAttenuationSteepness * kAttenuationMultiplier + kAttenuationMinimumCoeff : 0.f; // color multiplier is used to add light intensity for UE
		static const float		kLightRadiusMultiplier = 3.f; // multiplier to remove PBR render from UE

		const bool				lightsTranslucentGeometry = forceLitTranslucentGeometry | matDesc.m_LightsTranslucent;

		const u32	pageCount = lockedStream->PageCount();
		for (u32 pagei = 0; pagei < pageCount; ++pagei)
		{
			const PopcornFX::CParticlePageToRender_MainMemory		*page = lockedStream->Page(pagei);
			PK_ASSERT(page != null);
			const u32	pcount = page == null ? 0 : page->InputParticleCount();
			if (pcount == 0)
				continue;

			// Position
			TStridedMemoryView<const CFloat3>	positions = page->StreamForReading<CFloat3>(bbRequest.m_PositionStreamId);
			PK_ASSERT(positions.Count() == pcount);

			// Radius
			TStridedMemoryView<const float>		sizes;
			PK_ALIGN(0x10) float				defaultSize = 0.0f;
			if (PK_VERIFY(bbRequest.m_RangeStreamId.Valid()))
				sizes = page->StreamForReading<float>(bbRequest.m_RangeStreamId);
			else
				sizes = TStridedMemoryView<const float>(&defaultSize, pcount, 0);

			// Color
			TStridedMemoryView<const CFloat3>	colors(&CFloat3::ONE, pcount, 0);
			if (bbRequest.m_ColorStreamId.Valid())
				colors = TStridedMemoryView<const CFloat3>::Reinterpret(page->StreamForReading<CFloat4>(bbRequest.m_ColorStreamId));

			PK_ALIGN(0X10) float				defaultScatteringIntensity = 0.0f; // By default, lights do not affect volumetric fog if not present in the light material
			TStridedMemoryView<const float>		volumetricScatteringIntensity(&defaultScatteringIntensity, pcount, 0);
			if (volScatteringIntensityStreamId.Valid())
				volumetricScatteringIntensity = page->StreamForReading<float>(volScatteringIntensityStreamId);

			if (!PK_VERIFY(!positions.Empty()) ||
				!PK_VERIFY(!sizes.Empty()) ||
				!PK_VERIFY(!colors.Empty()))
				continue;

			const u8						enabledTrue = u8(-1);
			TStridedMemoryView<const u8>	enabledParticles = (bbRequest.m_EnabledStreamId.Valid()) ? page->StreamForReading<bool>(bbRequest.m_EnabledStreamId) : TStridedMemoryView<const u8>(&enabledTrue, pcount, 0);

			for (u32 parti = 0; parti < pcount; ++parti)
			{
				if (!enabledParticles[parti])
					continue;

				const float					radius = sizes[parti] * globalScale * kLightRadiusMultiplier;
				if (radius < kMinLightSize)
					continue;
				PopcornFX::CGuid			lposi = lightPositions.PushBack();
				FSimpleLightPerViewEntry	&lightpos = lightPositions[lposi];
				lightpos.Position = ToUE(positions[parti] * globalScale);

				PopcornFX::CGuid			ldatai = lightDatas.PushBack();
				FSimpleLightEntry			&lightdata = lightDatas[ldatai];
				lightdata.Color = ToUE(colors[parti] * kColorMultiplier);
				lightdata.Radius = radius;

				// Set the exponent to 0 if we want to enable inverse squared falloff
				// Note:
				//		- Radius will now represent the falloff's clamped area
				//		- Color will now represent the physical light's lumens
				lightdata.Exponent = kExponent;

				lightdata.VolumetricScatteringIntensity = volumetricScatteringIntensity[parti];

				lightdata.bAffectTranslucency = lightsTranslucentGeometry;
			}
		}
	}
}

//----------------------------------------------------------------------------
//
// Sounds
//
//----------------------------------------------------------------------------

namespace
{
	bool		_EnableAudio(UWorld *world)
	{
		if (GEngine == null || !GEngine->UseSound())
			return false;
		if (world == null)
			return false;
#if (ENGINE_MINOR_VERSION >= 25)
		if (!world->GetAudioDevice().IsValid())
#else
		if (world->GetAudioDevice() == null)
#endif // (ENGINE_MINOR_VERSION >= 25)
			return false;
		if (world->WorldType != EWorldType::Game &&
			world->WorldType != EWorldType::PIE &&
			world->WorldType != EWorldType::Editor)
			return false;
		return true;
	}
} // namespace

//----------------------------------------------------------------------------

bool	CRenderBatchPolicy::LaunchCustomTasks(const SRenderContext &renderContext, const TMemoryView<const PopcornFX::Drawers::SSound_DrawRequest * const> &drawRequests, PopcornFX::Drawers::CBillboard_CPU *emptyBatch)
{
	PK_ASSERT(emptyBatch == null);
	return true;
}

//----------------------------------------------------------------------------

void	CRenderBatchPolicy::_IssueDrawCall_Sound(const SRenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Sound");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);
	const float				globalScale = view->GlobalScale();

	PK_ASSERT(!desc.m_DrawRequests.Empty());
	PK_ASSERT(desc.m_TotalParticleCount > 0);
	const u32		totalParticleCount = desc.m_TotalParticleCount;
	PK_ASSERT(totalParticleCount == m_TotalParticleCount);

	PK_ASSERT(view->Pass() == CRendererSubView::UpdatePass_PostUpdate);
	PK_ASSERT(IsInGameThread());

	UWorld	*world = renderContext.GetWorld();
	if (!_EnableAudio(world))
		return;

	PK_ASSERT(desc.m_DrawRequests.Count() == 1);

	PK_ASSERT(world->WorldType == EWorldType::PIE ||
		world->WorldType == EWorldType::Game ||
		world->WorldType == EWorldType::Editor);

	CRendererCache	*matCache = static_cast<CRendererCache*>(desc.m_RendererCaches.First().Get());
	if (!PK_VERIFY(matCache != null))
		return;
	CMaterialDesc_GameThread		&matDesc = matCache->GameThread_Desc();
	CSoundDescriptorPoolCollection	*sPoolCollection = matDesc.m_SoundPoolCollection;

	if (!PK_VERIFY(sPoolCollection != null))
		return;

	sPoolCollection->BeginInsert(world);

	const u32		soundPoolCount = sPoolCollection->m_Pools.Count();
	const u32		maxSoundPoolID = soundPoolCount - 1;
	const float		maxSoundPoolsFp = maxSoundPoolID;

	for (u32 iDr = 0; iDr < desc.m_DrawRequests.Count(); ++iDr)
	{
		const PopcornFX::Drawers::SSound_DrawRequest			&drawRequest = *static_cast<const PopcornFX::Drawers::SSound_DrawRequest*>(desc.m_DrawRequests[iDr]);
		const PopcornFX::Drawers::SSound_BillboardingRequest	&bbRequest = static_cast<const PopcornFX::Drawers::SSound_BillboardingRequest&>(drawRequest.BaseBillboardingRequest());
		PK_ASSERT(drawRequest.Valid());
		PK_ASSERT(drawRequest.RenderedParticleCount() > 0);

		const float		dopplerFactor = bbRequest.m_DopplerFactor;
		const float		isBlended = false; // TODO: Implement sound atlas

		if (drawRequest.StorageClass() != PopcornFX::CParticleStorageManager_MainMemory::DefaultStorageClass())
		{
			PK_ASSERT_NOT_REACHED();
			return;
		}

		const PopcornFX::CParticleStreamToRender_MainMemory	*lockedStream = drawRequest.StreamToRender_MainMemory();
		if (!PK_VERIFY(lockedStream != null)) // Light particles shouldn't handle GPU streams for now
			return;

		const u32	pageCount = lockedStream->PageCount();
		for (u32 iPage = 0; iPage < pageCount; ++iPage)
		{
			const PopcornFX::CParticlePageToRender_MainMemory	*page = lockedStream->Page(iPage);
			PK_ASSERT(page != null && !page->Empty());

			const u32	particleCount = page->InputParticleCount();

			PopcornFX::TStridedMemoryView<const float>		lifeRatios = page->StreamForReading<float>(bbRequest.m_LifeRatioStreamId);
			PopcornFX::TStridedMemoryView<const float>		invLives = page->StreamForReading<float>(bbRequest.m_InvLifeStreamId);
			PopcornFX::TStridedMemoryView<const CInt2>		selfIDs = page->StreamForReading<CInt2>(bbRequest.m_SelfIDStreamId);
			PopcornFX::TStridedMemoryView<const CFloat3>	positions = page->StreamForReading<CFloat3>(bbRequest.m_PositionStreamId);

			PopcornFX::TStridedMemoryView<const CFloat3>	velocities = bbRequest.m_VelocityStreamId.Valid() ? page->StreamForReading<CFloat3>(bbRequest.m_VelocityStreamId) : PopcornFX::TStridedMemoryView<const CFloat3>(&CFloat4::ZERO.xyz(), positions.Count(), 0);
//			PopcornFX::TStridedMemoryView<const float>		soundIDs = bbRequest.m_SoundIDStreamId.Valid() ? page->StreamForReading<float>(bbRequest.m_SoundIDStreamId) : PopcornFX::TStridedMemoryView<const float>(&CFloat4::ZERO.x(), positions.Count(), 0);
			PopcornFX::TStridedMemoryView<const float>		volumes = bbRequest.m_VolumeStreamId.Valid() ? page->StreamForReading<float>(bbRequest.m_VolumeStreamId) : PopcornFX::TStridedMemoryView<const float>(&CFloat4::ONE.x(), positions.Count(), 0);
			PopcornFX::TStridedMemoryView<const float>		radii = bbRequest.m_RangeStreamId.Valid() ? page->StreamForReading<float>(bbRequest.m_RangeStreamId) : PopcornFX::TStridedMemoryView<const float>(&CFloat4::ONE.x(), positions.Count(), 0);

			const u8										enabledTrue = u8(-1);
			TStridedMemoryView<const u8>					enabledParticles = (bbRequest.m_EnabledStreamId.Valid()) ? page->StreamForReading<bool>(bbRequest.m_EnabledStreamId) : TStridedMemoryView<const u8>(&enabledTrue, particleCount, 0);

			const u32	posCount = positions.Count();
			PK_ASSERT(
				posCount == particleCount &&
				posCount == lifeRatios.Count() &&
				posCount == invLives.Count() &&
				posCount == selfIDs.Count() &&
				posCount == velocities.Count() &&
				//posCount == soundIDs.Count() &&
				posCount == volumes.Count() &&
				posCount == radii.Count());

			for (u32 iParticle = 0; iParticle < posCount; ++iParticle)
			{
				if (!enabledParticles[iParticle])
					continue;

				const float		volume = volumes[iParticle];

				const FVector	pos = ToUE(positions[iParticle] * globalScale);
				const float		radius = radii[iParticle] * globalScale;

#if (PLATFORM_PS4)
				const bool		audible = true; // Since 4.19.1/2 ....
#else
				const bool		audible = world->GetAudioDevice()->LocationIsAudible(pos, radius + radius * 0.5f);
#endif // (PLATFORM_PS4)

				const float		soundId = 0;//PopcornFX::PKMin(soundIDs[iParticle], maxSoundPoolsFp);
				const float		soundIdFrac = PopcornFX::PKFrac(soundId) * isBlended;
				const u32		soundId0 = u32(soundId);
				const u32		soundId1 = PopcornFX::PKMin(soundId0 + 1, maxSoundPoolID);

				SSoundInsertDesc	sDesc;
				sDesc.m_SelfID = selfIDs[iParticle];
				sDesc.m_Position = pos;
				sDesc.m_Velocity = ToUE(velocities[iParticle] * globalScale);
				sDesc.m_Radius = radius;
				sDesc.m_DopplerLevel = dopplerFactor;
				sDesc.m_Age = lifeRatios[iParticle] / invLives[iParticle];
				sDesc.m_Audible = audible;

				sDesc.m_Volume = volume * (1.0f - soundIdFrac);
				sPoolCollection->m_Pools[soundId0].InsertSoundIFP(sDesc);

				if (!isBlended)
					continue;

				sDesc.m_Volume = volume * soundIdFrac;
				sPoolCollection->m_Pools[soundId1].InsertSoundIFP(sDesc);
			}
		}
	}

	sPoolCollection->EndInsert(world);
}

//----------------------------------------------------------------------------
