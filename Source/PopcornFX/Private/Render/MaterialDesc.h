//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PopcornFXSDK.h"
#include "RHIDefinitions.h"
#include "Engine/SkeletalMesh.h"
#include <pk_particles/include/Renderers/ps_renderer_base.h>
#include <pk_render_helpers/include/frame_collector/rh_particle_render_data_factory.h>
#include <pk_render_helpers/include/draw_requests/rh_billboard.h>

#include "RenderTypesPolicies.h"

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 2)
#	if RHI_RAYTRACING
#		include "RayTracingGeometry.h"
#	endif
#endif

//----------------------------------------------------------------------------

class	UPopcornFXRendererMaterial;
class	UMaterialInterface;
struct	FStaticMeshLODResources;
struct	FStaticMeshVertexFactories;
class	FStaticMeshRenderData;
class	FSkeletalMeshRenderData;
struct	FSkeletalMeshLODInfo;
class	UStaticMesh;
class	USkeletalMesh;
class	FSkeletalMeshLODRenderData;
class	FMaterialRenderProxy;
class	CSoundDescriptorPoolCollection;

//----------------------------------------------------------------------------

class	CMaterialDesc_GameThread
{
public:
	CMaterialDesc_GameThread() { }
	virtual ~CMaterialDesc_GameThread();

	bool			GameThread_Setup();
	bool			MaterialIsValid() const;
	bool			HasMaterial() const;

	virtual void	Clear();

public:
	// Note: should be refactored, render thread should have no need to maintain a weak pointer to the uasset.
	TWeakObjectPtr<UPopcornFXRendererMaterial>	m_RendererMaterial;
	PopcornFX::ERendererClass					m_RendererClass;
	PopcornFX::PCRendererDataBase				m_Renderer = null;

	CSoundDescriptorPoolCollection		*m_SoundPoolCollection = null;

	u32									m_BaseLODLevel = 0;

	// Static meshes
	UStaticMesh							*m_StaticMesh = null;
	const FStaticMeshRenderData			*m_StaticMeshRenderData = null;

	// Skeletal meshes
	USkeletalMesh									*m_SkeletalMesh = null;
	const FSkeletalMeshRenderData					*m_SkeletalMeshRenderData = null;
	UTexture2D										*m_SkeletalAnimationTexture = null;
	u32												m_SkeletalAnimationCount = 0;
	FVector3f										m_SkeletalAnimationPosBoundsMin = FVector3f::ZeroVector;
	FVector3f										m_SkeletalAnimationPosBoundsMax = FVector3f::ZeroVector;
	u32												m_SkeletalAnimationLinearInterpolate = 0;
	u32												m_SkeletalAnimationLinearInterpolateTracks = 0;
	u32												m_TotalBoneCount = 0;
	PopcornFX::TMemoryView<const u32>				m_SkeletalMeshBoneIndicesReorder;

	u32												m_DynamicParameterMask = 0;

#if RHI_RAYTRACING
	::TArray<FRayTracingGeometry>		m_RayTracingGeometries;
#endif // RHI_RAYTRACING

	bool		m_NeedSort = false;
	bool		m_HasAtlasBlending = false;
	bool		m_HasAlphaRemapper = false;
	bool		m_IsLit = false;
	bool		m_LightsTranslucent = false;
	bool		m_HasMeshAtlas = false;
	bool		m_Raytraced = false;
	bool		m_CastShadows = false;
	bool		m_CorrectDeformation = false;
	bool		m_PerParticleLOD = false;
	bool		m_MotionBlur = false;

protected:
#if WITH_EDITOR
	void		OnSkelMeshChanged();
	void		OnMeshChanged();
	void		OnMeshPostBuild(UStaticMesh *staticMesh);
#endif // WITH_EDITOR

	void		_BuildStaticMesh();
	void		_BuildSkelMesh();
};

//----------------------------------------------------------------------------

class	CMaterialDesc_RenderThread : public CMaterialDesc_GameThread
{
public:
	virtual void		Clear() override;
	virtual bool		ValidForRendering() const;

	bool		SetupFromGame(const CMaterialDesc_GameThread &gameMat);
	bool		ResolveMaterial(PopcornFX::Drawers::EBillboardingLocation bbLocation);

	TArray<FMaterialRenderProxy*>		RenderProxy() const { return m_MaterialRenderProxy; }
	const TArray<UMaterialInterface*>	MaterialInterface() const { return m_MaterialInterface; }

private:
	// Stateless, resolved every frame
	TArray<UMaterialInterface*>			m_MaterialInterface;
	TArray<FMaterialRenderProxy*>		m_MaterialRenderProxy;
};

//----------------------------------------------------------------------------

class	CRendererCache : public PopcornFX::CRendererCacheBase
{
public:
	CRendererCache() { }
	virtual ~CRendererCache() { }

	bool			GameThread_ResolveRenderer(PopcornFX::PCRendererDataBase renderer, const PopcornFX::CParticleDescriptor *particleDesc);
	bool			GameThread_Setup() { return m_GameThreadDesc.GameThread_Setup(); }
	bool			RenderThread_Setup();

	void			BuildCacheInfos_StaticMesh();
	void			BuildCacheInfos_SkeletalMesh();

	virtual void	UpdateThread_BuildBillboardingFlags(const PopcornFX::PRendererDataBase &renderer) override;

	const CMaterialDesc_GameThread		&GameThread_Desc() const { return m_GameThreadDesc; }
	const CMaterialDesc_RenderThread	&RenderThread_Desc() const { return m_RenderThreadDesc; }
	CMaterialDesc_GameThread			&GameThread_Desc() { return m_GameThreadDesc; }
	CMaterialDesc_RenderThread			&RenderThread_Desc() { return m_RenderThreadDesc; }

public:
	CMaterialDesc_GameThread	m_GameThreadDesc;
	CMaterialDesc_RenderThread	m_RenderThreadDesc;
};
PK_DECLARE_REFPTRCLASS(RendererCache);

//----------------------------------------------------------------------------
