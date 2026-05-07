//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "RendererSubView.h"

#include "PopcornFXPlugin.h"

#include "SceneView.h"

FWD_PK_API_BEGIN

//----------------------------------------------------------------------------
//
// CRendererSubView
//
//----------------------------------------------------------------------------

CRendererSubView::CRendererSubView()
:	m_Pass(Pass_Unknown)
,	m_HasShadowPass(false)
,	_m_Next_HasShadowPass(false)
{

}

//----------------------------------------------------------------------------

void	CRendererSubView::SBBView::Setup(const CFloat4x4 &viewMatrix, u32 viewIndex)
{
	_m_OriginalViewForCmp = viewMatrix;
	m_BillboardingMatrix = viewMatrix;
	m_BillboardingMatrix.Invert();
	m_BillboardingMatrix.StrippedZAxis() = -m_BillboardingMatrix.StrippedZAxis(); // because
	m_BillboardingMatrix.StrippedTranslations() *= FPopcornFXPlugin::GlobalScaleRcp();
	m_ViewsMask = 0;
	m_ViewIndex = viewIndex;
}

//----------------------------------------------------------------------------

#if RHI_RAYTRACING
bool	CRendererSubView::Setup_GetDynamicRayTracingInstances(
	const FPopcornFXSceneProxy *sceneProxy,
	FRayTracingMaterialGatheringContext &context,
	::TArray<FRayTracingInstance> &outRayTracingInstances)
{
	m_GlobalScale = FPopcornFXPlugin::GlobalScale();
	m_SceneProxy = sceneProxy;
	m_ViewFamily = &context.ReferenceViewFamily; // First view, Epic might change that later?
	m_Scene = context.Scene;
	m_RTCollector = &context.RayTracingMeshResourceCollector;
	m_DynamicRayTracingGeometries = &context.DynamicRayTracingGeometriesToUpdate;
	m_OutRayTracingInstances = &outRayTracingInstances;
	m_Pass = CRendererSubView::RenderPass_RT_AccelStructs;

	m_SceneViews.Clear();
	m_BBViews.Clear();

	if (!PK_VERIFY(m_SceneViews.PushBack().Valid()) ||
		!PK_VERIFY(m_BBViews.PushBack().Valid()))
		return false;

	SSceneView			&sceneView = m_SceneViews.Last();
	SBBView				&bbView = m_BBViews.Last();
	PK_ASSERT(context.ReferenceView != null);

	sceneView.m_ToRender = true;
	sceneView.m_BBViewIndex = 0;
	sceneView.m_SceneView = context.ReferenceView;

	const CFloat4x4		viewMatrix = ToPk(context.ReferenceView->ViewMatrices.GetViewMatrix());
	bbView.Setup(viewMatrix);
	bbView.m_ViewsMask |= (1U << 0);

	m_ViewsMask = bbView.m_ViewsMask;
	return m_ViewsMask != 0;
}
#endif

//----------------------------------------------------------------------------

bool	CRendererSubView::Setup_GetDynamicMeshElements(
	const FPopcornFXSceneProxy *sceneProxy,
	const ::TArray<const FSceneView*>& Views,
	const FSceneViewFamily& ViewFamily,
	uint32 VisibilityMap,
	FMeshElementCollector &collector)
{
	PK_ASSERT(m_Pass == Pass_Unknown || IsRenderPass()); // cannot be used both for update and render

	bool		somethingBigChanged = false;

	somethingBigChanged |= (m_ViewFamily != &ViewFamily);
	somethingBigChanged |= (m_SceneProxy != sceneProxy);

	m_GlobalScale = FPopcornFXPlugin::GlobalScale();
	m_SceneProxy = sceneProxy;
	m_ViewFamily = &ViewFamily;
	m_Collector = &collector;

	if (somethingBigChanged)
	{
		m_Pass = CRendererSubView::RenderPass_Main;
	}
	// !!! HACK !!! to detect shadow pass
	else if (VisibilityMap == 0x1 && Views[0]->GetDynamicMeshElementsShadowCullFrustum() != null)
	{
		// Nothing much we can do to detect if the current view is a shadow render pass
		// If GetDynamicMeshElementsShadowCullFrustum isn't null, this will be the frustum to cull against
		// And it means we are in a shadow render pass

		m_Pass = CRendererSubView::RenderPass_Shadow;
		//m_Pass = CRendererSubView::RenderPass_Main;
	}
	else
	{
		// should work for shawdow pass too, but will be much heavier
		m_Pass = CRendererSubView::RenderPass_Main;
	}

	const bool		isNewFrame = (m_Pass == CRendererSubView::RenderPass_Main || m_Pass == CRendererSubView::RenderPass_RT_AccelStructs);
	if (isNewFrame)
	{
		m_HasShadowPass = _m_Next_HasShadowPass;
		_m_Next_HasShadowPass = false;
	}
	else
	{
		_m_Next_HasShadowPass |= (m_Pass == CRendererSubView::RenderPass_Shadow);
	}

	m_SceneViews.Clear();
	m_BBViews.Clear();

	PK_ASSERT(uint32(Views.Num()) < kMaxViews);

	const u32		sceneViewCount = PopcornFX::PKMin(uint32(Views.Num()), kMaxViews);
	if (sceneViewCount == 0)
		return false;

	u32				viewsMask = 0;

	m_SceneViews.Resize(sceneViewCount);

	for (uint32 sceneViewi = 0; sceneViewi < sceneViewCount; ++sceneViewi)
	{
		SSceneView			&outSceneView = m_SceneViews[sceneViewi];
		const FSceneView	*sceneView = Views[sceneViewi];

		outSceneView.m_ToRender = false;
		if ((VisibilityMap & (1 << sceneViewi)) == 0)
			continue;

		const CFloat4x4			viewMatrix = ToPk(sceneView->ViewMatrices.GetViewMatrix());

		PK_TODO("Optmization for VR !?");
		// For VR we could merge both views into a single BBView ?

		CGuid			bbViewi;
		for (u32 bbi = 0; bbi < m_BBViews.Count(); ++bbi)
		{
			const SBBView		&bbView = m_BBViews[bbi];
			if (viewMatrix.Equals(bbView._m_OriginalViewForCmp))
			{
				bbViewi = bbi;
				break;
			}
		}
		if (!bbViewi.Valid())
		{
			bbViewi = m_BBViews.PushBack();
			PK_ASSERT(bbViewi.Valid());
			SBBView			&bbView = m_BBViews[bbViewi];
			bbView.Setup(viewMatrix, sceneViewi);

			PK_FIXME("UE: Better fix directional lights");
			// Directional lights have a position at the origin for shadow passes,
			// this make view position aligned billboards buggee
			if (m_Pass == CRendererSubView::RenderPass_Shadow &&
				bbView.m_BillboardingMatrix.StrippedTranslations() == CFloat3::ZERO)
			{
				const float		fixDirectionalLightDistance = 1000000.f; // 1 km
				bbView.m_BillboardingMatrix.StrippedTranslations() = bbView.m_BillboardingMatrix.StrippedZAxis() * fixDirectionalLightDistance;
			}
		}
		SBBView			&bbView = m_BBViews[bbViewi];
		bbView.m_ViewsMask |= (1U << sceneViewi);

		viewsMask |= (1U << sceneViewi);

		outSceneView.m_ToRender = true;
		outSceneView.m_BBViewIndex = bbViewi;
		outSceneView.m_SceneView = sceneView;
	}

	m_ViewsMask = viewsMask;
	PK_ASSERT(viewsMask == VisibilityMap);

	return m_ViewsMask != 0;
}

//----------------------------------------------------------------------------

bool	CRendererSubView::Setup_PostUpdate()
{
	PK_ASSERT(m_Pass == Pass_Unknown || IsUpdatePass()); // cannot be used both for update and render

	m_GlobalScale = FPopcornFXPlugin::GlobalScale();
	m_SceneProxy = null;
	m_Pass = CRendererSubView::UpdatePass_PostUpdate;

	return true;
}

//----------------------------------------------------------------------------
FWD_PK_API_END
