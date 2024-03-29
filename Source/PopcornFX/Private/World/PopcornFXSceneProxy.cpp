//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXSceneProxy.h"

#include "PopcornFXSceneComponent.h"
#include "Render/RendererSubView.h"
#include "Render/PopcornFXVertexFactoryShaderParameters.h"
#include "Internal/ParticleScene.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_mediums.h>

//----------------------------------------------------------------------------

FPopcornFXSceneProxy::FPopcornFXSceneProxy(UPopcornFXSceneComponent *component)
:	FPrimitiveSceneProxy(component)
{
	m_SceneComponent = component;

	bVerifyUsedMaterials = true;

#if (ENGINE_MAJOR_VERSION == 5)
	bAlwaysHasVelocity = true;
#endif // (ENGINE_MAJOR_VERSION == 5)
}

//----------------------------------------------------------------------------

FPopcornFXSceneProxy::~FPopcornFXSceneProxy()
{
}

//----------------------------------------------------------------------------

void	FPopcornFXSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	LLM_SCOPE(ELLMTag::Particles);
	CParticleScene		*particleScene = ParticleSceneToRender();
	if (particleScene == null)
		return;
	particleScene->GetDynamicMeshElements(this, Views, ViewFamily, VisibilityMap, Collector);

	if (ViewFamily.EngineShowFlags.Particles)
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
				if (HasCustomOcclusionBounds())
				{
					RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetCustomOcclusionBounds(), IsSelected());
				}
			}
		}
	}
}

//----------------------------------------------------------------------------

#if RHI_RAYTRACING
void	FPopcornFXSceneProxy::GetDynamicRayTracingInstances(FRayTracingMaterialGatheringContext &context, TArray<FRayTracingInstance> &outRayTracingInstances)
{
	CParticleScene	*particleScene = ParticleSceneToRender();
	if (particleScene == null)
		return;
	particleScene->GetDynamicRayTracingInstances(this, context, outRayTracingInstances);
}
#endif // RHI_RAYTRACING

//----------------------------------------------------------------------------

void	FPopcornFXSceneProxy::GatherSimpleLights(const FSceneViewFamily& ViewFamily, FSimpleLightArray& OutParticleLights) const
{
	LLM_SCOPE(ELLMTag::Particles);
	const CParticleScene		*particleScene = ParticleSceneToRender();
	if (particleScene == null)
		return;
	particleScene->GatherSimpleLights(ViewFamily, OutParticleLights);
}

//----------------------------------------------------------------------------

SIZE_T	FPopcornFXSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

//----------------------------------------------------------------------------

uint32	FPopcornFXSceneProxy::GetMemoryFootprint() const
{
	return sizeof(*this) + FPrimitiveSceneProxy::GetAllocatedSize();
}

//----------------------------------------------------------------------------

FPrimitiveViewRelevance	FPopcornFXSceneProxy::GetViewRelevance(const FSceneView* view) const
{
	FPrimitiveViewRelevance	viewRelevance;

	// The primitive has one or more distortion elements.
	viewRelevance.bDistortion = true;

	// The primitive is drawn.
	viewRelevance.bDrawRelevance = IsShown(view) && view->Family->EngineShowFlags.Particles;

	// The primitive's dynamic elements are rendered for the view.
	viewRelevance.bDynamicRelevance = true;

	// The primitive is drawn only in the editor and composited onto the scene after post processing using no depth testing.
	//viewRelevance.bEditorNoDepthTestPrimitiveRelevance

	// The primitive is drawn only in the editor and composited onto the scene after post processing.
	//viewRelevance.bEditorPrimitiveRelevance

	// The primitive should have GatherSimpleLights called on the proxy when gathering simple lights.
	viewRelevance.bHasSimpleLights = true;

	// Whether this primitive view relevance has been initialized this frame.
	//viewRelevance.bInitializedThisFrame = true;

	// The primitive has one or more masked elements.
	//viewRelevance.bMaskedRelevance

	// The primitive has one or more elements that have normal translucency.
	viewRelevance.bNormalTranslucency = true;

	// The primitive has one or more opaque or masked elements.
	viewRelevance.bOpaque = true;
	viewRelevance.bMasked = true;

	// The primitive should render to the custom depth pass.
	viewRelevance.bRenderCustomDepth = ShouldRenderCustomDepth();

	// The primitive should render to the main depth pass.
	viewRelevance.bRenderInMainPass = true;

	// The primitive has one or more elements that have SeparateTranslucency.
	viewRelevance.bSeparateTranslucency = true;

	// The primitive is casting a shadow.
	viewRelevance.bShadowRelevance = true;// IsShadowCast(view);

	// The primitive's static elements are rendered for the view.
	//viewRelevance.bStaticRelevance

	//viewRelevance.bVelocityRelevance = false;
	//viewRelevance.bUsesSceneColorCopy = true;
	//viewRelevance.bDisableOffscreenRendering = true;
	//viewRelevance.bUsesGlobalDistanceField = false;
	//viewRelevance.bUsesWorldPositionOffset = false;
	//viewRelevance.bDecal = true;
	//viewRelevance.bTranslucentSurfaceLighting = false;
	//viewRelevance.bUsesSceneDepth = true;
	viewRelevance.bHasVolumeMaterialDomain = true;

	viewRelevance.bTranslucentSelfShadow = true;
	viewRelevance.bVelocityRelevance = true;
	return viewRelevance;
}

//----------------------------------------------------------------------------

void	FPopcornFXSceneProxy::GetLightRelevance(const FLightSceneProxy * LightSceneProxy, bool & bDynamic, bool & bRelevant, bool & bLightMapped, bool & bShadowMapped) const
{
	bDynamic = true;
	bRelevant = true;
	bLightMapped = false;
	bShadowMapped = false;
}

//----------------------------------------------------------------------------
