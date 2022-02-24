//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PopcornFXSDK.h"
#include "RHIDefinitions.h"
#include <pk_particles/include/Renderers/ps_renderer_base.h>
#include <pk_render_helpers/include/frame_collector/rh_particle_render_data_factory.h>
#include <pk_render_helpers/include/draw_requests/rh_billboard.h>

#include "RenderTypesPolicies.h"

//----------------------------------------------------------------------------

class	UPopcornFXRendererMaterial;
class	UMaterialInterface;
struct	FStaticMeshLODResources;
struct	FStaticMeshVertexFactories;
class	UStaticMesh;
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
	const UPopcornFXRendererMaterial	*m_RendererMaterial = null;
	PopcornFX::ERendererClass			m_RendererClass;
	PopcornFX::PCRendererDataBase		m_Renderer = null;

	CSoundDescriptorPoolCollection		*m_SoundPoolCollection = null;

	FStaticMeshLODResources				*m_LODResources = null;
	FStaticMeshVertexFactories			*m_LODVertexFactories = null;
	UStaticMesh							*m_StaticMesh = null;

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

protected:
#if WITH_EDITOR
	void		OnMeshChanged();
	void		OnMeshPostBuild(UStaticMesh *staticMesh);
#endif // WITH_EDITOR
};

//----------------------------------------------------------------------------

class	CMaterialDesc_RenderThread : public CMaterialDesc_GameThread
{
public:
	virtual void		Clear() override;
	virtual bool		ValidForRendering() const;

	bool		SetupFromGame(const CMaterialDesc_GameThread &gameMat);
	bool		ResolveMaterial(PopcornFX::Drawers::EBillboardingLocation bbLocation);

	FMaterialRenderProxy		*RenderProxy() const { return m_MaterialRenderProxy; }
	const UMaterialInterface	*MaterialInterface() const { return m_MaterialInterface; }

private:
	// Stateless, resolved every frame
	UMaterialInterface			*m_MaterialInterface = null;
	FMaterialRenderProxy		*m_MaterialRenderProxy = null;
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
