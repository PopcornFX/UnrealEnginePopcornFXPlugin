//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "Render/RendererSubView.h"
#include "PopcornFXSceneComponent.h"

#include "PrimitiveSceneProxy.h"

class	FParticleAttributes;

class	FPopcornFXSceneProxy : public FPrimitiveSceneProxy
{
public:
	FPopcornFXSceneProxy(UPopcornFXSceneComponent *component);
	~FPopcornFXSceneProxy();

	virtual void					GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

#if RHI_RAYTRACING
	virtual void					GetDynamicRayTracingInstances(FRayTracingMaterialGatheringContext& Context, TArray<FRayTracingInstance>& OutRayTracingInstances) override;
	virtual bool					IsRayTracingRelevant() const override { return true; }
#endif // RHI_RAYTRACING

	virtual SIZE_T					GetTypeHash() const override;

	virtual void					GatherSimpleLights(const FSceneViewFamily& ViewFamily, FSimpleLightArray& OutParticleLights) const override;
	virtual uint32					GetMemoryFootprint() const;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

	virtual void					GetLightRelevance(const FLightSceneProxy * LightSceneProxy, bool & bDynamic, bool & bRelevant, bool & bLightMapped, bool & bShadowMapped) const override;

	virtual const FBoxSphereBounds	&GetBounds() const { check(m_SceneComponent != null); return m_SceneComponent->Bounds; }

	CParticleScene					*ParticleScene() { return m_SceneComponent != null ? m_SceneComponent->ParticleScene() : null; }
	CParticleScene					*ParticleSceneToRender() { return m_SceneComponent != null ? m_SceneComponent->ParticleSceneToRender() : null; }
	CParticleScene					*ParticleSceneToRender() const { return m_SceneComponent != null ? m_SceneComponent->ParticleSceneToRender() : null; }

private:
	UPopcornFXSceneComponent		*m_SceneComponent;

};
