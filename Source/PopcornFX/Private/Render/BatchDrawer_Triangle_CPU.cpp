//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "BatchDrawer_Triangle_CPU.h"
#include "RenderBatchManager.h"
#include "SceneManagement.h"
#include "MaterialDesc.h"
#include "SceneInterface.h"

#include "Engine/Engine.h"
#include "World/PopcornFXSceneProxy.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "Render/PopcornFXVertexFactory.h"
#include "Render/PopcornFXVertexFactoryCommon.h"
#include "PopcornFXStats.h"

#include <pk_render_helpers/include/render_features/rh_features_basic.h>
#include <pk_render_helpers/include/render_features/rh_features_vat_static.h>

//----------------------------------------------------------------------------

void	CBatchDrawer_Triangle_CPUBB::SViewDependent::UnmapBuffers()
{
	m_Indices.Unmap();
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Triangle_CPUBB::SViewDependent::ClearBuffers()
{
	m_Indices.UnmapAndClear();
}

//----------------------------------------------------------------------------

CBatchDrawer_Triangle_CPUBB::CBatchDrawer_Triangle_CPUBB()
{
}

//----------------------------------------------------------------------------

CBatchDrawer_Triangle_CPUBB::~CBatchDrawer_Triangle_CPUBB()
{
	// Render resources cannot be released on the game thread.
	// When re-importing an effect, render medium gets destroyed on the main thread (expected scenario)
	// When unloading a scene, render mediums are destroyed on the render thread (RenderBatchManager.cpp::Clean())
	if (IsInRenderingThread() || IsInRHIThread())
	{
		_ClearBuffers();
	}
	else
	{
		ENQUEUE_RENDER_COMMAND(ReleaseRenderResourcesCommand)(
			[this](FRHICommandListImmediate &RHICmdList)
			{
				_ClearBuffers();
			});

		FlushRenderingCommands(); // so we can safely release frames
	}

	PK_ASSERT(!m_Indices.Valid());
	PK_ASSERT(!m_Positions.Valid());
	PK_ASSERT(!m_Normals.Valid());
	PK_ASSERT(!m_Tangents.Valid());
	PK_ASSERT(!m_Texcoords.Valid());

	PK_ASSERT(!m_SimData.Valid());

	for (u32 iView = 0; iView < m_ViewDependents.Count(); ++iView)
		PK_ASSERT(!m_ViewDependents[iView].m_Indices.Valid());
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Triangle_CPUBB::Setup(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleRenderMedium *owner, const PopcornFX::CFrameCollector *fc, const PopcornFX::CStringId &storageClass)
{
	if (!Super::Setup(renderer, owner, fc, storageClass))
		return false;

	m_NeedsBTN = renderer->m_RendererCache->m_Flags.m_HasNormal;
	m_SecondUVSet = renderer->m_RendererCache->m_Flags.m_HasAtlasBlending && renderer->m_RendererCache->m_Flags.m_HasUV;
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Triangle_CPUBB::AreRenderersCompatible(const PopcornFX::CRendererDataBase *rendererA, const PopcornFX::CRendererDataBase *rendererB) const
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

	PK_ASSERT(rDesc0.m_RendererClass == PopcornFX::Renderer_Triangle);
	const FPopcornFXSubRendererMaterial	*mat0 = rDesc0.m_RendererMaterial->GetSubMaterial(0);
	const FPopcornFXSubRendererMaterial	*mat1 = rDesc1.m_RendererMaterial->GetSubMaterial(0);
	if (mat0 == null || mat1 == null)
		return false;
	if (mat0 == mat1 ||
		mat0->RenderThread_SameMaterial_Billboard(*mat1))
		return true;
	return false;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Triangle_CPUBB::CanRender(PopcornFX::SRenderContext &ctx) const
{
	PK_ASSERT(DrawPass().m_RendererCaches.First() != null);

	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);
	CMaterialDesc_RenderThread	&matDesc = static_cast<CRendererCache*>(DrawPass().m_RendererCaches.First().Get())->RenderThread_Desc();
	PK_ASSERT(renderContext.m_RendererSubView != null);

	return renderContext.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Main ||
		(renderContext.m_RendererSubView->Pass() == CRendererSubView::RenderPass_Shadow && matDesc.m_CastShadows);
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Triangle_CPUBB::BeginFrame(PopcornFX::SRenderContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Triangle_CPUBB::BeginFrame");

	Super::BeginFrame(ctx);

	INC_DWORD_STAT_BY(STAT_PopcornFX_BatchesCount, 1);

	_ClearBuffers();
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Triangle_CPUBB::_IsAdditionalInputSupported(const PopcornFX::CStringId &fieldName, PopcornFX::EBaseTypeID type, EPopcornFXAdditionalStreamOffsets &outStreamOffsetType)
{
	if (type == PopcornFX::BaseType_Float4)
	{
		if (fieldName == PopcornFX::BasicRendererProperties::SID_Diffuse_Color() || // Legacy
			fieldName == PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseColor() ||
			fieldName == PopcornFX::BasicRendererProperties::SID_Distortion_Color())
			outStreamOffsetType = StreamOffset_Colors;
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
	}
	return outStreamOffsetType != EPopcornFXAdditionalStreamOffsets::__SupportedAdditionalStreamCount;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Triangle_CPUBB::AllocBuffers(PopcornFX::SRenderContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Triangle_CPUBB::AllocBuffers");
	const SUERenderContext		&renderContext = static_cast<SUERenderContext&>(ctx);
	
	const PopcornFX::SRendererBatchDrawPass &drawPass = DrawPass();

	const bool	resizeBuffers = m_TotalParticleCount != drawPass.m_TotalParticleCount;
	m_TotalParticleCount = drawPass.m_TotalParticleCount;

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
	const u32	totalVertexCount = m_TotalParticleCount * 3;
	const u32	totalIndexCount = totalVertexCount;

	// View independent
	const PopcornFX::SGeneratedInputs	&toGenerate = drawPass.m_ToGenerate;

	const bool	needsIndices = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Indices) != 0;
	const bool	needsPositions = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Position) != 0;
	const bool	needsNormals = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Normal) != 0;
	const bool	needsTangents = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Tangent) != 0;
	PK_ASSERT(needsNormals == needsTangents);

	// Billboard View independent
	const bool	needsUV0 = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UV0) != 0;
	const bool	needsUV1 = (toGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UV1) != 0;

	bool	largeIndices = false;

	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Triangle_CPUBB::AllocBuffers_ViewIndependent");

		if (!mainIBPool->AllocateIf(needsIndices, m_Indices, largeIndices, totalIndexCount, false))
			return false;
		if (!mainVBPool->AllocateIf(needsPositions, m_Positions, totalVertexCount, sizeof(CFloat4), false))
			return false;
		if (!mainVBPool->AllocateIf(needsNormals, m_Normals, totalVertexCount, sizeof(CFloat4), false))
			return false;
		if (!mainVBPool->AllocateIf(needsTangents, m_Tangents, totalVertexCount, sizeof(CFloat4), false))
			return false;
		if (!mainVBPool->AllocateIf(needsUV0, m_Texcoords, totalVertexCount, sizeof(CFloat2), false))
			return false;
	}

	if (drawPass.m_IsNewFrame ||
		resizeBuffers) // m_TotalParticleCount can differ between passes, realloc buffers if that's the case
	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Triangle_CPUBB::AllocBuffers_AdditionalInputs");

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
		else
		{
			// Dummy allocate to fix crash when the only additional input is unsupported.
			if (!particleDataVBPool->Allocate(m_SimData, 1, sizeof(float), false))
				return false;
		}
	}

	const u32	activeViewCount = toGenerate.m_PerViewGeneratedInputs.Count();
	if (!PK_VERIFY(m_ViewDependents.Resize(activeViewCount)))
		return false;
	if (activeViewCount > 0)
	{
		PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Triangle_CPUBB::AllocBuffers_ViewDependents");

		// View deps
		for (u32 iView = 0; iView < activeViewCount; ++iView)
		{
			const u32	inputFlags = toGenerate.m_PerViewGeneratedInputs[iView].m_GeneratedInputs;
			if (inputFlags == 0)
				continue;

			const bool		viewNeedsIndices = (inputFlags & PopcornFX::Drawers::GenInput_Indices) != 0;
			SViewDependent	&viewDep = m_ViewDependents[iView];

			viewDep.m_ViewIndex = toGenerate.m_PerViewGeneratedInputs[iView].m_ViewIndex;
			if (!mainIBPool->AllocateIf(viewNeedsIndices, viewDep.m_Indices, largeIndices, totalIndexCount, false))
				return false;
		}
	}
	else
	{
		PK_ASSERT(drawPass.m_Views.Count() == 0 || !drawPass.m_IsNewFrame || needsIndices);
	}
	return true;
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Triangle_CPUBB::UnmapBuffers(PopcornFX::SRenderContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Triangle_CPUBB::UnmapBuffers");

	const PopcornFX::SRendererBatchDrawPass &drawPass = DrawPass();

	m_Indices.Unmap();
	m_Positions.Unmap();
	m_Normals.Unmap();
	m_Tangents.Unmap();
	m_Texcoords.Unmap();

	m_SimData.Unmap();

	for (u32 iView = 0; iView < m_ViewDependents.Count(); ++iView)
		m_ViewDependents[iView].UnmapBuffers();

	return true;
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Triangle_CPUBB::_ClearBuffers()
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Triangle_CPUBB::_ClearBuffers");

	m_FeatureLevel = ERHIFeatureLevel::Num;

	m_Indices.UnmapAndClear();
	m_Positions.UnmapAndClear();
	m_Normals.UnmapAndClear();
	m_Tangents.UnmapAndClear();
	m_Texcoords.UnmapAndClear();

	m_SimData.UnmapAndClear();

	for (u32 iView = 0; iView < m_ViewDependents.Count(); ++iView)
		m_ViewDependents[iView].ClearBuffers();
}

//----------------------------------------------------------------------------

void	CBatchDrawer_Triangle_CPUBB::_ClearStreamOffsets()
{
	for (u32 iStream = 0; iStream < __SupportedAdditionalStreamCount; ++iStream)
		m_AdditionalStreamOffsets[iStream].Reset();
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Triangle_CPUBB::MapBuffers(PopcornFX::SRenderContext &ctx)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Triangle_CPUBB::MapBuffers_Billboard");

	const PopcornFX::SRendererBatchDrawPass &drawPass = DrawPass();

	const u32	totalParticleCount = m_TotalParticleCount;
	const u32	totalVertexCount = totalParticleCount * 3;
	const u32	totalIndexCount = totalVertexCount;

	// View independent
	if (drawPass.m_ToGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Indices)
	{
		PK_ASSERT(m_Indices.Valid());
		void	*indices = null;
		bool	largeIndices = false;

		if (!m_Indices->Map(indices, largeIndices, totalIndexCount))
			return false;
		if (!m_BBJobs_Triangle.m_Exec_Indices.m_IndexStream.Setup(indices, totalIndexCount, largeIndices))
			return false;
	}
	if (drawPass.m_ToGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Position)
	{
		PK_ASSERT(m_Positions.Valid());
		TStridedMemoryView<CFloat3, 0x10>	positions(null, totalVertexCount, 0x10);
		if (!m_Positions->Map(positions))
			return false;
		m_BBJobs_Triangle.m_Exec_PNT.m_Positions = positions;
	}
	if (drawPass.m_ToGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Normal)
	{
		PK_ASSERT(m_Normals.Valid());
		TStridedMemoryView<CFloat3, 0x10>	normals(null, totalVertexCount, 0x10);
		if (!m_Normals->Map(normals))
			return false;
		m_BBJobs_Triangle.m_Exec_PNT.m_Normals = normals;
	}
	if (drawPass.m_ToGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_Tangent)
	{
		PK_ASSERT(m_Tangents.Valid());
		TStridedMemoryView<CFloat4, 0x10>	tangents(null, totalVertexCount, 0x10);
		if (!m_Tangents->Map(tangents))
			return false;
		m_BBJobs_Triangle.m_Exec_PNT.m_Tangents = tangents;
	}
	if (drawPass.m_ToGenerate.m_GeneratedInputs & PopcornFX::Drawers::GenInput_UV0)
	{
		PK_ASSERT(m_Texcoords.Valid());
		TStridedMemoryView<CFloat2>	uv0s(null, totalVertexCount);
		if (!m_Texcoords->Map(uv0s))
			return false;
		m_BBJobs_Triangle.m_Exec_PNT.m_Texcoords = uv0s;
	}
	if (!drawPass.m_ToGenerate.m_AdditionalGeneratedInputs.Empty())
	{
		PK_ASSERT(m_SimData.Valid());
		PopcornFX::TMemoryView<float>	simData;
		{
			PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Triangle_CPUBB::MapBuffers_Triangle (particle data buffer)");
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

		m_BBJobs_Triangle.m_Exec_CopyField.m_FieldsToCopy = m_MappedAdditionalInputs.View();
		m_BBJobs_Triangle.m_Exec_CopyField.m_PerVertex = false;
	}

	// View deps
	const u32	activeViewCount = m_ViewDependents.Count();
	PK_ASSERT(activeViewCount == m_BBJobs_Triangle.m_PerView.Count());
	for (u32 iView = 0; iView < activeViewCount; ++iView)
	{
		const u32									viewGeneratedInputs = drawPass.m_ToGenerate.m_PerViewGeneratedInputs[iView].m_GeneratedInputs;
		SViewDependent								&viewDep = m_ViewDependents[iView];
		PopcornFX::STriangleBatchJobs::SPerView		&dstView = m_BBJobs_Triangle.m_PerView[iView];

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

void	CBatchDrawer_Triangle_CPUBB::_IssueDrawCall_Triangle(const SUERenderContext &renderContext, const PopcornFX::SDrawCallDesc &desc)
{
	PK_NAMEDSCOPEDPROFILE("CRenderBatchPolicy::IssueDrawCall_Triangle");

	CRendererSubView	*view = renderContext.m_RendererSubView;
	PK_ASSERT(view != null);

	FMeshElementCollector		*collector = view->Collector();
	const FPopcornFXSceneProxy	*sceneProxy = view->SceneProxy();
	PK_ASSERT(collector != null);
	PK_ASSERT(sceneProxy != null);

	const u32	indexCount = desc.m_TotalParticleCount * 3;
	const u32	indexOffset = desc.m_IndexOffset;

	PK_ASSERT(indexOffset + indexCount <= m_TotalParticleCount * 3);

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
	{
		localToWorld *= view->GlobalScale();
		previousLocalToWorld *= view->GlobalScale();
	}

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

			FPopcornFXBillboardCollector	*collectorRes = &(collector->AllocateOneFrameResource<FPopcornFXBillboardCollector>());
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

			FPopcornFXUniforms				vsUniforms;
			FPopcornFXBillboardVSUniforms		vsUniformsBillboard;
			FPopcornFXBillboardCommonUniforms	commonUniformsBillboard;

			vsUniforms.InSimData = m_SimData.Buffer()->SRV();
			vsUniforms.DynamicParameterMask = matDesc.m_DynamicParameterMask;

			vsUniformsBillboard.RendererType = static_cast<u32>(PopcornFX::Renderer_Triangle);
			vsUniformsBillboard.TotalParticleCount = desc.m_TotalParticleCount;
			vsUniformsBillboard.InColorsOffset = m_AdditionalStreamOffsets[StreamOffset_Colors].OffsetForShaderConstant();
			vsUniformsBillboard.InVelocityOffset = m_AdditionalStreamOffsets[StreamOffset_Velocity].OffsetForShaderConstant();
			vsUniformsBillboard.InEmissiveColorsOffset3 = m_AdditionalStreamOffsets[StreamOffset_EmissiveColors3].OffsetForShaderConstant();
			vsUniformsBillboard.InEmissiveColorsOffset4 = m_AdditionalStreamOffsets[StreamOffset_EmissiveColors4].OffsetForShaderConstant();
			vsUniformsBillboard.InAlphaCursorsOffset = m_AdditionalStreamOffsets[StreamOffset_AlphaCursors].OffsetForShaderConstant();
			vsUniformsBillboard.InDynamicParameter1sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam1s].OffsetForShaderConstant();
			vsUniformsBillboard.InDynamicParameter2sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam2s].OffsetForShaderConstant();
			vsUniformsBillboard.InDynamicParameter3sOffset = m_AdditionalStreamOffsets[StreamOffset_DynParam3s].OffsetForShaderConstant();

			commonUniformsBillboard.HasSecondUVSet = m_SecondUVSet;
			commonUniformsBillboard.FlipUVs = false;
			commonUniformsBillboard.NeedsBTN = m_NeedsBTN;
			commonUniformsBillboard.CorrectRibbonDeformation = false;

			vertexFactory->m_VSUniformBuffer = FPopcornFXUniformsRef::CreateUniformBufferImmediate(vsUniforms, UniformBuffer_SingleDraw);
			vertexFactory->m_BillboardVSUniformBuffer = FPopcornFXBillboardVSUniformsRef::CreateUniformBufferImmediate(vsUniformsBillboard, UniformBuffer_SingleDraw);
			vertexFactory->m_BillboardCommonUniformBuffer = FPopcornFXBillboardCommonUniformsRef::CreateUniformBufferImmediate(commonUniformsBillboard, UniformBuffer_SingleDraw);

			PK_ASSERT(!vertexFactory->IsInitialized());
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
			vertexFactory->InitResource(FRHICommandListExecutor::GetImmediateCommandList());
#else
			vertexFactory->InitResource();
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
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
		meshBatch.MaterialRenderProxy = matDesc.RenderProxy()[0];

		FMeshBatchElement	&meshElement = meshBatch.Elements[0];

		meshElement.IndexBuffer = viewIndependentIndices ? m_Indices.Buffer() : viewDep->m_Indices.Buffer();
		meshElement.FirstIndex = indexOffset;
		meshElement.NumPrimitives = indexCount / 3;
		meshElement.MinVertexIndex = 0;
		meshElement.MaxVertexIndex = (m_TotalParticleCount * 3) - 1;
		FDynamicPrimitiveUniformBuffer	&dynamicPrimitiveUniformBuffer = collector->AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 4)
		dynamicPrimitiveUniformBuffer.Set(FRHICommandListExecutor::GetImmediateCommandList(), localToWorld, previousLocalToWorld, bounds, localBounds, true, hasPrecomputedVolumetricLightmap, outputVelocity);
#elif (ENGINE_MAJOR_VERSION == 5)
		dynamicPrimitiveUniformBuffer.Set(localToWorld, previousLocalToWorld, bounds, localBounds, true, hasPrecomputedVolumetricLightmap, outputVelocity);
#else
		dynamicPrimitiveUniformBuffer.Set(localToWorld, previousLocalToWorld, bounds, localBounds, true, hasPrecomputedVolumetricLightmap, drawsVelocity, outputVelocity);
#endif // (ENGINE_MAJOR_VERSION == 5)
		meshElement.PrimitiveUniformBuffer = dynamicPrimitiveUniformBuffer.UniformBuffer.GetUniformBufferRHI();

		PK_ASSERT(meshElement.NumPrimitives > 0);

		{
			INC_DWORD_STAT_BY(STAT_PopcornFX_DrawCallsTriangleCount, 1);

			collector->AddMesh(realViewIndex, meshBatch);
		}
	}
}

//----------------------------------------------------------------------------

bool	CBatchDrawer_Triangle_CPUBB::EmitDrawCall(PopcornFX::SRenderContext &ctx, const PopcornFX::SDrawCallDesc &toEmit)
{
	PK_NAMEDSCOPEDPROFILE("CBatchDrawer_Triangle_CPUBB::EmitDrawCall");
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

	_IssueDrawCall_Triangle(renderContext, toEmit);
	return true;
}

//----------------------------------------------------------------------------
