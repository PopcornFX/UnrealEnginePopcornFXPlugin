//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_mediums.h>

#include "SceneManagement.h"
#include "SceneView.h"

#if RHI_RAYTRACING
#	include "RayTracingInstance.h"
#endif // RHI_RAYTRACING

class	FPopcornFXVertexFactory;
class	FPrimitiveDrawInterface;
class	FPopcornFXSceneProxy;
class	FSceneView;
class	FSceneViewFamily;
class	FMeshElementCollector;
class	CParticleScene;
class	FRayTracingMeshResourceCollector;
class	FScene;

FWD_PK_API_BEGIN
//----------------------------------------------------------------------------

// Fast Forwarded through PopcornFX (in v2, there is no need for that anymore)
class	CRendererSubView
{
public:
	enum : uint32 { kMaxViews = 8 };

	enum EPass
	{
		Pass_Unknown = 0,
		UpdatePass_PostUpdate,
		RenderPass_RT_AccelStructs,
		RenderPass_Main,
		RenderPass_Shadow,
	};

	struct SBBView
	{
		CFloat4x4		_m_OriginalViewForCmp;
		CFloat4x4		m_BillboardingMatrix;
		u32				m_ViewsMask;
		u32				m_ViewIndex;

		void			Setup(const CFloat4x4 &viewMatrix, u32 viewIndex = 0);
	};

	struct SSceneView
	{
		bool				m_ToRender;
		u32					m_BBViewIndex;
		const FSceneView	*m_SceneView;
	};

public:
	CRendererSubView();

	bool			Setup_GetDynamicMeshElements(
		const FPopcornFXSceneProxy *sceneProxy,
		const ::TArray<const FSceneView*> &Views,
		const FSceneViewFamily &ViewFamily,
		uint32 VisibilityMap,
		FMeshElementCollector &collector);

#if RHI_RAYTRACING
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	bool			Setup_GetDynamicRayTracingInstances(
		const FPopcornFXSceneProxy *sceneProxy,
		FRayTracingInstanceCollector &context);
#else
	bool			Setup_GetDynamicRayTracingInstances(
		const FPopcornFXSceneProxy *sceneProxy,
		FRayTracingMaterialGatheringContext &context,
		::TArray<FRayTracingInstance> &outRayTracingInstances);
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
#endif //RHI_RAYTRACING

	bool						Setup_PostUpdate();

	float						GlobalScale() const { return m_GlobalScale; }

	bool						IsRenderPass() const { return m_Pass >= RenderPass_RT_AccelStructs; }
	bool						IsUpdatePass() const { return m_Pass < RenderPass_RT_AccelStructs; }

	EPass						Pass() const { return m_Pass; }
	EPass						RenderPass() const { PK_ASSERT(IsRenderPass()); return m_Pass; }

	const FPopcornFXSceneProxy	*SceneProxy() const { return m_SceneProxy; }
	const FSceneViewFamily		*ViewFamily() const { return m_ViewFamily; }
	FMeshElementCollector		*Collector() const { return m_Collector; }

#if RHI_RAYTRACING
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	FRayTracingInstanceCollector						*RTCollector() const { return m_RTCollector; }
#else
	const FScene										*Scene() const { return m_Scene; }
	FRayTracingMeshResourceCollector					*RTCollector() const { return m_RTCollector; }
	::TArray<FRayTracingDynamicGeometryUpdateParams>	*DynamicRayTracingGeometries() { return m_DynamicRayTracingGeometries; }
	::TArray<FRayTracingInstance>						*OutRayTracingInstances() { return m_OutRayTracingInstances; }
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
#endif // RHI_RAYTRACING

	const TStaticCountedArray<SSceneView, kMaxViews>	&SceneViews() const { return m_SceneViews; }
	const TStaticCountedArray<SBBView, kMaxViews>		&BBViews() const { return m_BBViews; }
	SBBView												&BBView(u32 i) { return m_BBViews[i]; }

private:
	float								m_GlobalScale = 1.0f;
	const FPopcornFXSceneProxy			*m_SceneProxy = null;

	const FSceneViewFamily				*m_ViewFamily = null;
	FMeshElementCollector				*m_Collector = null;
#if RHI_RAYTRACING
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	FRayTracingInstanceCollector						*m_RTCollector = null;
#else
	const FScene										*m_Scene = null;
	FRayTracingMeshResourceCollector					*m_RTCollector = null;
	::TArray<FRayTracingDynamicGeometryUpdateParams>	*m_DynamicRayTracingGeometries = null;
	::TArray<FRayTracingInstance>						*m_OutRayTracingInstances = null;
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
#endif // RHI_RAYTRACING

	u32									m_ViewsMask = 0;

	EPass								m_Pass = Pass_Unknown;
	bool								m_HasShadowPass = false; // Deduced from last frame, so m_HasShadowPass is one frame late
	bool								_m_Next_HasShadowPass = false;

	// m_SceneViews.Count() can be > m_BBViews if some views are not visible (ie. VR). SBBView::m_ViewIndex points to the correct SSceneView index.
	TStaticCountedArray<SSceneView, kMaxViews>		m_SceneViews;
	TStaticCountedArray<SBBView, kMaxViews>			m_BBViews;

};

//----------------------------------------------------------------------------
FWD_PK_API_END
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

using PopcornFX::CRendererSubView;
