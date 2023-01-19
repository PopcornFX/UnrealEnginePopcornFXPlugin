//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "MaterialDesc.h"
#include "Assets/PopcornFXEffect.h"
#include "Assets/PopcornFXRendererMaterial.h"
#include "PopcornFXMeshVertexFactory.h"
#include "PopcornFXSkeletalMeshVertexFactory.h"
#include "PopcornFXVertexFactory.h"
#include "PopcornFXGPUVertexFactory.h"
#include "AudioPools.h"
#include "PopcornFXPlugin.h"

#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialShared.h"

#include <pk_particles/include/Renderers/ps_renderer_sound.h>
#include <pk_particles/include/ps_effect.h>
#include <pk_particles/include/ps_event_map.h>
#include <pk_render_helpers/include/render_features/rh_features_basic.h>

//----------------------------------------------------------------------------

namespace
{
	UPopcornFXRendererMaterial	*ResolveCachedMaterial(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::CParticleDescriptor *particleDesc)
	{
		// Resolves the cached UPopcornFXRendererMaterial from a renderer pointer
		PK_NAMEDSCOPEDPROFILE("ResolveCachedMaterial");
	
		if (renderer == null || particleDesc == null)
			return null;
		PK_ASSERT(renderer->m_RendererType != PopcornFX::Renderer_Light &&
				  renderer->m_RendererType != PopcornFX::Renderer_Sound); // We could create renderer materials for sounds!
		const PopcornFX::CParticleEffect	*parentEffect = particleDesc->ParentEffect();
		if (parentEffect == null)
			return null;
	
		UPopcornFXFile		*file = FPopcornFXPlugin::Get().GetPopcornFXFile(parentEffect->File());
		check(file != null);
		UPopcornFXEffect	*effect = Cast<UPopcornFXEffect>(file);
		if (!PK_VERIFY(effect != null))
			return null;
	
		bool	srcRendererFound = false;
		u32		rIndex = 0;
		{
			// Quick resolve renderer index, find better way ?
			PK_ASSERT(IsInGameThread());
			const PopcornFX::PCEventConnectionMap	&ecMap = parentEffect->EventConnectionMap();
			if (!PK_VERIFY(ecMap != null))
				return null;
			const u32	layerCount = ecMap->m_LayerSlots.Count();
			for (u32 iLayer = 0; iLayer < layerCount && !srcRendererFound; ++iLayer)
			{
				const PopcornFX::PParticleDescriptor	&desc = ecMap->m_LayerSlots[iLayer].m_ParentDescriptor;
				if (!PK_VERIFY(desc != null))
					return null;
				PopcornFX::TMemoryView<const PopcornFX::PRendererDataBase>	renderers = desc->Renderers();
				const u32													rendererCount = renderers.Count();
				for (u32 iRenderer = 0; iRenderer < rendererCount; ++iRenderer)
				{
					const PopcornFX::CRendererDataBase	*rBase = renderers[iRenderer].Get();
					if (!PK_VERIFY(rBase != null))
						return null;
					if (rBase == renderer)
					{
						srcRendererFound = true;
						break;
					}
					++rIndex;
				}
			}
		}
		if (!srcRendererFound)
		{
			PK_ASSERT_MESSAGE(false, "Could not find src renderer %d in %s", rIndex, particleDesc->ParentEffect()->File()->Path().Data());
			return null;
		}
	
		for (int32 mati = 0; mati < effect->ParticleRendererMaterials.Num(); ++mati)
		{
			UPopcornFXRendererMaterial	*mat = effect->ParticleRendererMaterials[mati];
			if (mat->Contains(rIndex))
				return mat;
		}
		PK_ASSERT_MESSAGE(false, "Could not find the correct material for renderer %d in %s", rIndex, particleDesc->ParentEffect()->File()->Path().Data());
		return null;
	}
}

//----------------------------------------------------------------------------
//
//	Renderer cache
//
//----------------------------------------------------------------------------

bool	CRendererCache::GameThread_ResolveRenderer(PopcornFX::PCRendererDataBase renderer, const PopcornFX::CParticleDescriptor *particleDesc)
{
	m_GameThreadDesc.m_RendererClass = renderer->m_RendererType;
	m_GameThreadDesc.m_Renderer = renderer;

	if (m_GameThreadDesc.HasMaterial())
	{
		UPopcornFXRendererMaterial	*rendererMat = ResolveCachedMaterial(renderer.Get(), particleDesc);
		m_GameThreadDesc.m_RendererMaterial = rendererMat;
		return rendererMat != null;
	}
	return true; // Light, sound
}

//----------------------------------------------------------------------------

void	CRendererCache::BuildCacheInfos_SkeletalMesh()
{
	PK_ASSERT(m_GameThreadDesc.m_SkeletalMesh != null);
	PK_ASSERT(m_GameThreadDesc.m_RendererMaterial != null);
	const FSkeletalMeshRenderData	*renderData = m_GameThreadDesc.m_SkeletalMesh->GetResourceForRendering();
	FPopcornFXSubRendererMaterial	*rendererSubMat = m_GameThreadDesc.m_RendererMaterial->GetSubMaterial(0);
	PK_ASSERT(rendererSubMat != null);

	if (renderData != null)
	{
		m_GlobalMeshBounds = ToPk(FBox3f(m_GameThreadDesc.m_SkeletalMesh->GetBounds().GetBox())) * FPopcornFXPlugin::GlobalScaleRcp();

		const u32		LODCount = rendererSubMat->PerParticleLOD ? renderData->LODRenderData.Num() - m_GameThreadDesc.m_BaseLODLevel : 1;
		PK_VERIFY(m_PerLODMeshCount.Resize(LODCount));
		for (u32 i = 0; i < m_PerLODMeshCount.Count(); ++i)
		{
			m_PerLODMeshCount[i] = renderData->LODRenderData[m_GameThreadDesc.m_BaseLODLevel + i].RenderSections.Num();

			// WIP
			const u32	oldCount = m_SubMeshBounds.Count();
			PK_VERIFY(m_SubMeshBounds.Resize(oldCount + m_PerLODMeshCount[i]));
			for (u32 iSection = 0; iSection < m_PerLODMeshCount[i]; ++iSection)
				m_SubMeshBounds[oldCount + iSection] = m_GlobalMeshBounds;
		}
	}
}

//----------------------------------------------------------------------------

void	CRendererCache::BuildCacheInfos_StaticMesh()
{
	PK_ASSERT(m_GameThreadDesc.m_StaticMesh != null);
	PK_ASSERT(m_GameThreadDesc.m_RendererMaterial != null);
	const FStaticMeshRenderData		*renderData = m_GameThreadDesc.m_StaticMesh->GetRenderData();
	FPopcornFXSubRendererMaterial	*rendererSubMat = m_GameThreadDesc.m_RendererMaterial->GetSubMaterial(0);
	PK_ASSERT(rendererSubMat != null);

	if (renderData != null)
	{
		m_GlobalMeshBounds = ToPk(FBox3f(m_GameThreadDesc.m_StaticMesh->GetBoundingBox())) * FPopcornFXPlugin::GlobalScaleRcp();

		const u32		LODCount = rendererSubMat->PerParticleLOD ? renderData->LODResources.Num() - m_GameThreadDesc.m_BaseLODLevel : 1;
		PK_VERIFY(m_PerLODMeshCount.Resize(LODCount));
		for (u32 i = 0; i < m_PerLODMeshCount.Count(); ++i)
		{
			m_PerLODMeshCount[i] = renderData->LODResources[m_GameThreadDesc.m_BaseLODLevel + i].Sections.Num();

			// WIP
			const u32	oldCount = m_SubMeshBounds.Count();
			PK_VERIFY(m_SubMeshBounds.Resize(oldCount + m_PerLODMeshCount[i]));
			for (u32 iSection = 0; iSection < m_PerLODMeshCount[i]; ++iSection)
				m_SubMeshBounds[oldCount + iSection] = m_GlobalMeshBounds;
		}
	}
}

//----------------------------------------------------------------------------

void	CRendererCache::UpdateThread_BuildBillboardingFlags(const PopcornFX::PRendererDataBase &renderer)
{
	PK_ASSERT(renderer != null);

	m_Flags.m_HasUV = true;
	m_Flags.m_FlipU = false;
	m_Flags.m_FlipV = false;
	m_Flags.m_RotateTexture = false;
	m_Flags.m_HasRibbonCorrectDeformation = false;
	m_Flags.m_HasAtlasBlending = m_GameThreadDesc.m_HasAtlasBlending;
	m_Flags.m_NeedSort = m_GameThreadDesc.m_NeedSort;
	m_Flags.m_HasNormal = m_GameThreadDesc.m_IsLit;
	m_Flags.m_HasTangent = m_GameThreadDesc.m_IsLit; // TODO: we could skip this when flat normal map is provided
	m_Flags.m_Slicable = false;

	if (renderer->m_RendererType == PopcornFX::Renderer_Billboard ||
		renderer->m_RendererType == PopcornFX::Renderer_Ribbon ||
		renderer->m_RendererType == PopcornFX::Renderer_Triangle)
	{
		if (m_GameThreadDesc.m_RendererMaterial != null)
		{
			const FPopcornFXSubRendererMaterial	*rendererSubMat = m_GameThreadDesc.m_RendererMaterial->GetSubMaterial(0);
			if (PK_VERIFY(rendererSubMat != null) &&
				rendererSubMat->Material != null)
			{
				const EBlendMode	blendMode = rendererSubMat->Material->GetBlendMode();
				m_Flags.m_Slicable = (blendMode == BLEND_Translucent) || (blendMode == BLEND_Additive) || (blendMode == BLEND_AlphaComposite) || (blendMode == BLEND_Modulate);
			}
		}
	}
	// Automatically disable sort for materials that do not support slicing (solid objects and unsupported renderers)
	if (m_Flags.m_NeedSort && !m_Flags.m_Slicable)
		m_Flags.m_NeedSort = false;

	if (renderer->m_RendererType == PopcornFX::Renderer_Billboard)
	{
		const PopcornFX::SRendererFeaturePropertyValue	*flipUVs = renderer->m_Declaration.FindProperty(PopcornFX::BasicRendererProperties::SID_FlipUVs());

		if (flipUVs != null && flipUVs->ValueB())
		{
			m_Flags.m_FlipU = true;
			m_Flags.m_FlipV = true;
		}
	}
	else if (renderer->m_RendererType == PopcornFX::Renderer_Ribbon)
	{
		const PopcornFX::SRendererFeaturePropertyValue	*textureUVs = renderer->m_Declaration.FindProperty(PopcornFX::BasicRendererProperties::SID_TextureUVs());

		if (textureUVs != null && textureUVs->ValueB())
		{
			const PopcornFX::SRendererFeaturePropertyValue	*flipU = renderer->m_Declaration.FindProperty(PopcornFX::BasicRendererProperties::SID_TextureUVs_FlipU());
			const PopcornFX::SRendererFeaturePropertyValue	*flipV = renderer->m_Declaration.FindProperty(PopcornFX::BasicRendererProperties::SID_TextureUVs_FlipV());
			const PopcornFX::SRendererFeaturePropertyValue	*rotateTexture = renderer->m_Declaration.FindProperty(PopcornFX::BasicRendererProperties::SID_TextureUVs_RotateTexture());

			m_Flags.m_FlipU = flipU != null ? flipU->ValueB() : false;
			m_Flags.m_FlipV = flipV != null ? flipV->ValueB() : false;
			m_Flags.m_RotateTexture = rotateTexture != null ? rotateTexture->ValueB() : false;
		}

		const PopcornFX::SRendererFeaturePropertyValue	*correctDeformation = renderer->m_Declaration.FindProperty(PopcornFX::BasicRendererProperties::SID_CorrectDeformation());
		m_Flags.m_HasRibbonCorrectDeformation = correctDeformation != null ? correctDeformation->ValueB() : false;
	}
	else if (renderer->m_RendererType == PopcornFX::Renderer_Mesh)
	{
		if (m_GameThreadDesc.m_SkeletalMesh != null)
			BuildCacheInfos_SkeletalMesh();
		else if (m_GameThreadDesc.m_StaticMesh != null)
			BuildCacheInfos_StaticMesh();
	}
}

//----------------------------------------------------------------------------

bool	CRendererCache::RenderThread_Setup()
{
	if (!m_RenderThreadDesc.SetupFromGame(m_GameThreadDesc))
		return false;
	return true;
}

//----------------------------------------------------------------------------
//
//	GameThread desc
//
//----------------------------------------------------------------------------

CMaterialDesc_GameThread::~CMaterialDesc_GameThread()
{
	Clear();
}

//----------------------------------------------------------------------------

bool	CMaterialDesc_GameThread::MaterialIsValid() const
{
	if (m_RendererMaterial == null)
		return false;
	return PK_VERIFY(m_RendererMaterial->IsValidLowLevel());
}

//----------------------------------------------------------------------------

bool	CMaterialDesc_GameThread::HasMaterial() const
{
	return m_RendererClass != PopcornFX::Renderer_Light &&
		m_RendererClass != PopcornFX::Renderer_Sound;
}

//----------------------------------------------------------------------------

void	CMaterialDesc_GameThread::Clear()
{
	m_RendererMaterial = null;

#if WITH_EDITOR
	if (m_StaticMesh != null)
	{
		m_StaticMesh->GetOnMeshChanged().RemoveAll(this);
		m_StaticMesh->OnPostMeshBuild().RemoveAll(this);
	}
	if (m_SkeletalMesh != null)
		m_SkeletalMesh->GetOnMeshChanged().RemoveAll(this);
#endif // WITH_EDITOR

	PK_SAFE_DELETE(m_SoundPoolCollection);
}

#if WITH_EDITOR
//----------------------------------------------------------------------------

void	CMaterialDesc_GameThread::OnMeshChanged()
{
	if (PK_VERIFY(m_StaticMesh != null))
	{
		_BuildStaticMesh();
	}
}

//----------------------------------------------------------------------------

void	CMaterialDesc_GameThread::OnSkelMeshChanged()
{
	if (PK_VERIFY(m_RendererMaterial != null))
	{
		FPopcornFXSubRendererMaterial	*rendererSubMat = m_RendererMaterial->GetSubMaterial(0);
		PK_ASSERT(rendererSubMat != null);

		if (PK_VERIFY(m_SkeletalMesh != null))
		{
			// Rebuild the bone indices reorder array
			rendererSubMat->BuildSkelMeshBoneIndicesReorder();

			// Grab new render data
			_BuildSkelMesh();

			// Update the renderer cache
			PK_ASSERT(m_Renderer != null);
			PK_ASSERT(m_Renderer->m_RendererCache != null);
			CRendererCache	*rCache = static_cast<CRendererCache*>(m_Renderer->m_RendererCache.Get());
			rCache->BuildCacheInfos_SkeletalMesh();
		}
	}
}

//----------------------------------------------------------------------------

void	CMaterialDesc_GameThread::OnMeshPostBuild(UStaticMesh *staticMesh)
{
	if (PK_VERIFY(m_StaticMesh == staticMesh))
		OnMeshChanged();
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR

//----------------------------------------------------------------------------

void	CMaterialDesc_GameThread::_BuildStaticMesh()
{
	const u32	baseLODLevel = 0; // TODO
	m_StaticMeshRenderData = m_StaticMesh->GetRenderData();
	m_BaseLODLevel = PopcornFX::PKMin(baseLODLevel, (u32)m_StaticMeshRenderData->LODResources.Num() - 1);
}

//----------------------------------------------------------------------------

void	CMaterialDesc_GameThread::_BuildSkelMesh()
{
	const u32	baseLODLevel = 0; // TODO
	m_SkeletalMeshRenderData = m_SkeletalMesh->GetResourceForRendering();
	const TArray<FSkeletalMeshLODInfo>	&LODInfos = m_SkeletalMesh->GetLODInfoArray();
	m_SkeletalMeshLODInfos = PopcornFX::TMemoryView<const FSkeletalMeshLODInfo>(LODInfos.GetData(), LODInfos.Num());
	m_BaseLODLevel = PopcornFX::PKMin(baseLODLevel, (u32)m_SkeletalMeshRenderData->LODRenderData.Num() - 1);
	m_TotalBoneCount = m_SkeletalMesh->GetRefSkeleton().GetNum();

	if (PK_VERIFY(m_RendererMaterial != null))
	{
		const FPopcornFXSubRendererMaterial	*rendererSubMat = m_RendererMaterial->GetSubMaterial(0);
		if (PK_VERIFY(rendererSubMat != null))
			m_SkeletalMeshBoneIndicesReorder = PopcornFX::TMemoryView<const u32>(rendererSubMat->m_SkeletalMeshBoneIndicesReorder.GetData(), rendererSubMat->m_SkeletalMeshBoneIndicesReorder.Num());
	}
}

//----------------------------------------------------------------------------

bool	CMaterialDesc_GameThread::GameThread_Setup()
{
	PK_ASSERT(IsInGameThread());

	if (m_RendererMaterial == null)
	{
		if (m_RendererClass != PopcornFX::Renderer_Light &&
			m_RendererClass != PopcornFX::Renderer_Sound)
		{
			PK_ASSERT_NOT_REACHED();
			return false;
		}
		m_CorrectDeformation = false;
		m_NeedSort = false;
		m_HasAtlasBlending = false;
		m_HasAlphaRemapper = false;
		m_IsLit = false;
		m_LightsTranslucent = false;
		m_CastShadows = false;

		if (m_RendererClass == PopcornFX::Renderer_Sound)
		{
			m_SoundPoolCollection = PK_NEW(CSoundDescriptorPoolCollection);

			const u32	soundCount = 1; // TODO: Sound atlases
			if (!PK_VERIFY(m_SoundPoolCollection->m_Pools.Resize(soundCount)))
				return false;
			if (!m_SoundPoolCollection->m_Pools[0].Setup(m_SoundPoolCollection, m_Renderer))
				return false;
		}
		else
		{
			const PopcornFX::SRendererFeaturePropertyValue	*lightsTranslucent = m_Renderer->m_Declaration.FindProperty(PopcornFX::BasicRendererProperties::SID_LightTranslucent());
			m_LightsTranslucent = lightsTranslucent != null ? lightsTranslucent->ValueB() : false;

			const PopcornFX::SRendererFeaturePropertyValue	*castShadows = m_Renderer->m_Declaration.FindProperty(PopcornFX::BasicRendererProperties::SID_LegacyLit_CastShadows());
			m_CastShadows = castShadows != null ? castShadows->ValueB() : false;
		}
	}
	else
	{
		if (!PK_VERIFY(MaterialIsValid()))
			return false;
		const FPopcornFXSubRendererMaterial	*rendererSubMat = m_RendererMaterial->GetSubMaterial(0);
		if (!PK_VERIFY(rendererSubMat != null))
		{
			Clear();
			return false;
		}
		m_DynamicParameterMask = rendererSubMat->DynamicParameterMask;
		m_Raytraced = (rendererSubMat->Raytraced != 0);
		m_CastShadows = (rendererSubMat->CastShadow != 0);
		m_CorrectDeformation = (rendererSubMat->CorrectDeformation != 0);
		m_IsLit = (rendererSubMat->Lit != 0);
		if (m_RendererClass == PopcornFX::Renderer_Mesh)
		{
#if WITH_EDITOR
			const bool	meshChanged = m_StaticMesh != rendererSubMat->StaticMesh;
			if (meshChanged && m_StaticMesh != null)
			{
				m_StaticMesh->GetOnMeshChanged().RemoveAll(this);
				m_StaticMesh->OnPostMeshBuild().RemoveAll(this);
			}
			const bool	skelMeshChanged = m_SkeletalMesh != rendererSubMat->SkeletalMesh;
			if (skelMeshChanged && m_SkeletalMesh != null)
				m_SkeletalMesh->GetOnMeshChanged().RemoveAll(this);
#endif // WITH_EDITOR

			m_PerParticleLOD = rendererSubMat->PerParticleLOD;
			m_MotionBlur = rendererSubMat->MotionBlur;
			const u32	baseLODLevel = 0; // TODO
			if (rendererSubMat->SkeletalMesh != null &&
				rendererSubMat->TextureSkeletalAnimation != null)
			{
				m_SkeletalMesh = rendererSubMat->SkeletalMesh;
				m_SkeletalAnimationTexture = rendererSubMat->TextureSkeletalAnimation;
				m_SkeletalAnimationCount = rendererSubMat->SkeletalAnimationCount;
				m_SkeletalAnimationPosBoundsMin = FVector3f(rendererSubMat->SkeletalAnimationPosBoundsMin);
				m_SkeletalAnimationPosBoundsMax = FVector3f(rendererSubMat->SkeletalAnimationPosBoundsMax);
				m_SkeletalAnimationLinearInterpolate = rendererSubMat->SkeletalAnimationLinearInterpolate ? 1 : 0;
				m_SkeletalAnimationLinearInterpolateTracks = rendererSubMat->SkeletalAnimationLinearInterpolateTracks ? 1 : 0;
				_BuildSkelMesh();
			}
			else if (rendererSubMat->StaticMesh != null)
			{
				if (!rendererSubMat->StaticMesh->HasValidRenderData())
					return false;
				m_StaticMesh = rendererSubMat->StaticMesh;
				_BuildStaticMesh();
			}
			else
				return false;

			m_HasMeshAtlas = rendererSubMat->MeshAtlas;

#if WITH_EDITOR
			if (meshChanged)
			{
				PK_ASSERT(m_StaticMesh != null);
				m_StaticMesh->GetOnMeshChanged().AddRaw(this, &CMaterialDesc_GameThread::OnMeshChanged);
				m_StaticMesh->OnPostMeshBuild().AddRaw(this, &CMaterialDesc_GameThread::OnMeshPostBuild);
			}
			if (skelMeshChanged)
			{
				PK_ASSERT(m_SkeletalMesh != null);
				m_SkeletalMesh->GetOnMeshChanged().AddRaw(this, &CMaterialDesc_GameThread::OnSkelMeshChanged);
			}
#endif // WITH_EDITOR
		}
		else
		{
			m_NeedSort = (rendererSubMat->SortIndices != 0);
			m_HasAtlasBlending = (rendererSubMat->SoftAnimBlending != 0 || rendererSubMat->MotionVectorsBlending != 0);
			m_HasAlphaRemapper = (rendererSubMat->TextureAlphaRemapper != 0);
		}
	}
	return true;
}

//----------------------------------------------------------------------------
//
//	RenderThread desc
//
//----------------------------------------------------------------------------

bool	CMaterialDesc_RenderThread::ValidForRendering() const
{
	if (m_MaterialInterface == null ||
		m_MaterialRenderProxy == null ||
		/* m_MaterialRenderProxy->IsDeleted() || */
		!m_MaterialRenderProxy->IsInitialized())
		return false;

//#if WITH_EDITOR Will happen in packaged games unless we can report that a material is invalid for rendering
	const FPopcornFXSubRendererMaterial	*rendererSubMat = m_RendererMaterial->GetSubMaterial(0);
	if (rendererSubMat == null)
		return false;
	if (m_RendererClass == PopcornFX::Renderer_Mesh)
	{
		return	rendererSubMat->StaticMesh != null ||
				(rendererSubMat->SkeletalMesh != null && rendererSubMat->TextureSkeletalAnimation != null);
	}
//#endif
	return true;
}

//----------------------------------------------------------------------------

void	CMaterialDesc_RenderThread::Clear()
{
	CMaterialDesc_GameThread::Clear();

	m_MaterialInterface = null;
	m_MaterialRenderProxy = null;
}

//----------------------------------------------------------------------------

bool	CMaterialDesc_RenderThread::SetupFromGame(const CMaterialDesc_GameThread &gameMat)
{
	PK_ASSERT(IsInRenderingThread());

	m_HasAtlasBlending = gameMat.m_HasAtlasBlending;
	m_HasAlphaRemapper = gameMat.m_HasAlphaRemapper;
	m_CastShadows = gameMat.m_CastShadows;
	m_CorrectDeformation = gameMat.m_CorrectDeformation;
	m_IsLit = gameMat.m_IsLit;
	m_LightsTranslucent = gameMat.m_LightsTranslucent;
	m_HasMeshAtlas = gameMat.m_HasMeshAtlas;
	m_Raytraced = gameMat.m_Raytraced;
	m_PerParticleLOD = gameMat.m_PerParticleLOD;
	m_MotionBlur = gameMat.m_MotionBlur;

	m_BaseLODLevel = gameMat.m_BaseLODLevel;

#if WITH_EDITOR
	const bool	meshChanged = m_StaticMesh != gameMat.m_StaticMesh;
	//const bool	skelMeshChanged = m_SkeletalMesh != gameMat.m_SkeletalMesh;
	if (meshChanged && m_StaticMesh != null)
	{
		m_StaticMesh->GetOnMeshChanged().RemoveAll(this);
		m_StaticMesh->OnPostMeshBuild().RemoveAll(this);
	}
	//if (skelMeshChanged && m_SkeletalMesh != null)
	//	m_SkeletalMesh->GetOnMeshChanged().RemoveAll(this);
#endif // WITH_EDITOR

	// Temp code for raytracing testing. Requires modification on UE4 side.
#if 0
#	if RHI_RAYTRACING
	if (IsRayTracingEnabled() && m_LODResources != null && m_HasMeshAtlas && m_Raytraced)
	{
		if (m_RayTracingGeometries.Num() == 0) // Careful here, in editor mode, SetupFromGame gets called every frame
		{
			FRayTracingGeometry& rayTracingGeometry = m_LODResources->RayTracingGeometry;
			const u32			sectionCount = rayTracingGeometry.Initializer.Segments.Num();

			// Create sub geoms, used by sub mesh id particles.
			m_RayTracingGeometries.SetNum(sectionCount);
			for (u32 iSection = 0; iSection < sectionCount; ++iSection)
			{
				FRayTracingGeometryInitializer	initializer;

				initializer.IndexBuffer = rayTracingGeometry.Initializer.IndexBuffer;
				initializer.IndexBufferOffset = rayTracingGeometry.Initializer.IndexBufferOffset;
				initializer.GeometryType = rayTracingGeometry.Initializer.GeometryType;
				initializer.bFastBuild = rayTracingGeometry.Initializer.bFastBuild;
				initializer.bAllowUpdate = rayTracingGeometry.Initializer.bAllowUpdate;
				initializer.TotalPrimitiveCount = rayTracingGeometry.Initializer.TotalPrimitiveCount;

				initializer.Segments.SetNum(1);
				FRayTracingGeometrySegment& dstSegment = initializer.Segments[0];
				FRayTracingGeometrySegment& srcSegment = rayTracingGeometry.Initializer.Segments[iSection];
				dstSegment.VertexBuffer = srcSegment.VertexBuffer;
				dstSegment.VertexBufferElementType = srcSegment.VertexBufferElementType;
				dstSegment.VertexBufferOffset = srcSegment.VertexBufferOffset;
				dstSegment.VertexBufferStride = srcSegment.VertexBufferStride;
				dstSegment.FirstPrimitive = srcSegment.FirstPrimitive;
				dstSegment.NumPrimitives = srcSegment.NumPrimitives;
				dstSegment.bAllowDuplicateAnyHitShaderInvocation = srcSegment.bAllowDuplicateAnyHitShaderInvocation;

				dstSegment.bForceOpaque = srcSegment.bForceOpaque;
				dstSegment.bEnabled = srcSegment.bEnabled;

				m_RayTracingGeometries[iSection].SetInitializer(initializer);
				m_RayTracingGeometries[iSection].InitResource();
			}
		}
	}
#	endif // RHI_RAYTRACING
#endif

	m_StaticMesh = gameMat.m_StaticMesh;
	m_StaticMeshRenderData = gameMat.m_StaticMeshRenderData;
	m_DynamicParameterMask = gameMat.m_DynamicParameterMask;
	m_SkeletalMesh = gameMat.m_SkeletalMesh;
	m_SkeletalMeshRenderData = gameMat.m_SkeletalMeshRenderData;
	m_SkeletalMeshLODInfos = gameMat.m_SkeletalMeshLODInfos;
	m_SkeletalAnimationTexture = gameMat.m_SkeletalAnimationTexture;
	m_SkeletalAnimationCount = gameMat.m_SkeletalAnimationCount;
	m_SkeletalAnimationPosBoundsMin = gameMat.m_SkeletalAnimationPosBoundsMin;
	m_SkeletalAnimationPosBoundsMax = gameMat.m_SkeletalAnimationPosBoundsMax;
	m_SkeletalAnimationLinearInterpolate = gameMat.m_SkeletalAnimationLinearInterpolate;
	m_SkeletalAnimationLinearInterpolateTracks = gameMat.m_SkeletalAnimationLinearInterpolateTracks;
	m_TotalBoneCount = gameMat.m_TotalBoneCount;
	m_SkeletalMeshBoneIndicesReorder = gameMat.m_SkeletalMeshBoneIndicesReorder;
#if WITH_EDITOR
	if (meshChanged && m_StaticMesh != null)
	{
		m_StaticMesh->GetOnMeshChanged().AddRaw(this, &CMaterialDesc_RenderThread::OnMeshChanged);
		m_StaticMesh->OnPostMeshBuild().AddRaw(this, &CMaterialDesc_RenderThread::OnMeshPostBuild);
	}
	//if (skelMeshChanged && m_SkeletalMesh != null)
	//	m_SkeletalMesh->GetOnMeshChanged().AddRaw(this, &CMaterialDesc_RenderThread::OnSkelMeshChanged);
#endif // WITH_EDITOR

	// m_RendererMaterial's access safety on the render thread is ensured by:
	// - FlushRenderingCommands called when the uasset is modified
	// - Fence when the asset is destroyed
	m_RendererMaterial = gameMat.m_RendererMaterial;
	m_RendererClass = gameMat.m_RendererClass;
	if (!HasMaterial()) // light and sound renderers
		return true;

	return MaterialIsValid();
}

//----------------------------------------------------------------------------

bool	CMaterialDesc_RenderThread::ResolveMaterial(PopcornFX::Drawers::EBillboardingLocation bbLocation)
{
	if (m_RendererClass == PopcornFX::Renderer_Light ||
		m_RendererClass == PopcornFX::Renderer_Sound)
		return true; // No material to resolve for lights/sounds
	PK_ASSERT(IsInRenderingThread());
	if (!MaterialIsValid())
		return false;
	UMaterialInstanceConstant	*materialInstance = m_RendererMaterial->GetInstance(0, true); // ~
	if (materialInstance == null) // We shouldn't be here
		return false;

	// We are here, we are going to be rendered. Fallback on the default material if user specified an invalid material
	m_MaterialInterface = materialInstance;
	switch (m_RendererClass)
	{
	case	PopcornFX::Renderer_Billboard:
	case	PopcornFX::Renderer_Triangle:
		if (bbLocation == PopcornFX::Drawers::BillboardingLocation_CPU ||
			bbLocation == PopcornFX::Drawers::BillboardingLocation_ComputeShader)
		{
			if (!FPopcornFXVertexFactory::IsCompatible(m_MaterialInterface))
				m_MaterialInterface = UMaterial::GetDefaultMaterial(MD_Surface);
		}
		else if (bbLocation == PopcornFX::Drawers::BillboardingLocation_VertexShader)
		{
			if (!FPopcornFXGPUVertexFactory::IsCompatible(m_MaterialInterface))
				m_MaterialInterface = UMaterial::GetDefaultMaterial(MD_Surface);
		}
		else
		{
			PK_ASSERT_NOT_REACHED();
		}
		break;
	case	PopcornFX::Renderer_Ribbon:
		if (!FPopcornFXVertexFactory::IsCompatible(m_MaterialInterface))
			m_MaterialInterface = UMaterial::GetDefaultMaterial(MD_Surface);
		break;
	case	PopcornFX::Renderer_Mesh:
		if (m_SkeletalMesh != null)
		{
			if (!FPopcornFXSkelMeshVertexFactory::IsCompatible(materialInstance))
				m_MaterialInterface = UMaterial::GetDefaultMaterial(MD_Surface);
		}
		else
		{
			if (!FPopcornFXMeshVertexFactory::IsCompatible(materialInstance))
				m_MaterialInterface = UMaterial::GetDefaultMaterial(MD_Surface);
		}
		break;
	default:
		PK_ASSERT_NOT_REACHED();
		break;
	}

	m_MaterialRenderProxy = materialInstance->GetRenderProxy();
	if (!PK_VERIFY(m_MaterialRenderProxy != null))
		return false;
	return true;
}

//----------------------------------------------------------------------------
