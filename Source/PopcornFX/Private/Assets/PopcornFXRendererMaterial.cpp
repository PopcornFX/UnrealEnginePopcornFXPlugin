//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "Assets/PopcornFXRendererMaterial.h"

#include "PopcornFXPlugin.h"
#include "Assets/PopcornFXEffect.h"
#include "Assets/PopcornFXTextureAtlas.h"
#include "PopcornFXCustomVersion.h"

#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#if (ENGINE_MAJOR_VERSION == 5)
#	include "Engine/SkinnedAssetCommon.h"
#endif
#include "MaterialShared.h"

#include "PopcornFXSDK.h"
#include <pk_base_object/include/hbo_handler.h>
#include <pk_particles/include/ps_effect.h>
#include <pk_particles/include/ps_event_map.h>
#include <pk_particles/include/ps_nodegraph_nodes_render.h>
#include <pk_particles/include/Renderers/ps_renderer_base.h>

#include "Render/PopcornFXRendererProperties.h"
#include <pk_render_helpers/include/render_features/rh_features_basic.h>
#include <pk_render_helpers/include/render_features/rh_features_vat_skeletal.h>
#include <pk_render_helpers/include/render_features/rh_features_vat_static.h>

#if WITH_EDITOR
#	include "Misc/MessageDialog.h"
#	include "Factories/FbxStaticMeshImportData.h"
#	include "Factories/ReimportFbxStaticMeshFactory.h"
#	include "Rendering/SkeletalMeshRenderData.h"
#endif

#if (ENGINE_MAJOR_VERSION == 5)
#include "UObject/ObjectSaveContext.h"
#endif // (ENGINE_MAJOR_VERSION == 5)

#define LOCTEXT_NAMESPACE "PopcornFXRendererMaterial"

// Helpers

UTexture2D					*LoadTexturePk(const PopcornFX::CString &pkPath);
UPopcornFXTextureAtlas		*LoadAtlasPk(const PopcornFX::CString &pkPath);
UStaticMesh					*LoadMeshPk(const PopcornFX::CString &pkPath);
USkeletalMesh				*LoadSkelMeshPk(const PopcornFX::CString &pkPath);
void						SetMaterialTextureParameter(UMaterialInstanceDynamic *mat, FName textureName, const PopcornFX::CString &pkTexturePath);

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXRendererMaterial, Log, All);

//----------------------------------------------------------------------------

namespace
{
	enum	ERMID
	{
		RMID_Billboard = 0,
		RMID_Ribbon,
		RMID_Mesh,
		RMID_Triangle,
		_RMID_Count,
	};

#if WITH_EDITOR
	bool	_IsVATTextureConfigured(const UTexture2D *texture)
	{
		PK_ASSERT(texture != null);
		return texture->Filter == TF_Nearest; // true if null, ignored
	}

	bool	_IsVATMeshConfigured(const UStaticMesh *staticMesh)
	{
		PK_ASSERT(staticMesh != null);
		return	staticMesh->GetSourceModel(0).BuildSettings.bUseFullPrecisionUVs &&
				!staticMesh->GetSourceModel(0).BuildSettings.bRemoveDegenerates;
	}

	void	_SetStaticSwitch(FStaticParameterSet &params, FName name, bool value)
	{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)
		for (int32 i = 0; i < params.StaticSwitchParameters.Num(); ++i)
		{
			FStaticSwitchParameter		&param  = params.StaticSwitchParameters[i];
#elif (ENGINE_MAJOR_VERSION == 5)
		for (int32 i = 0; i < params.EditorOnly.StaticSwitchParameters.Num(); ++i)
		{
			FStaticSwitchParameter		&param  = params.EditorOnly.StaticSwitchParameters[i];
#else
		for (int32 i = 0; i < params.StaticSwitchParameters.Num(); ++i)
		{
			FStaticSwitchParameter		&param  = params.StaticSwitchParameters[i];
#endif

			if (param.ParameterInfo.Name == name)
			{
				param.Value = value;
				param.bOverride = true;
				// should be uniq !?
				return;
			}
		}
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)
		new (params.StaticSwitchParameters) FStaticSwitchParameter(name, value, true, FGuid()); // FGuid ???
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 1)
		new (params.EditorOnly.StaticSwitchParameters) FStaticSwitchParameter(name, value, true, FGuid()); // FGuid ???
#else
		new (params.StaticSwitchParameters) FStaticSwitchParameter(name, value, true, FGuid()); // FGuid ???
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	}

	void	_SetStaticSwitches_Common(UMaterialInstanceConstant *material, FPopcornFXSubRendererMaterial &mat)
	{
		FStaticParameterSet			params = material->GetStaticParameters();

		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_SOFT_PARTICLES(), mat.SoftnessDistance > 0.f);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_SOFT_ANIM(), mat.SoftAnimBlending != 0);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_MOTION_VECTORS_BLENDING(), mat.MotionVectorsBlending != 0);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_LIT(), mat.Lit!= 0); // Currently used by VAT materials only
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_HAS_NORMAL_TEXTURE(), mat.TextureNormal != null);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_HAS_ROUGH_METAL_TEXTURE(), mat.TextureRoughMetal != null);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_HAS_ALPHA_REMAPPER(), mat.TextureAlphaRemapper != null);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_HAS_DIFFUSE_RAMP(), mat.TextureDiffuseRamp != null);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_HAS_EMISSIVE_TEXTURE(), mat.TextureEmissive != null);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_HAS_SIXWAYLIGHTMAP_TEXTURES(), mat.TextureSixWay_RLTS != null && mat.TextureSixWay_BBF != null);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_NO_ALPHA(), mat.NoAlpha != 0);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_CAST_SHADOW(), mat.CastShadow != 0);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_HAS_VAT_NORMALIZEDDATA(), mat.VATNormalizedData != 0);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_HAS_VAT_PACKEDDATA(), mat.VATPackedData != 0);

		PK_ASSERT(mat.m_RMId >= 0 && mat.m_RMId < _RMID_Count);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_BILLBOARD(), mat.m_RMId == RMID_Billboard);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_RIBBON(), mat.m_RMId == RMID_Ribbon);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_MESH(), mat.m_RMId == RMID_Mesh);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_TRIANGLE(), mat.m_RMId == RMID_Triangle);

		material->UpdateStaticPermutation(params);
	}
#endif

	struct FRendererMaterialsFuncs
	{
#if WITH_EDITOR
		typedef bool(*CbSetup)(UPopcornFXRendererMaterial *self, const void *rendererBase /* PopcornFX::CRendererDataBase so PK type doesn't leak in public header */);
		typedef bool(*CbConstParams)(UPopcornFXRendererMaterial *self, UMaterialInstanceConstant *material, uint32 subMaterialIndex);
		CbSetup			m_SetupFunc;
		CbConstParams	m_ParamsConstFunc;
#endif // WITH_EDITOR
	};

#if WITH_EDITOR
	bool		RM_Setup_Billboard_Legacy(FPopcornFXSubRendererMaterial& mat, const PopcornFX::SRendererDeclaration &decl)
	{
		const bool	legacyLit = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_LegacyLit());
		mat.Lit = legacyLit || decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Lit());
		EPopcornFXLegacyMaterialType	finalMatType = mat.LegacyMaterialType;

		if (mat.Lit)
		{
			if (legacyLit)
			{
				mat.CastShadow = decl.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_LegacyLit_CastShadows(), false);
				mat.TextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_LegacyLit_NormalMap(), PopcornFX::CString::EmptyString));
			}
			else
			{
				mat.CastShadow = decl.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_Lit_CastShadows(), false);
				mat.TextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_NormalMap(), PopcornFX::CString::EmptyString));
				mat.TextureRoughMetal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_RoughMetalMap(), PopcornFX::CString::EmptyString));
				mat.Roughness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Roughness(), 1.0f);
				mat.Metalness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Metalness(), 0.0f);
			}
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Transparent()))
		{
			finalMatType = kLegacyTransparentBillboard_Material_ToUE[decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Transparent_Type(), 0)];
			mat.DrawOrder = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Transparent_GlobalSortOverride(), 0);
			if (decl.IsFeatureEnabled(UERendererProperties::SID_SixWayLightmap()))
			{
				mat.CastShadow = true; // Tmp: Force cast shadows true when six way lightmap material is used
				finalMatType = EPopcornFXLegacyMaterialType::Billboard_SixWayLightmap;
				mat.TextureSixWay_RLTS = LoadTexturePk(decl.GetPropertyValue_Path(UERendererProperties::SID_SixWayLightmap_RightLeftTopSmoke(), PopcornFX::CString::EmptyString));
				mat.TextureSixWay_BBF = LoadTexturePk(decl.GetPropertyValue_Path(UERendererProperties::SID_SixWayLightmap_BottomBackFront(), PopcornFX::CString::EmptyString));
			}
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Opaque()))
		{
			finalMatType = kOpaqueBillboard_LegacyMaterial_ToUE[decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Opaque_Type(), 0)];
			if (finalMatType == EPopcornFXLegacyMaterialType::Billboard_Masked)
				mat.MaskThreshold = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Opaque_MaskThreshold(), 0.0f);
		}
		else if (decl.IsFeatureEnabled(UERendererProperties::SID_VolumetricFog()))
		{
			finalMatType = EPopcornFXLegacyMaterialType::Billboard_VolumetricFog;
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(UERendererProperties::SID_VolumetricFog_AlbedoMap(), PopcornFX::CString::EmptyString));
			mat.SphereMaskHardness = decl.GetPropertyValue_F1(UERendererProperties::SID_VolumetricFog_SphereMaskHardness(), 0.0f);
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Emissive()))
			mat.TextureEmissive = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveMap(), PopcornFX::CString::EmptyString));
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Diffuse()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseMap(), PopcornFX::CString::EmptyString));
			if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_DiffuseRamp()))
				mat.TextureDiffuseRamp = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_DiffuseRamp_RampMap(), PopcornFX::CString::EmptyString));
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Distortion()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Distortion_DistortionMap(), PopcornFX::CString::EmptyString));
			finalMatType = EPopcornFXLegacyMaterialType::Billboard_Distortion;

			if (mat.TextureDiffuse != null &&
				mat.TextureDiffuse->CompressionSettings != TextureCompressionSettings::TC_Normalmap)
			{
				FText		title = LOCTEXT("import_distortion_texture_title", "PopcornFX: Compression Settings");
				const FText	message = FText::Format(LOCTEXT("import_distortion_texture_message",
					"Texture \"{0}\" is used for distortion but has incompatible compression settings. Do you want to import it using NormalMap compression settings instead?"),
					FText::FromString(mat.TextureDiffuse->GetPathName()));

				if (OpenMessageBox(EAppMsgType::YesNo, message, title) == EAppReturnType::Yes)
				{
					mat.TextureDiffuse->Modify();
					mat.TextureDiffuse->CompressionSettings = TextureCompressionSettings::TC_Normalmap;
				}
			}
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Atlas()))
		{
			mat.TextureAtlas = LoadAtlasPk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Atlas_Definition(), PopcornFX::CString::EmptyString));

			const s32	blendingType = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Atlas_Blending(), 0); // 1 = LinearAtlasBlending. 2 = MotionVectors blending (optical flow);
			mat.SoftAnimBlending = blendingType == 1;
			mat.MotionVectorsBlending = blendingType == 2;
			if (mat.MotionVectorsBlending)
			{
				mat.TextureMotionVectors = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Atlas_MotionVectorsMap(), PopcornFX::CString::EmptyString));

				const CFloat2	distortionStrength = decl.GetPropertyValue_F2(PopcornFX::BasicRendererProperties::SID_Atlas_DistortionStrength(), CFloat2::ZERO);
				mat.MVDistortionStrengthColumns = distortionStrength.x();
				mat.MVDistortionStrengthRows = distortionStrength.y();
			}
		}
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_AlphaRemap()))
			mat.TextureAlphaRemapper = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_AlphaRemap_AlphaMap(), PopcornFX::CString::EmptyString));
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_SoftParticles()))
			mat.SoftnessDistance = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_SoftParticles_SoftnessDistance(), 0.0f);
		
		switch (finalMatType)
		{
		case	EPopcornFXLegacyMaterialType::Billboard_Additive_NoAlpha:
			finalMatType = EPopcornFXLegacyMaterialType::Billboard_Additive;
			mat.NoAlpha = true;
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_Additive:
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_AlphaBlend:
			if (mat.Lit)
				finalMatType = EPopcornFXLegacyMaterialType::Billboard_AlphaBlend_Lit;
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_AlphaBlendAdditive:
			if (mat.Lit)
				finalMatType = EPopcornFXLegacyMaterialType::Billboard_AlphaBlendAdditive_Lit;
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_Distortion:
			PK_ASSERT(!mat.Lit);
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_Solid:
			if (mat.Lit)
				finalMatType = EPopcornFXLegacyMaterialType::Billboard_Solid_Lit;
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_Masked:
			if (mat.Lit)
				finalMatType = EPopcornFXLegacyMaterialType::Billboard_Masked_Lit;
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_VolumetricFog:
		case	EPopcornFXLegacyMaterialType::Billboard_SixWayLightmap:
			finalMatType = mat.LegacyMaterialType;
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			return false;
		}
		mat.LegacyMaterialType = finalMatType;
		return true;
	}

bool		RM_Setup_Billboard_Default(FPopcornFXSubRendererMaterial& mat, const PopcornFX::SRendererDeclaration &decl)
	{
		mat.Lit = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Lit());
		EPopcornFXDefaultMaterialType	finalMatType = mat.DefaultMaterialType;

		if (mat.Lit)
		{
			mat.CastShadow = decl.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_Lit_CastShadows(), false);
			mat.TextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_NormalMap(), PopcornFX::CString::EmptyString));
			mat.TextureRoughMetal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_RoughMetalMap(), PopcornFX::CString::EmptyString));
			mat.Roughness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Roughness(), 1.0f);
			mat.Metalness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Metalness(), 0.0f);
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Transparent()))
		{
			mat.DrawOrder = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Transparent_GlobalSortOverride(), 0);
			finalMatType = EPopcornFXDefaultMaterialType::Billboard_AlphaBlendAdditive;
			if (!decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Emissive()))
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_AlphaBlend;
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Opaque()))
		{
			finalMatType = kOpaqueBillboard_DefaultMaterial_ToUE[decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Opaque_Type(), 0)];
			if (finalMatType == EPopcornFXDefaultMaterialType::Billboard_Masked)
				mat.MaskThreshold = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Opaque_MaskThreshold(), 0.0f);
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Emissive()))
		{
			mat.TextureEmissive = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveMap(), PopcornFX::CString::EmptyString));
			if (!decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Diffuse()))
			{
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_Additive;
				mat.NoAlpha = true;
			}
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Diffuse()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseMap(), PopcornFX::CString::EmptyString));
			if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_DiffuseRamp()))
				mat.TextureDiffuseRamp = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_DiffuseRamp_RampMap(), PopcornFX::CString::EmptyString));
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Distortion()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Distortion_DistortionMap(), PopcornFX::CString::EmptyString));
			finalMatType = EPopcornFXDefaultMaterialType::Billboard_Distortion;

			if (mat.TextureDiffuse != null &&
				mat.TextureDiffuse->CompressionSettings != TextureCompressionSettings::TC_Normalmap)
			{
				FText		title = LOCTEXT("import_distortion_texture_title", "PopcornFX: Compression Settings");
				const FText	message = FText::Format(LOCTEXT("import_distortion_texture_message",
					"Texture \"{0}\" is used for distortion but has incompatible compression settings. Do you want to import it using NormalMap compression settings instead?"),
					FText::FromString(mat.TextureDiffuse->GetPathName()));

				if (OpenMessageBox(EAppMsgType::YesNo, message, title) == EAppReturnType::Yes)
				{
					mat.TextureDiffuse->Modify();
					mat.TextureDiffuse->CompressionSettings = TextureCompressionSettings::TC_Normalmap;
				}
			}
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Atlas()))
		{
			mat.TextureAtlas = LoadAtlasPk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Atlas_Definition(), PopcornFX::CString::EmptyString));

			const s32	blendingType = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Atlas_Blending(), 0); // 1 = LinearAtlasBlending. 2 = MotionVectors blending (optical flow);
			mat.SoftAnimBlending = blendingType == 1;
			mat.MotionVectorsBlending = blendingType == 2;
			if (mat.MotionVectorsBlending)
			{
				mat.TextureMotionVectors = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Atlas_MotionVectorsMap(), PopcornFX::CString::EmptyString));

				const CFloat2	distortionStrength = decl.GetPropertyValue_F2(PopcornFX::BasicRendererProperties::SID_Atlas_DistortionStrength(), CFloat2::ZERO);
				mat.MVDistortionStrengthColumns = distortionStrength.x();
				mat.MVDistortionStrengthRows = distortionStrength.y();
			}
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_AlphaRemap()))
			mat.TextureAlphaRemapper = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_AlphaRemap_AlphaMap(), PopcornFX::CString::EmptyString));

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_SoftParticles()))
			mat.SoftnessDistance = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_SoftParticles_SoftnessDistance(), 0.0f);


		switch (finalMatType)
		{
#if 0
		case	EPopcornFXDefaultMaterialType::Billboard_Additive_NoAlpha:
			finalMatType = EPopcornFXLegacyMaterialType::Billboard_Additive;
			mat.NoAlpha = true;
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
#endif
		case	EPopcornFXDefaultMaterialType::Billboard_Additive:
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_AlphaBlend:
			if (mat.Lit)
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_AlphaBlend_Lit;
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_AlphaBlendAdditive:
			if (mat.Lit)
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_AlphaBlendAdditive_Lit;
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_Distortion:
			PK_ASSERT(!mat.Lit);
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_Solid:
			if (mat.Lit)
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_Solid_Lit;
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_Masked:
			if (mat.Lit)
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_Masked_Lit;
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			return false;
		}
		mat.DefaultMaterialType = finalMatType;
		return true;
	}


	bool		RM_Setup_Billboard(UPopcornFXRendererMaterial *self, const void *rendererBase)
	{
		const PopcornFX::CRendererDataBase	*_rendererBase = static_cast<const PopcornFX::CRendererDataBase*>(rendererBase);
		if (_rendererBase->m_RendererType != PopcornFX::Renderer_Billboard)
			return false;
		const PopcornFX::CRendererDataBillboard	*renderer = static_cast<const PopcornFX::CRendererDataBillboard*>(_rendererBase);
		if (renderer == null)
			return false;


		const PopcornFX::SRendererFeaturePropertyValue	*prop = null;
		const PopcornFX::SRendererDeclaration			&decl = renderer->m_Declaration;
		bool success = true;


		self->SubMaterials.SetNum(1);
		FPopcornFXSubRendererMaterial &mat = self->SubMaterials[0];

		// Default values:
		mat.IsLegacy = decl.FindAdditionalFieldDefinition(PopcornFX::CStringId("Diffuse.Color")) != null
						|| (decl.m_MaterialPath.Contains("Legacy", PopcornFX::CaseInsensitive)
							&& decl.m_MaterialPath.Contains("PopcornFXCore", PopcornFX::CaseInsensitive));
		mat.LegacyMaterialType = EPopcornFXLegacyMaterialType::Billboard_Additive;
		mat.DefaultMaterialType = EPopcornFXDefaultMaterialType::Billboard_Additive;
		mat.Lit = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Lit());
		mat.Raytraced = decl.IsFeatureEnabled(PopcornFX::CStringId("Raytraced")); // tmp

		if (mat.IsLegacy)
			success &= RM_Setup_Billboard_Legacy(mat, decl);
		else
			success &= RM_Setup_Billboard_Default(mat, decl);

		// Not correct, this should be derived from the material Blend Mode as it can be overriden
		mat.SortIndices = mat.NeedsSorting();

		mat.BuildDynamicParameterMask(_rendererBase, decl);

		return success;
	}

	bool		RM_Params_Billboard(UPopcornFXRendererMaterial *self, UMaterialInstanceDynamic *material, uint32 subMaterialIndex)
	{
		check(subMaterialIndex < uint32(self->SubMaterials.Num()));
		FPopcornFXSubRendererMaterial		&mat = self->SubMaterials[subMaterialIndex];
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_DiffuseTexture(), mat.TextureDiffuse);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_DiffuseTextureRamp(), mat.TextureDiffuseRamp);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_EmissiveTexture(), mat.TextureEmissive);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_NormalTexture(), mat.TextureNormal);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_RoughMetalTexture(), mat.TextureRoughMetal);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_SpecularTexture(), mat.TextureSpecular);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_AlphaRemapper(), mat.TextureAlphaRemapper);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_MotionVectorsTexture(), mat.TextureMotionVectors);

		material->SetTextureParameterValue(FPopcornFXPlugin::Name_SixWayLightmap_RLTSTexture(), mat.TextureSixWay_RLTS);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_SixWayLightmap_BBFTexture(), mat.TextureSixWay_BBF);

		material->SetScalarParameterValue(FPopcornFXPlugin::Name_Roughness(), mat.Roughness);
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_Metalness(), mat.Metalness);
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_SoftnessDistance(), mat.SoftnessDistance * FPopcornFXPlugin::GlobalScale());
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_MaskThreshold(), mat.MaskThreshold);
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_SphereMaskHardness(), mat.SphereMaskHardness);
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_MVDistortionStrengthColumns(), mat.MVDistortionStrengthColumns);
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_MVDistortionStrengthRows(), mat.MVDistortionStrengthRows);
		return true;
	}

	bool		RM_ParamsConst_Billboard(UPopcornFXRendererMaterial *self, UMaterialInstanceConstant *material, uint32 subMaterialIndex)
	{
		check(subMaterialIndex < uint32(self->SubMaterials.Num()));
		FPopcornFXSubRendererMaterial		&mat = self->SubMaterials[subMaterialIndex];

		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_DiffuseTexture(), mat.TextureDiffuse);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_DiffuseTextureRamp(), mat.TextureDiffuseRamp);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_EmissiveTexture(), mat.TextureEmissive);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_NormalTexture(), mat.TextureNormal);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_RoughMetalTexture(), mat.TextureRoughMetal);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_SpecularTexture(), mat.TextureSpecular);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_AlphaRemapper(), mat.TextureAlphaRemapper);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_MotionVectorsTexture(), mat.TextureMotionVectors);

		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_SixWayLightmap_RLTSTexture(), mat.TextureSixWay_RLTS);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_SixWayLightmap_BBFTexture(), mat.TextureSixWay_BBF);

		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_Roughness(), mat.Roughness);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_Metalness(), mat.Metalness);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_SoftnessDistance(), mat.SoftnessDistance * FPopcornFXPlugin::GlobalScale());
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_MaskThreshold(), mat.MaskThreshold);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_SphereMaskHardness(), mat.SphereMaskHardness);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_MVDistortionStrengthColumns(), mat.MVDistortionStrengthColumns);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_MVDistortionStrengthRows(), mat.MVDistortionStrengthRows);

		_SetStaticSwitches_Common(material, mat);

		// !!
		material->PostEditChange();
		return true;
	}
#endif // WITH_EDITOR

#if WITH_EDITOR
	bool		RM_Setup_Ribbon_Legacy(FPopcornFXSubRendererMaterial &mat, const PopcornFX::SRendererDeclaration &decl)
	{
		const bool	legacyLit = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_LegacyLit());
		mat.Lit = legacyLit || decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Lit());

		EPopcornFXLegacyMaterialType	finalMatType = mat.LegacyMaterialType;
		if (mat.Lit)
		{
			if (legacyLit)
			{
				mat.CastShadow = decl.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_LegacyLit_CastShadows(), false);
				mat.TextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_LegacyLit_NormalMap(), PopcornFX::CString::EmptyString));
			}
			else
			{
				mat.CastShadow = decl.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_Lit_CastShadows(), false);
				mat.TextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_NormalMap(), PopcornFX::CString::EmptyString));
				mat.TextureRoughMetal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_RoughMetalMap(), PopcornFX::CString::EmptyString));
				mat.Roughness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Roughness(), 1.0f);
				mat.Metalness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Metalness(), 0.0f);
			}
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Transparent()))
		{
			finalMatType = kLegacyTransparentBillboard_Material_ToUE[decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Transparent_Type(), 0)];
			mat.DrawOrder = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Transparent_GlobalSortOverride(), 0);
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Opaque()))
		{
			finalMatType = kOpaqueBillboard_LegacyMaterial_ToUE[decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Opaque_Type(), 0)];
			if (finalMatType == EPopcornFXLegacyMaterialType::Billboard_Masked)
				mat.MaskThreshold = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Opaque_MaskThreshold(), 0.0f);
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Emissive()))
			mat.TextureEmissive = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveMap(), PopcornFX::CString::EmptyString));
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Diffuse()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseMap(), PopcornFX::CString::EmptyString));
			if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_DiffuseRamp()))
				mat.TextureDiffuseRamp = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_DiffuseRamp_RampMap(), PopcornFX::CString::EmptyString));
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Distortion()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Distortion_DistortionMap(), PopcornFX::CString::EmptyString));
			finalMatType = EPopcornFXLegacyMaterialType::Billboard_Distortion;

			if (mat.TextureDiffuse != null &&
				mat.TextureDiffuse->CompressionSettings != TextureCompressionSettings::TC_Normalmap)
			{
				FText		title = LOCTEXT("import_distortion_texture_title", "PopcornFX: Compression Settings");
				const FText	message = FText::Format(LOCTEXT("import_distortion_texture_message",
					"Texture \"{0}\" is used for distortion but has incompatible compression settings. Do you want to import it using NormalMap compression settings instead?"),
					FText::FromString(mat.TextureDiffuse->GetPathName()));

				if (OpenMessageBox(EAppMsgType::YesNo, message, title) == EAppReturnType::Yes)
				{
					mat.TextureDiffuse->Modify();
					mat.TextureDiffuse->CompressionSettings = TextureCompressionSettings::TC_Normalmap;
				}
			}
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Atlas()))
		{
			mat.TextureAtlas = LoadAtlasPk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Atlas_Definition(), PopcornFX::CString::EmptyString));

			const s32	blendingType = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Atlas_Blending(), 0); // 1 = LinearAtlasBlending. 2 = MotionVectors blending (optical flow);
			mat.SoftAnimBlending = blendingType == 1;
			mat.MotionVectorsBlending = blendingType == 2;
			if (mat.MotionVectorsBlending)
			{
				mat.TextureMotionVectors = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Atlas_MotionVectorsMap(), PopcornFX::CString::EmptyString));

				const CFloat2	distortionStrength = decl.GetPropertyValue_F2(PopcornFX::BasicRendererProperties::SID_Atlas_DistortionStrength(), CFloat2::ZERO);
				mat.MVDistortionStrengthColumns = distortionStrength.x();
				mat.MVDistortionStrengthRows = distortionStrength.y();
			}
		}
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_AlphaRemap()))
			mat.TextureAlphaRemapper = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_AlphaRemap_AlphaMap(), PopcornFX::CString::EmptyString));
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_SoftParticles()))
			mat.SoftnessDistance = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_SoftParticles_SoftnessDistance(), 0.0f);

		
		switch (finalMatType)
		{
		case	EPopcornFXLegacyMaterialType::Billboard_Additive_NoAlpha:
		{
			finalMatType = EPopcornFXLegacyMaterialType::Billboard_Additive;
			mat.NoAlpha = true;
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		}
		case	EPopcornFXLegacyMaterialType::Billboard_Additive:
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_AlphaBlend:
			if (mat.Lit)
				finalMatType = EPopcornFXLegacyMaterialType::Billboard_AlphaBlend_Lit;
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_AlphaBlendAdditive:
			if (mat.Lit)
				finalMatType = EPopcornFXLegacyMaterialType::Billboard_AlphaBlendAdditive_Lit;
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_Distortion:
			PK_ASSERT(!mat.Lit);
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_Solid:
			if (mat.Lit)
				finalMatType = EPopcornFXLegacyMaterialType::Billboard_Solid_Lit;
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_Masked:
			if (mat.Lit)
				finalMatType = EPopcornFXLegacyMaterialType::Billboard_Masked_Lit;
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			return false;
		}
		mat.LegacyMaterialType = finalMatType;
		return true;
	}

	bool		RM_Setup_Ribbon_Default(FPopcornFXSubRendererMaterial &mat, const PopcornFX::SRendererDeclaration &decl)
	{
		mat.Lit = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Lit());
		EPopcornFXDefaultMaterialType	finalMatType = mat.DefaultMaterialType;
		if (mat.Lit)
		{
			mat.CastShadow = decl.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_Lit_CastShadows(), false);
			mat.TextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_NormalMap(), PopcornFX::CString::EmptyString));
			mat.TextureRoughMetal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_RoughMetalMap(), PopcornFX::CString::EmptyString));
			mat.Roughness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Roughness(), 1.0f);
			mat.Metalness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Metalness(), 0.0f);
		
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Transparent()))
		{
			finalMatType = EPopcornFXDefaultMaterialType::Billboard_AlphaBlendAdditive;
			mat.DrawOrder = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Transparent_GlobalSortOverride(), 0);
			if (!decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Emissive()))
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_AlphaBlend;
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Opaque()))
		{
			finalMatType = kOpaqueBillboard_DefaultMaterial_ToUE[decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Opaque_Type(), 0)];
			if (finalMatType == EPopcornFXDefaultMaterialType::Billboard_Masked)
				mat.MaskThreshold = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Opaque_MaskThreshold(), 0.0f);
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Emissive())) 
		{
			mat.TextureEmissive = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveMap(), PopcornFX::CString::EmptyString));
			if (!decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Diffuse()))
			{
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_Additive;
				mat.NoAlpha = true;
			}
		}	
			
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Diffuse()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseMap(), PopcornFX::CString::EmptyString));
			if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_DiffuseRamp()))
				mat.TextureDiffuseRamp = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_DiffuseRamp_RampMap(), PopcornFX::CString::EmptyString));
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Distortion()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Distortion_DistortionMap(), PopcornFX::CString::EmptyString));
			finalMatType = EPopcornFXDefaultMaterialType::Billboard_Distortion;

			if (mat.TextureDiffuse != null &&
				mat.TextureDiffuse->CompressionSettings != TextureCompressionSettings::TC_Normalmap)
			{
				FText		title = LOCTEXT("import_distortion_texture_title", "PopcornFX: Compression Settings");
				const FText	message = FText::Format(LOCTEXT("import_distortion_texture_message",
					"Texture \"{0}\" is used for distortion but has incompatible compression settings. Do you want to import it using NormalMap compression settings instead?"),
					FText::FromString(mat.TextureDiffuse->GetPathName()));

				if (OpenMessageBox(EAppMsgType::YesNo, message, title) == EAppReturnType::Yes)
				{
					mat.TextureDiffuse->Modify();
					mat.TextureDiffuse->CompressionSettings = TextureCompressionSettings::TC_Normalmap;
				}
			}
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Atlas()))
		{
			mat.TextureAtlas = LoadAtlasPk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Atlas_Definition(), PopcornFX::CString::EmptyString));

			const s32	blendingType = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Atlas_Blending(), 0); // 1 = LinearAtlasBlending. 2 = MotionVectors blending (optical flow);
			mat.SoftAnimBlending = blendingType == 1;
			mat.MotionVectorsBlending = blendingType == 2;
			if (mat.MotionVectorsBlending)
			{
				mat.TextureMotionVectors = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Atlas_MotionVectorsMap(), PopcornFX::CString::EmptyString));

				const CFloat2	distortionStrength = decl.GetPropertyValue_F2(PopcornFX::BasicRendererProperties::SID_Atlas_DistortionStrength(), CFloat2::ZERO);
				mat.MVDistortionStrengthColumns = distortionStrength.x();
				mat.MVDistortionStrengthRows = distortionStrength.y();
			}
		}
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_AlphaRemap()))
			mat.TextureAlphaRemapper = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_AlphaRemap_AlphaMap(), PopcornFX::CString::EmptyString));
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_SoftParticles()))
			mat.SoftnessDistance = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_SoftParticles_SoftnessDistance(), 0.0f);

		switch (finalMatType)
		{
#if 0
		case	EPopcornFXDefaultMaterialType::Billboard_Additive_NoAlpha:
		{
			finalMatType = EPopcornFXLegacyMaterialType::Billboard_Additive;
			mat.NoAlpha = true;
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
#endif
		case	EPopcornFXDefaultMaterialType::Billboard_Additive:
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_AlphaBlend:
			if (mat.Lit)
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_AlphaBlend_Lit;
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_AlphaBlendAdditive:
			if (mat.Lit)
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_AlphaBlendAdditive_Lit;
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_Distortion:
			PK_ASSERT(!mat.Lit);
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_Solid:
			if (mat.Lit)
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_Solid_Lit;
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_Masked:
			if (mat.Lit)
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_Masked_Lit;
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			return false;
		}
		mat.DefaultMaterialType = finalMatType;
		return true;
	}

	bool		RM_Setup_Ribbon(UPopcornFXRendererMaterial *self, const void *rendererBase)
	{
		const PopcornFX::CRendererDataBase	*_rendererBase = static_cast<const PopcornFX::CRendererDataBase*>(rendererBase);
		if (_rendererBase->m_RendererType != PopcornFX::Renderer_Ribbon)
			return false;
		const PopcornFX::CRendererDataRibbon	*renderer = static_cast<const PopcornFX::CRendererDataRibbon*>(_rendererBase);
		if (renderer == null)
			return false;
		self->SubMaterials.SetNum(1);
		FPopcornFXSubRendererMaterial	&mat = self->SubMaterials[0];

		const PopcornFX::SRendererDeclaration		&decl = renderer->m_Declaration;

		

		// Default values:
		mat.DefaultMaterialType = EPopcornFXDefaultMaterialType::Billboard_Additive;
		mat.LegacyMaterialType = EPopcornFXLegacyMaterialType::Billboard_Additive;
		mat.CorrectDeformation = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_CorrectDeformation());
		mat.IsLegacy = decl.FindAdditionalFieldDefinition(PopcornFX::CStringId("Diffuse.Color")) != null
						|| (decl.m_MaterialPath.Contains("Legacy", PopcornFX::CaseInsensitive)
							&& decl.m_MaterialPath.Contains("PopcornFXCore", PopcornFX::CaseInsensitive));

		bool success = true;

		if (mat.IsLegacy)
			success &= RM_Setup_Ribbon_Legacy(mat, decl);
		else
			success &= RM_Setup_Ribbon_Default(mat, decl);

		mat.SortIndices = mat.NeedsSorting();

		mat.BuildDynamicParameterMask(_rendererBase, decl);
		return true;
	}

	bool		RM_Setup_Mesh_Legacy(FPopcornFXSubRendererMaterial &mat, const PopcornFX::SRendererDeclaration &decl)
	{
		const bool	legacyLit = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_LegacyLit());
		const bool	fluidVAT = decl.IsFeatureEnabled(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid());
		const bool	softVAT = decl.IsFeatureEnabled(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft());
		const bool	rigidVAT = decl.IsFeatureEnabled(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid());
		const bool	normalizedVAT = decl.IsFeatureEnabled(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_NormalizedData());

		const bool	skelMesh = decl.IsFeatureEnabled(PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimation());
		if (skelMesh)
			mat.SkeletalMesh = LoadSkelMeshPk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Mesh(), PopcornFX::CString::EmptyString));
		else
		{
			mat.StaticMesh = LoadMeshPk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Mesh(), PopcornFX::CString::EmptyString));
			mat.StaticMeshLOD = 0; // TODO
		}

		mat.Lit = legacyLit || decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Lit());
		EPopcornFXLegacyMaterialType type = mat.LegacyMaterialType;

		if (mat.Lit)
		{
			if (legacyLit)
			{
				mat.CastShadow = decl.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_LegacyLit_CastShadows(), false);
				mat.TextureSpecular = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_LegacyLit_SpecularMap(), PopcornFX::CString::EmptyString));
				mat.TextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_LegacyLit_NormalMap(), PopcornFX::CString::EmptyString));
			}
			else
			{
				mat.CastShadow = decl.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_Lit_CastShadows(), false);
				mat.TextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_NormalMap(), PopcornFX::CString::EmptyString));
				mat.TextureRoughMetal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_RoughMetalMap(), PopcornFX::CString::EmptyString));
				mat.Roughness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Roughness(), 1.0f);
				mat.Metalness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Metalness(), 0.0f);
			}
		}

		if (fluidVAT)
		{
			mat.VATTexturePosition = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_PositionMap(), PopcornFX::CString::EmptyString));
			mat.VATTextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_NormalMap(), PopcornFX::CString::EmptyString));
			mat.VATTextureColor = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_ColorMap(), PopcornFX::CString::EmptyString));
			mat.VATNumFrames = decl.GetPropertyValue_I1(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_NumFrames(), 0);
			mat.VATPackedData = decl.GetPropertyValue_B(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_PackedData(), false);
		}
		else if (softVAT)
		{
			mat.VATTexturePosition = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft_PositionMap(), PopcornFX::CString::EmptyString));
			mat.VATTextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft_NormalMap(), PopcornFX::CString::EmptyString));
			mat.VATNumFrames = decl.GetPropertyValue_I1(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft_NumFrames(), 0);
			mat.VATPackedData = decl.GetPropertyValue_B(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft_PackedData(), false);
		}
		else if (rigidVAT)
		{
			mat.VATTexturePosition = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid_PositionMap(), PopcornFX::CString::EmptyString));
			mat.VATTextureRotation = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid_RotationMap(), PopcornFX::CString::EmptyString));
			mat.VATNumFrames = decl.GetPropertyValue_I1(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid_NumFrames(), 0);

			const float		globalScale = FPopcornFXPlugin::GlobalScale();
			const CFloat2	pivotBounds = decl.GetPropertyValue_F2(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid_BoundsPivot(), CFloat2::ZERO);
			mat.VATPivotBoundsMin = pivotBounds.x() * globalScale;
			mat.VATPivotBoundsMax = pivotBounds.y() * globalScale;
		}
		if ((fluidVAT || softVAT || rigidVAT) &&
			normalizedVAT)
		{
			mat.VATNormalizedData = true;

			const float		globalScale = FPopcornFXPlugin::GlobalScale();
			const CFloat2	positionBounds = decl.GetPropertyValue_F2(PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_BoundsPosition(), CFloat2::ZERO);
			mat.VATPositionBoundsMin = positionBounds.x() * globalScale;
			mat.VATPositionBoundsMax = positionBounds.y() * globalScale;
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Atlas()))
			mat.TextureAtlas = LoadAtlasPk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Atlas_Definition(), PopcornFX::CString::EmptyString));

		if (fluidVAT || softVAT || rigidVAT)
		{
			const FText	title = LOCTEXT("import_VAT_asset_title", "PopcornFX: Asset used for vertex animation");
			if (mat.VATTexturePosition != null &&
				!_IsVATTextureConfigured(mat.VATTexturePosition))
			{
				const FText	message = FText::Format(LOCTEXT("import_VAT_asset_message", "Texture \"{0}\" is used for vertex animation. Do you want to import it using VAT compatible settings?"), FText::FromString(mat.VATTexturePosition->GetPathName()));
				if (OpenMessageBox(EAppMsgType::YesNo, message, title) == EAppReturnType::Yes)
				{
					mat.VATTexturePosition->Modify();
					mat.VATTexturePosition->Filter = TF_Nearest;
				}
			}
			if (mat.VATTextureNormal != null &&
				!_IsVATTextureConfigured(mat.VATTextureNormal))
			{
				const FText	message = FText::Format(LOCTEXT("import_VAT_asset_message", "Texture \"{0}\" is used for vertex animation. Do you want to import it using VAT compatible settings?"), FText::FromString(mat.VATTextureNormal->GetPathName()));
				if (OpenMessageBox(EAppMsgType::YesNo, message, title) == EAppReturnType::Yes)
				{
					mat.VATTextureNormal->Modify();
					mat.VATTextureNormal->Filter = TF_Nearest;
				}
			}
			if (mat.VATTextureColor != null &&
				!_IsVATTextureConfigured(mat.VATTextureColor))
			{
				const FText	message = FText::Format(LOCTEXT("import_VAT_asset_message", "Texture \"{0}\" is used for vertex animation. Do you want to import it using VAT compatible settings?"), FText::FromString(mat.VATTextureColor->GetPathName()));
				if (OpenMessageBox(EAppMsgType::YesNo, message, title) == EAppReturnType::Yes)
				{
					mat.VATTextureColor->Modify();
					mat.VATTextureColor->Filter = TF_Nearest;
				}
			}
			if (mat.VATTextureRotation != null &&
				!_IsVATTextureConfigured(mat.VATTextureRotation))
			{
				const FText	message = FText::Format(LOCTEXT("import_VAT_asset_message", "Texture \"{0}\" is used for vertex animation. Do you want to import it using VAT compatible settings?"), FText::FromString(mat.VATTextureRotation->GetPathName()));
				if (OpenMessageBox(EAppMsgType::YesNo, message, title) == EAppReturnType::Yes)
				{
					mat.VATTextureRotation->Modify();
					mat.VATTextureRotation->Filter = TF_Nearest;
				}
			}
			if (mat.StaticMesh != null)
			{
				if (rigidVAT)
				{
					// The info isn't stored in the serialized mesh, force a reimport whenever the effect is reimported.
					UFbxStaticMeshImportData*	meshImportData = Cast<UFbxStaticMeshImportData>(mat.StaticMesh->AssetImportData);
					meshImportData->Modify();
					meshImportData->VertexColorImportOption = EVertexColorImportOption::Replace;
					UReimportFbxStaticMeshFactory*	FbxStaticMeshReimportFactory = NewObject<UReimportFbxStaticMeshFactory>(UReimportFbxStaticMeshFactory::StaticClass());
					FbxStaticMeshReimportFactory->Reimport(mat.StaticMesh);
				}
				if (mat.StaticMesh != null &&
					!_IsVATMeshConfigured(mat.StaticMesh))
				{
					const FText	message = FText::Format(LOCTEXT("import_VAT_asset_message", "Mesh \"{0}\" is used for vertex animation. Do you want to import it using VAT compatible settings settings?"), FText::FromString(mat.StaticMesh->GetPathName()));
					if (OpenMessageBox(EAppMsgType::YesNo, message, title) == EAppReturnType::Yes)
					{
						mat.StaticMesh->Modify();
						mat.StaticMesh->GetSourceModel(0).BuildSettings.bUseFullPrecisionUVs = true;
						mat.StaticMesh->GetSourceModel(0).BuildSettings.bRemoveDegenerates = false;
						mat.StaticMesh->Build();
					}
				}
			}
		}
		else
		{
			// Alpha remap only supported for non VAT materials.
			if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_AlphaRemap()))
				mat.TextureAlphaRemapper = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_AlphaRemap_AlphaMap(), PopcornFX::CString::EmptyString));
		}
		if (skelMesh && mat.SkeletalMesh != null)
		{
			mat.TextureSkeletalAnimation = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimation_AnimationTexture(), PopcornFX::CString::EmptyString));
			mat.SkeletalAnimationCount = decl.GetPropertyValue_I1(PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimation_AnimTracksCount(), 0);
			mat.SkeletalAnimationPosBoundsMin = FVector(ToUE(decl.GetPropertyValue_F3(PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimation_AnimPositionsBoundsMin(), PopcornFX::CFloat3::ZERO)));
			mat.SkeletalAnimationPosBoundsMax = FVector(ToUE(decl.GetPropertyValue_F3(PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimation_AnimPositionsBoundsMax(), PopcornFX::CFloat3::ZERO)));
			mat.SkeletalAnimationLinearInterpolate = decl.IsFeatureEnabled(PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimationInterpolate());
			mat.SkeletalAnimationLinearInterpolateTracks = decl.IsFeatureEnabled(PopcornFX::SkeletalAnimationTexture::SID_SkeletalAnimationInterpolateTracks());
			if (!mat.BuildSkelMeshBoneIndicesReorder())
				return false;
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Opaque()))
		{
			const u32 OpaqueType = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Opaque_Type(), 0);

			// * 2 because unlit/lit pair per enum value
			if (fluidVAT)
				type = static_cast<EPopcornFXLegacyMaterialType>((uint32)EPopcornFXLegacyMaterialType::Mesh_VAT_Opaque_Fluid + OpaqueType * 2);
			else if (softVAT)
				type = static_cast<EPopcornFXLegacyMaterialType>((uint32)EPopcornFXLegacyMaterialType::Mesh_VAT_Opaque_Soft + OpaqueType * 2);
			else if (rigidVAT)
				type = static_cast<EPopcornFXLegacyMaterialType>((uint32)EPopcornFXLegacyMaterialType::Mesh_VAT_Opaque_Rigid + OpaqueType * 2);
			else
				type = kOpaqueMesh_LegacyMaterial_ToUE[OpaqueType];

			if (static_cast<PopcornFX::BasicRendererProperties::EOpaqueType>(OpaqueType) == PopcornFX::BasicRendererProperties::EOpaqueType::Masked)
				mat.MaskThreshold = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Opaque_MaskThreshold(), 0.0f);

			if (mat.Lit)
				type = static_cast<EPopcornFXLegacyMaterialType>((uint32)type + 1); // Next enum index
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Transparent()))
		{
			if (fluidVAT || softVAT || rigidVAT)
				return false; // unsupported
			else
			{
				// For now, only support unlit alpha blended & additive meshes.
				if (decl.GetPropertyValue_Enum<PopcornFX::BasicRendererProperties::ETransparentType>(PopcornFX::BasicRendererProperties::SID_Transparent_Type(), PopcornFX::BasicRendererProperties::Additive) == PopcornFX::BasicRendererProperties::AlphaBlend)
					type = EPopcornFXLegacyMaterialType::Mesh_AlphaBlend;
				else
					type = EPopcornFXLegacyMaterialType::Mesh_Additive;
			}

			mat.DrawOrder = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Transparent_GlobalSortOverride(), 0);
			mat.Lit = false;
		}
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Diffuse()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseMap(), PopcornFX::CString::EmptyString));
			if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_DiffuseRamp()))
				mat.TextureDiffuseRamp = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_DiffuseRamp_RampMap(), PopcornFX::CString::EmptyString));
		}
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Emissive()))
			mat.TextureEmissive = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveMap(), PopcornFX::CString::EmptyString));

		mat.LegacyMaterialType = type;

		return true;
	
	}

	bool		RM_Setup_Mesh_Default(FPopcornFXSubRendererMaterial &mat, const PopcornFX::SRendererDeclaration &decl)
	{
		mat.StaticMesh = LoadMeshPk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Mesh(), PopcornFX::CString::EmptyString));
		mat.StaticMeshLOD = 0; // TODO

		mat.Lit = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Lit());
		EPopcornFXDefaultMaterialType type = EPopcornFXDefaultMaterialType::Mesh_Additive;

		if (mat.Lit)
		{
			mat.CastShadow = decl.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_Lit_CastShadows(), false);
			mat.TextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_NormalMap(), PopcornFX::CString::EmptyString));
			mat.TextureRoughMetal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_RoughMetalMap(), PopcornFX::CString::EmptyString));
			mat.Roughness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Roughness(), 1.0f);
			mat.Metalness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Metalness(), 0.0f);
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Atlas()))
			mat.TextureAtlas = LoadAtlasPk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Atlas_Definition(), PopcornFX::CString::EmptyString));

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_AlphaRemap()))
			mat.TextureAlphaRemapper = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_AlphaRemap_AlphaMap(), PopcornFX::CString::EmptyString));

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Opaque()))
		{
			type = kOpaqueMesh_DefaultMaterial_ToUE[decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Opaque_Type(), 0)];

			if (type == EPopcornFXDefaultMaterialType::Mesh_Masked)
				mat.MaskThreshold = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Opaque_MaskThreshold(), 0.0f);

			if (mat.Lit)
				type = static_cast<EPopcornFXDefaultMaterialType>((uint32)type + 1); // Next enum index
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Transparent()))
		{
			type = EPopcornFXDefaultMaterialType::Mesh_AlphaBlend;
			mat.DrawOrder = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Transparent_GlobalSortOverride(), 0);
			mat.Lit = false;
		}
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Diffuse()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseMap(), PopcornFX::CString::EmptyString));
			if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_DiffuseRamp()))
				mat.TextureDiffuseRamp = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_DiffuseRamp_RampMap(), PopcornFX::CString::EmptyString));
		}
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Emissive()))
		{
			if (!decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Diffuse()))
			{
				type = EPopcornFXDefaultMaterialType::Mesh_Additive;
				mat.NoAlpha = true;
			}
			mat.TextureEmissive = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveMap(), PopcornFX::CString::EmptyString)); 
		}
			
		mat.DefaultMaterialType = type;
		return true;
	}

	bool		RM_Setup_Mesh(UPopcornFXRendererMaterial *self, const void *rendererBase)
	{
		const PopcornFX::CRendererDataBase	*_rendererBase = static_cast<const PopcornFX::CRendererDataBase*>(rendererBase);
		if (_rendererBase->m_RendererType != PopcornFX::Renderer_Mesh)
			return false;
		const PopcornFX::CRendererDataMesh	*renderer = static_cast<const PopcornFX::CRendererDataMesh*>(_rendererBase);
		if (renderer == null)
			return false;

		// TODO: Remove sub materials
		self->SubMaterials.SetNum(1);
		FPopcornFXSubRendererMaterial	&mat = self->SubMaterials[0];

		const PopcornFX::SRendererDeclaration		&decl = renderer->m_Declaration;

		// Default values:
		mat.LegacyMaterialType = EPopcornFXLegacyMaterialType::Mesh_Solid;
		mat.DefaultMaterialType = EPopcornFXDefaultMaterialType::Mesh_Solid;
		mat.IsLegacy = decl.FindAdditionalFieldDefinition(PopcornFX::CStringId("Diffuse.Color")) != null
						|| (decl.m_MaterialPath.Contains("Legacy", PopcornFX::CaseInsensitive)
							&& decl.m_MaterialPath.Contains("PopcornFXCore", PopcornFX::CaseInsensitive));
		mat.PerParticleLOD = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_MeshLOD());
		mat.MotionBlur = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_MotionBlur());
		mat.MeshAtlas = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_MeshAtlas());
		mat.Raytraced = decl.IsFeatureEnabled(PopcornFX::CStringId("Raytraced")); // tmp

		if (mat.IsLegacy)
			RM_Setup_Mesh_Legacy(mat, decl);
		else 
			RM_Setup_Mesh_Default(mat, decl);

		mat.BuildDynamicParameterMask(_rendererBase, decl);
		return true;
	}

	bool		RM_Params_Mesh(UPopcornFXRendererMaterial *self, UMaterialInstanceDynamic *material, uint32 subMaterialIndex)
	{
		check(subMaterialIndex < uint32(self->SubMaterials.Num()));
		FPopcornFXSubRendererMaterial		&mat = self->SubMaterials[subMaterialIndex];
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_DiffuseTexture(), mat.TextureDiffuse);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_DiffuseTextureRamp(), mat.TextureDiffuseRamp);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_EmissiveTexture(), mat.TextureEmissive);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_NormalTexture(), mat.TextureNormal);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_RoughMetalTexture(), mat.TextureRoughMetal);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_SpecularTexture(), mat.TextureSpecular);

		material->SetTextureParameterValue(FPopcornFXPlugin::Name_VATPositionTexture(), mat.VATTexturePosition);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_VATNormalTexture(), mat.VATTextureNormal);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_VATColorTexture(), mat.VATTextureColor);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_VATRotationTexture(), mat.VATTextureRotation);

		material->SetScalarParameterValue(FPopcornFXPlugin::Name_VATNumFrames(), mat.VATNumFrames);
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_VATPivotBoundsMin(), mat.VATPivotBoundsMin);
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_VATPivotBoundsMax(), mat.VATPivotBoundsMax);
		if (mat.VATNormalizedData)
		{
			material->SetScalarParameterValue(FPopcornFXPlugin::Name_VATPositionBoundsMin(), mat.VATPositionBoundsMin);
			material->SetScalarParameterValue(FPopcornFXPlugin::Name_VATPositionBoundsMax(), mat.VATPositionBoundsMax);
		}
		
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_AlphaRemapper(), mat.TextureAlphaRemapper);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_MotionVectorsTexture(), mat.TextureMotionVectors);
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_Roughness(), mat.Roughness);
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_Metalness(), mat.Metalness);
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_SoftnessDistance(), mat.SoftnessDistance * FPopcornFXPlugin::GlobalScale());
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_MaskThreshold(), mat.MaskThreshold);
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_MVDistortionStrengthColumns(), mat.MVDistortionStrengthColumns);
		material->SetScalarParameterValue(FPopcornFXPlugin::Name_MVDistortionStrengthRows(), mat.MVDistortionStrengthRows);
		return true;
	}

	bool		RM_ParamsConst_Mesh(UPopcornFXRendererMaterial *self, UMaterialInstanceConstant *material, uint32 subMaterialIndex)
	{
		check(subMaterialIndex < uint32(self->SubMaterials.Num()));
		FPopcornFXSubRendererMaterial		&mat = self->SubMaterials[subMaterialIndex];

		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_DiffuseTexture(), mat.TextureDiffuse);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_DiffuseTextureRamp(), mat.TextureDiffuseRamp);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_EmissiveTexture(), mat.TextureEmissive);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_NormalTexture(), mat.TextureNormal);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_RoughMetalTexture(), mat.TextureRoughMetal);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_SpecularTexture(), mat.TextureSpecular);

		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_VATPositionTexture(), mat.VATTexturePosition);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_VATNormalTexture(), mat.VATTextureNormal);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_VATColorTexture(), mat.VATTextureColor);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_VATRotationTexture(), mat.VATTextureRotation);

		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_VATNumFrames(), mat.VATNumFrames);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_VATPivotBoundsMin(), mat.VATPivotBoundsMin);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_VATPivotBoundsMax(), mat.VATPivotBoundsMax);
		if (mat.VATNormalizedData)
		{
			material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_VATPositionBoundsMin(), mat.VATPositionBoundsMin);
			material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_VATPositionBoundsMax(), mat.VATPositionBoundsMax);
		}

		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_AlphaRemapper(), mat.TextureAlphaRemapper);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_MotionVectorsTexture(), mat.TextureMotionVectors);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_Roughness(), mat.Roughness);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_Metalness(), mat.Metalness);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_SoftnessDistance(), mat.SoftnessDistance * FPopcornFXPlugin::GlobalScale());
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_MaskThreshold(), mat.MaskThreshold);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_MVDistortionStrengthColumns(), mat.MVDistortionStrengthColumns);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_MVDistortionStrengthRows(), mat.MVDistortionStrengthRows);

		_SetStaticSwitches_Common(material, mat);

		// !!
		material->PostEditChange();
		return true;
	}

	bool		RM_Setup_Triangle_Legacy(FPopcornFXSubRendererMaterial& mat, const PopcornFX::SRendererDeclaration &decl)
	{
		const bool	legacyLit = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_LegacyLit());
		mat.Lit = legacyLit || decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Lit());
		EPopcornFXLegacyMaterialType	finalMatType = mat.LegacyMaterialType;

		if (legacyLit)
		{
			mat.CastShadow = decl.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_LegacyLit_CastShadows(), false);
			mat.TextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_LegacyLit_NormalMap(), PopcornFX::CString::EmptyString));
		}
		else if (mat.Lit)
		{
			mat.CastShadow = decl.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_Lit_CastShadows(), false);
			mat.TextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_NormalMap(), PopcornFX::CString::EmptyString));
			mat.TextureRoughMetal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_RoughMetalMap(), PopcornFX::CString::EmptyString));
			mat.Roughness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Roughness(), 1.0f);
			mat.Metalness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Metalness(), 0.0f);
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Transparent()))
		{
			finalMatType = kLegacyTransparentBillboard_Material_ToUE[decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Transparent_Type(), 0)];
			mat.DrawOrder = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Transparent_GlobalSortOverride(), 0);
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Opaque()))
		{
			finalMatType = kOpaqueBillboard_LegacyMaterial_ToUE[decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Opaque_Type(), 0)];
			if (finalMatType == EPopcornFXLegacyMaterialType::Billboard_Masked)
				mat.MaskThreshold = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Opaque_MaskThreshold(), 0.0f);
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Emissive()))
			mat.TextureEmissive = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveMap(), PopcornFX::CString::EmptyString));
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Diffuse()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseMap(), PopcornFX::CString::EmptyString));
			if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_DiffuseRamp()))
				mat.TextureDiffuseRamp = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_DiffuseRamp_RampMap(), PopcornFX::CString::EmptyString));
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Distortion()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Distortion_DistortionMap(), PopcornFX::CString::EmptyString));
			finalMatType = EPopcornFXLegacyMaterialType::Mesh_Distortion;

			if (mat.TextureDiffuse != null &&
				mat.TextureDiffuse->CompressionSettings != TextureCompressionSettings::TC_Normalmap)
			{
				FText		title = LOCTEXT("import_distortion_texture_title", "PopcornFX: Compression Settings");
				const FText	message = FText::Format(LOCTEXT("import_distortion_texture_message",
					"Texture \"{0}\" is used for distortion but has incompatible compression settings. Do you want to import it using NormalMap compression settings instead?"),
					FText::FromString(mat.TextureDiffuse->GetPathName()));

				if (OpenMessageBox(EAppMsgType::YesNo, message, title) == EAppReturnType::Yes)
				{
					mat.TextureDiffuse->Modify();
					mat.TextureDiffuse->CompressionSettings = TextureCompressionSettings::TC_Normalmap;
				}
			}
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_AlphaRemap()))
			mat.TextureAlphaRemapper = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_AlphaRemap_AlphaMap(), PopcornFX::CString::EmptyString));


		switch (finalMatType)
		{
		case	EPopcornFXLegacyMaterialType::Billboard_Additive_NoAlpha:
		{
			finalMatType = EPopcornFXLegacyMaterialType::Billboard_Additive;
			mat.NoAlpha = true;
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		}
		case	EPopcornFXLegacyMaterialType::Billboard_Additive:
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_AlphaBlend:
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_AlphaBlendAdditive:
			if (mat.Lit)
				finalMatType = EPopcornFXLegacyMaterialType::Billboard_AlphaBlendAdditive_Lit;
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_Distortion:
			PK_ASSERT(!mat.Lit);
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_Solid:
			if (mat.Lit)
				finalMatType = EPopcornFXLegacyMaterialType::Billboard_Solid_Lit;
			break;
		case	EPopcornFXLegacyMaterialType::Billboard_Masked:
			if (mat.Lit)
				finalMatType = EPopcornFXLegacyMaterialType::Billboard_Masked_Lit;
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			return false;
		}
		mat.LegacyMaterialType = finalMatType;
		return true;
	}

	bool		RM_Setup_Triangle_Default(FPopcornFXSubRendererMaterial& mat, const PopcornFX::SRendererDeclaration &decl)
	{
		mat.Lit = decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Lit());
		EPopcornFXDefaultMaterialType	finalMatType = mat.DefaultMaterialType;

		if (mat.Lit)
		{
			mat.CastShadow = decl.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_Lit_CastShadows(), false);
			mat.TextureNormal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_NormalMap(), PopcornFX::CString::EmptyString));
			mat.TextureRoughMetal = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Lit_RoughMetalMap(), PopcornFX::CString::EmptyString));
			mat.Roughness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Roughness(), 1.0f);
			mat.Metalness = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Lit_Metalness(), 0.0f);
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Transparent()))
		{
			finalMatType = EPopcornFXDefaultMaterialType::Billboard_AlphaBlend;
			mat.DrawOrder = decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Transparent_GlobalSortOverride(), 0);
		}
		else if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Opaque()))
		{
			finalMatType = kOpaqueBillboard_DefaultMaterial_ToUE[decl.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_Opaque_Type(), 0)];
			if (finalMatType == EPopcornFXDefaultMaterialType::Billboard_Masked)
				mat.MaskThreshold = decl.GetPropertyValue_F1(PopcornFX::BasicRendererProperties::SID_Opaque_MaskThreshold(), 0.0f);
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Emissive()))
		{
			mat.TextureEmissive = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveMap(), PopcornFX::CString::EmptyString));
			if (!decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Diffuse()))
			{
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_Additive;
				mat.NoAlpha = true;
			}
		}
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_Diffuse()))
		{
			mat.TextureDiffuse = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseMap(), PopcornFX::CString::EmptyString));
			if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_DiffuseRamp()))
				mat.TextureDiffuseRamp = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_DiffuseRamp_RampMap(), PopcornFX::CString::EmptyString));
		}

		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_AlphaRemap()))
			mat.TextureAlphaRemapper = LoadTexturePk(decl.GetPropertyValue_Path(PopcornFX::BasicRendererProperties::SID_AlphaRemap_AlphaMap(), PopcornFX::CString::EmptyString));

		switch (finalMatType)
		{
#if 0
		case	EPopcornFXDefaultMaterialType::Billboard_Additive_NoAlpha:
		{
			finalMatType = EPopcornFXDefaultMaterialType::Billboard_Additive;
			mat.NoAlpha = true;
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		}
#endif
		case	EPopcornFXDefaultMaterialType::Billboard_Additive:
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_AlphaBlend:
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_AlphaBlendAdditive:
			if (mat.Lit)
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_AlphaBlendAdditive_Lit;
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_Distortion:
			PK_ASSERT(!mat.Lit);
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_Solid:
			if (mat.Lit)
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_Solid_Lit;
			break;
		case	EPopcornFXDefaultMaterialType::Billboard_Masked:
			if (mat.Lit)
				finalMatType = EPopcornFXDefaultMaterialType::Billboard_Masked_Lit;
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			return false;
		}
		mat.DefaultMaterialType = finalMatType;
		return true;
	}

	bool		RM_Setup_Triangle(UPopcornFXRendererMaterial *self, const void *rendererBase)
	{
		const PopcornFX::CRendererDataBase	*_rendererBase = static_cast<const PopcornFX::CRendererDataBase*>(rendererBase);
		if (_rendererBase->m_RendererType != PopcornFX::Renderer_Triangle)
			return false;

		const PopcornFX::CRendererDataTriangle	*renderer = static_cast<const PopcornFX::CRendererDataTriangle*>(_rendererBase);
		if (renderer == null)
			return false;

		self->SubMaterials.SetNum(1);
		FPopcornFXSubRendererMaterial	&mat = self->SubMaterials[0];

		const PopcornFX::SRendererDeclaration		&decl = renderer->m_Declaration;
		

		mat.LegacyMaterialType = EPopcornFXLegacyMaterialType::Billboard_Additive;
		mat.DefaultMaterialType = EPopcornFXDefaultMaterialType::Billboard_Additive;
		mat.IsLegacy = decl.FindAdditionalFieldDefinition(PopcornFX::CStringId("Diffuse.Color")) != null
						|| (decl.m_MaterialPath.Contains("Legacy", PopcornFX::CaseInsensitive)
							&& decl.m_MaterialPath.Contains("PopcornFXCore", PopcornFX::CaseInsensitive));

		bool success = true;
		if (mat.IsLegacy)
			success &= RM_Setup_Triangle_Legacy(mat, decl);
		else
			success &= RM_Setup_Triangle_Default(mat, decl); 

		mat.SortIndices = mat.NeedsSorting();

		mat.BuildDynamicParameterMask(_rendererBase, decl);
		return true;
	}

#endif // WITH_EDITOR

#if WITH_EDITOR
#	define RENDERMATCALLBACKS(__setup, __params)	{ &RM_Setup_ ## __setup, &RM_ParamsConst_ ## __params }
#else
#	define RENDERMATCALLBACKS(__setup, __params)	{ }
#endif

	const FRendererMaterialsFuncs		kRendererMaterialFuncs[] =
	{
		RENDERMATCALLBACKS(Billboard, Billboard), // RMID_Billboard
		RENDERMATCALLBACKS(Ribbon, Billboard), // RMID_Ribbon
		RENDERMATCALLBACKS(Mesh, Mesh), // RMID_Mesh
		RENDERMATCALLBACKS(Triangle, Billboard), // RMID_Triangle

		// ONLY APPEND NEW RENDERERS (or it will mess-up ids)
	};

	PK_STATIC_ASSERT(_RMID_Count == PK_ARRAY_COUNT(kRendererMaterialFuncs));
}

//----------------------------------------------------------------------------
//
// FPopcornFXSubRendererMaterial
//
//----------------------------------------------------------------------------

FPopcornFXSubRendererMaterial::FPopcornFXSubRendererMaterial()
:	Material()
,	LegacyMaterialType(EPopcornFXLegacyMaterialType::Billboard_Additive)
,	DefaultMaterialType(EPopcornFXDefaultMaterialType::Billboard_Additive)
,	TextureDiffuse(null)
,	TextureDiffuseRamp(null)
,	TextureEmissive(null)
,	TextureNormal(null)
,	TextureRoughMetal(null)
,	TextureSpecular(null)
,	TextureAlphaRemapper(null)
,	TextureMotionVectors(null)
,	TextureSixWay_RLTS(null)
,	TextureSixWay_BBF(null)
,	VATTexturePosition(null)
,	VATTextureNormal(null)
,	VATTextureColor(null)
,	VATTextureRotation(null)
,	TextureSkeletalAnimation(null)
,	TextureAtlas(null)
,	NoAlpha(false)
,	MeshAtlas(false)
,	Raytraced(false)
,	SoftAnimBlending(false)
,	MotionVectorsBlending(false)
,	CastShadow(false)
,	Lit(false)
,	SortIndices(false)
,	CorrectDeformation(false)
,	Roughness(1)
,	Metalness(0)
,	SoftnessDistance(0)
,	SphereMaskHardness(100.0f)
,	MVDistortionStrengthColumns(1.0f)
,	MVDistortionStrengthRows(1.0f)
,	VATNumFrames(0)
,	VATPackedData(false)
,	VATPivotBoundsMin(0.0f)
,	VATPivotBoundsMax(0.0f)
,	VATNormalizedData(false)
,	VATPositionBoundsMin(0.0f)
,	VATPositionBoundsMax(0.0f)
,	SkeletalAnimationCount(0)
,	SkeletalAnimationPosBoundsMin(FVector::ZeroVector)
,	SkeletalAnimationPosBoundsMax(FVector::ZeroVector)
,	SkeletalAnimationLinearInterpolate(false)
,	SkeletalAnimationLinearInterpolateTracks(false)
,	MaskThreshold(0.0f)
,	DrawOrder(0)
,	DynamicParameterMask(0)
,	SkeletalMesh(null)
,	StaticMesh(null)
,	StaticMeshLOD(0)
,	PerParticleLOD(false)
,	MotionBlur(false)
,	IsLegacy(false)
,	m_RMId(-1)
,	MaterialInstance(null)
{

}

//----------------------------------------------------------------------------

FPopcornFXSubRendererMaterial::~FPopcornFXSubRendererMaterial()
{
#if WITH_EDITOR
	if (SkeletalMesh != null)
		SkeletalMesh->GetOnMeshChanged().RemoveAll(this);
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------------
#if WITH_EDITOR

bool	FPopcornFXSubRendererMaterial::_ResetDefaultMaterial_NoReload()
{
	if (IsLegacy)
		Material = FindLegacyMaterial();
	else
		Material = FindDefaultMaterial();
	return Material != null;
}

//----------------------------------------------------------------------------

namespace Test
{
	template <typename _TObj>
	_TObj		*__DuplicateObjectToPath(_TObj *source, const FString &dstPath)
	{
		if (source == null)
			return null;
		_TObj			*dupObj = null;
		UObject			*outer = null;
		FString			resPath = dstPath;
		if (ResolveName(outer, resPath, true, true))
			dupObj = DuplicateObject(source, outer, *resPath);
		if (dupObj != null)
		{
			dupObj->MarkPackageDirty();
			if (dupObj->GetOuter())
				dupObj->GetOuter()->MarkPackageDirty();
		}
		return dupObj;
	}
}
using namespace Test;

//----------------------------------------------------------------------------

UMaterialInterface		*FPopcornFXSubRendererMaterial::FindDefaultMaterial() const
{
	if (!PK_VERIFY(uint32(m_RMId) < PK_ARRAY_COUNT(kRendererMaterialFuncs)))
		return null;
	UMaterialInterface		*defaultMaterial = FPopcornFXPlugin::Get().Settings()->GetConfigDefaultMaterial((uint32)DefaultMaterialType);
	return defaultMaterial;
}

//----------------------------------------------------------------------------

UMaterialInterface		*FPopcornFXSubRendererMaterial::FindLegacyMaterial() const
{
	if (!PK_VERIFY(uint32(m_RMId) < PK_ARRAY_COUNT(kRendererMaterialFuncs)))
		return null;
	UMaterialInterface		*legacyMaterial = FPopcornFXPlugin::Get().Settings()->GetConfigLegacyMaterial((uint32)LegacyMaterialType);
	return legacyMaterial;
}

//----------------------------------------------------------------------------

void	FPopcornFXSubRendererMaterial::RenameDependenciesIFN(UObject* oldAsset, UObject* newAsset)
{
	PK_ASSERT(oldAsset != null);

#define	PK_RENAME_MAT_PROP_IFN(propName, propType) \
	if (propName == oldAsset) \
	{ \
		PK_ASSERT(newAsset == null || newAsset->IsA<propType>()); \
		propName->Modify(); \
		propName = Cast<propType>(newAsset); \
		propName->PostEditChange(); \
	}

	PK_RENAME_MAT_PROP_IFN(TextureDiffuse, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(TextureDiffuseRamp, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(TextureEmissive, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(TextureNormal, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(TextureRoughMetal, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(TextureSpecular, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(TextureAtlas, UPopcornFXTextureAtlas);
	PK_RENAME_MAT_PROP_IFN(TextureMotionVectors, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(TextureSixWay_RLTS, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(TextureSixWay_BBF, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(TextureSkeletalAnimation, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(VATTexturePosition, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(VATTextureNormal, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(VATTextureColor, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(VATTextureRotation, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(SkeletalMesh, USkeletalMesh);
	PK_RENAME_MAT_PROP_IFN(StaticMesh, UStaticMesh);

#undef PK_RENAME_MAT_PROP_IFN
}

//----------------------------------------------------------------------------

// Keeping this around for now, however enabling this generates invalid indices:
// Meshes imported in UE (sanitized by UE import pipe), exported to PK-Editor and used to generate a texture: PK-Editor considers unmapped bones to be mapped, probably due a to a UE export specificity.
// Meshes imported in UE from external packages (only affects some meshes), UE end up having a different mapped bone count compared to PK-Editor.
// Additionally, this does not current take into account all sections so the logic isn't correct.
#define _SKIP_UNMAPPED_BONES	0

namespace
{
	s16	_UEBoneIdToPK(/*FSkeletalMeshLODRenderData *renderData, */const TArray<FMeshBoneInfo> &refBoneInfos, u16 boneId, u16 currentBoneLevel, u16 &currentBoneId)
	{
		// This function generates a PKFX bone id based on UE's bone id.
		// In PopcornFX Editor (where the BAT is generated), bones ids are generated based on the bone name
		// This is only done when importing an effect or when the source skel mesh was modified in UE.
		PK_ASSERT(boneId < refBoneInfos.Num());
		PK_ASSERT(currentBoneLevel < refBoneInfos.Num());

		if (refBoneInfos[boneId] == refBoneInfos[currentBoneLevel])
			return currentBoneId;
		// Gather child bones
		TArray<const FMeshBoneInfo*>	childBones;
		for (s32 i = 0; i < refBoneInfos.Num(); ++i)
		{
			if (refBoneInfos[i].ParentIndex == currentBoneLevel)
				childBones.Push(&refBoneInfos[i]);
		}

		// Sort
		TArray<const FMeshBoneInfo*>	childBonesAlphabetical;
		for (s32 i = 0; i < childBones.Num(); ++i)
		{
			bool	inserted = false;
			for (s32 j = 0; j < childBonesAlphabetical.Num(); ++j)
			{
				if (childBonesAlphabetical[j]->Name.Compare(childBones[i]->Name) > 0)
				{
					childBonesAlphabetical.Insert(childBones[i], j);
					inserted = true;
					break;
				}
			}
			if (!inserted)
				childBonesAlphabetical.Push(childBones[i]);
		}
		PK_ASSERT(childBonesAlphabetical.Num() == childBones.Num());

		for (s32 i = 0; i < childBonesAlphabetical.Num(); ++i)
		{
			const s32	boneIdInMainMap = refBoneInfos.Find(*childBonesAlphabetical[i]);

	#if (_SKIP_UNMAPPED_BONES != 0)
			if (renderData->ActiveBoneIndices.Find(boneIdInMainMap) != -1)
				++currentBoneId;
	#endif

	#if (_SKIP_UNMAPPED_BONES != 0)
			const s16	foundBone = _UEBoneIdToPK(renderData, refBoneInfos, boneId, boneIdInMainMap, currentBoneId);
	#else
			const s16	foundBone = _UEBoneIdToPK(refBoneInfos, boneId, boneIdInMainMap, ++currentBoneId);
	#endif
			if (foundBone != -1)
				return foundBone;
		}
		return -1;
	}
}

//----------------------------------------------------------------------------

bool	FPopcornFXSubRendererMaterial::BuildSkelMeshBoneIndicesReorder()
{
	m_SkeletalMeshBoneIndicesReorder.Empty();
	if (SkeletalMesh == null)
		return true;

	const FSkeletalMeshRenderData	*skelMeshRenderData = SkeletalMesh->GetResourceForRendering();
	if (skelMeshRenderData == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Error, TEXT("Renderer material cannot be built, skeletal mesh '%s' does not contain any render data"), *SkeletalMesh->GetPathName());
		return false;
	}
	u32									skeletalMeshBoneMapSum = 0;
	const u32							baseLODLevel = 0; // TODO: BaseLODLevel
	const u32							LODCount = PerParticleLOD ? skelMeshRenderData->LODRenderData.Num() - baseLODLevel : 1; 
	for (u32 i = baseLODLevel; i < LODCount; ++i)
	{
		const FSkeletalMeshLODRenderData	&LODRenderData = skelMeshRenderData->LODRenderData[i];
		for (const FSkelMeshRenderSection &renderSection : LODRenderData.RenderSections)
		{
			PK_ASSERT(renderSection.BoneMap.Num() > 0);
			skeletalMeshBoneMapSum += renderSection.BoneMap.Num();
		}
	}
	if (skeletalMeshBoneMapSum == 0)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Error, TEXT("Renderer material cannot be built, skeletal mesh '%s' does not contain any mapped bones"), *SkeletalMesh->GetPathName());
		return false;
	}
	const FReferenceSkeleton	&refSkeleton = SkeletalMesh->GetRefSkeleton();
	const TArray<FMeshBoneInfo>	&boneInfos = refSkeleton.GetRefBoneInfo();

	m_SkeletalMeshBoneIndicesReorder.SetNum(skeletalMeshBoneMapSum);

	u32	*boneIndicesReorder = m_SkeletalMeshBoneIndicesReorder.GetData();
	u32	offset = 0;
	for (u32 i = baseLODLevel; i < LODCount; ++i)
	{
		const FSkeletalMeshLODInfo			*LODInfo = SkeletalMesh->GetLODInfo(i);
		const FSkeletalMeshLODRenderData	&LODRenderData = skelMeshRenderData->LODRenderData[i];
		for (u32 iSection = 0; iSection < (u32)LODRenderData.RenderSections.Num(); ++iSection)
		{
			const u32						renderSectionId = LODInfo != null && LODInfo->LODMaterialMap.Num() > 0 ? LODInfo->LODMaterialMap[iSection] : iSection;
			const FSkelMeshRenderSection	&renderSection = LODRenderData.RenderSections[renderSectionId];
			const u16						mappedBoneCount = renderSection.BoneMap.Num();

			if (PK_VERIFY(mappedBoneCount > 0))
			{
				for (u32 j = 0; j < mappedBoneCount; ++j)
				{
					u16	currentId = 0;
	#if (_SKIP_UNMAPPED_BONES != 0)
					boneIndicesReorder[offset + j] = _UEBoneIdToPK(LODRenderData, boneInfos, renderSection.BoneMap[j], 0, currentId);
	#else
					boneIndicesReorder[offset + j] = _UEBoneIdToPK(boneInfos, renderSection.BoneMap[j], 0, currentId);
	#endif
				}
				offset += mappedBoneCount;
			}
		}
	}
	return true;
}

#undef _SKIP_UNMAPPED_BONES

//----------------------------------------------------------------------------

void	FPopcornFXSubRendererMaterial::BuildDynamicParameterMask(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::SRendererDeclaration &decl)
{
	// Note: ShaderInput0 is mapped to DynamicParameter0 only for mesh particles (currently other renderers reserve it for the texture ID & alpha remap cursor in .xy)
	// This means that currently mesh renderers do not support alpha remap and linear/mv atlas blending
	// For mesh particles ShaderInput3.w is also mapped to alphaRect, which means you can only pass ShaderInput3.xyz on mesh particles.
	// FIXME harmonize how reserved dynamicParameters work, with a clear reserved dynamicParameter

	// In versions older than 5.3, certain binary mask checks in Unreal shaders ("DynamicParameterMask & 8" for example),
	// Seem to not get converted to hexadecimal mask checks (we get "& 0b1000" instead of "& 0xF000").
	// To account for that, mask values are set in binary instead of hexadecimal in UE 5.2 and below.
#define HEX_MASK (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 2)

	if (renderer->m_RendererType == PopcornFX::Renderer_Mesh)
	{
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_ShaderInput0()))
		{
			#if HEX_MASK
			DynamicParameterMask |= 0x000F;
			#else
			DynamicParameterMask |= 0b0001;
			#endif
		}
		#if HEX_MASK
		DynamicParameterMask |= 0x8000;
		#else
		DynamicParameterMask |= 0b1000;
		#endif
	}
	else
	{
		if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_AlphaRemap()) ||
			SoftAnimBlending || MotionVectorsBlending) // Atlas enabled, with motion vectors blending or linear frame blending
		{
			#if HEX_MASK
			DynamicParameterMask |= 0x0003;
			#else
			DynamicParameterMask |= 0b0001;
			#endif
		}
	}

	if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_ShaderInput1()))
	{
		#if HEX_MASK
		DynamicParameterMask |= 0x00F0;
		#else
		DynamicParameterMask |= 0b0010;
		#endif
	}
	if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_ShaderInput2()))
	{
		#if HEX_MASK
		DynamicParameterMask |= 0x0F00;
		#else
		DynamicParameterMask |= 0b0100;
		#endif
	}
	if (decl.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_ShaderInput3()))
	{
		#if HEX_MASK
		DynamicParameterMask |= 0xF000;
		#else
		DynamicParameterMask |= 0b1000;
		#endif
	}

#undef HEX_MASK
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

bool	FPopcornFXSubRendererMaterial::CanBeMergedWith(const FPopcornFXSubRendererMaterial &other) const
{
	return
		m_RMId == other.m_RMId &&
		DefaultMaterialType == other.DefaultMaterialType &&
		LegacyMaterialType == other.LegacyMaterialType &&
		TextureDiffuse == other.TextureDiffuse &&
		TextureDiffuseRamp == other.TextureDiffuseRamp &&
		TextureEmissive == other.TextureEmissive &&
		TextureNormal == other.TextureNormal &&
		TextureRoughMetal == other.TextureRoughMetal &&
		TextureSpecular == other.TextureSpecular &&
		TextureAlphaRemapper == other.TextureAlphaRemapper &&
		TextureAtlas == other.TextureAtlas &&
		NoAlpha == other.NoAlpha &&
		SoftAnimBlending == other.SoftAnimBlending &&
		MotionVectorsBlending == other.MotionVectorsBlending &&
		TextureMotionVectors == other.TextureMotionVectors &&
		TextureSixWay_RLTS == other.TextureSixWay_RLTS &&
		TextureSixWay_BBF == other.TextureSixWay_BBF &&
		VATTexturePosition == other.VATTexturePosition &&
		VATTextureNormal == other.VATTextureNormal &&
		VATTextureColor == other.VATTextureColor &&
		VATTextureRotation == other.VATTextureRotation &&
		TextureSkeletalAnimation == other.TextureSkeletalAnimation &&
		VATNumFrames == other.VATNumFrames &&
		VATPackedData == other.VATPackedData &&
		VATPivotBoundsMin == other.VATPivotBoundsMin &&
		VATPivotBoundsMax == other.VATPivotBoundsMax &&
		VATNormalizedData == other.VATNormalizedData &&
		VATPositionBoundsMin == other.VATPositionBoundsMin &&
		VATPositionBoundsMax == other.VATPositionBoundsMax &&
		SkeletalAnimationCount == other.SkeletalAnimationCount &&
		SkeletalAnimationPosBoundsMin == other.SkeletalAnimationPosBoundsMin &&
		SkeletalAnimationPosBoundsMax == other.SkeletalAnimationPosBoundsMax &&
		SkeletalAnimationLinearInterpolate == other.SkeletalAnimationLinearInterpolate &&
		SkeletalAnimationLinearInterpolateTracks == other.SkeletalAnimationLinearInterpolateTracks &&
		CastShadow == other.CastShadow &&
		Roughness == other.Roughness &&
		Metalness == other.Metalness &&
		SoftnessDistance == other.SoftnessDistance &&
		MaskThreshold == other.MaskThreshold &&
		DrawOrder == other.DrawOrder &&
		DynamicParameterMask == other.DynamicParameterMask &&
		SkeletalMesh == other.SkeletalMesh &&
		StaticMesh == other.StaticMesh &&
		StaticMeshLOD == other.StaticMeshLOD &&
		PerParticleLOD == other.PerParticleLOD &&
		MotionBlur == other.MotionBlur;
}

//----------------------------------------------------------------------------
//
// UPopcornFXRendererMaterial
//
//----------------------------------------------------------------------------

UPopcornFXRendererMaterial::UPopcornFXRendererMaterial(const FObjectInitializer& PCIP)
	: Super(PCIP)
{
}

void	UPopcornFXRendererMaterial::BeginDestroy()
{
	Super::BeginDestroy();
	m_DetachFence.BeginFence();
}

bool	UPopcornFXRendererMaterial::IsReadyForFinishDestroy()
{
	return Super::IsReadyForFinishDestroy() && m_DetachFence.IsFenceComplete();
}

void	UPopcornFXRendererMaterial::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FPopcornFXCustomVersion::GUID);
}

void	UPopcornFXRendererMaterial::PostLoad()
{
	Super::PostLoad();

	const int32		version = GetLinkerCustomVersion(FPopcornFXCustomVersion::GUID);
	if (version < FPopcornFXCustomVersion::LatestVersion)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("UPopcornFXRendererMaterial '%s' is outdated (%d), please re-save or re-cook asset"), *GetPathName(), version);
	}

}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5)
void	UPopcornFXRendererMaterial::PreSave(FObjectPreSaveContext SaveContext)
#else
void	UPopcornFXRendererMaterial::PreSave(const class ITargetPlatform* TargetPlatform)
#endif // (ENGINE_MAJOR_VERSION == 5)
{
	// Flush rendering commands to ensure the rendering thread do not touch us
	FlushRenderingCommands();

#if (ENGINE_MAJOR_VERSION == 5)
	Super::PreSave(SaveContext);
#else
	Super::PreSave(TargetPlatform);
#endif // (ENGINE_MAJOR_VERSION == 5)
}

//----------------------------------------------------------------------------
#if WITH_EDITOR
//----------------------------------------------------------------------------

void	UPopcornFXRendererMaterial::PreEditChange(FProperty *propertyThatWillChange)
{
	Super::PreEditChange(propertyThatWillChange);

	// Flush rendering commands to ensure the rendering thread do not touch us
	// UActorComponent already does that ? we are not UActorComponent !?
	FlushRenderingCommands();
}

//----------------------------------------------------------------------------

void	UPopcornFXRendererMaterial::PostEditChangeChainProperty(struct FPropertyChangedChainEvent & propertyChangedEvent)
{
	static FName		SubMaterialsName(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXRendererMaterial, SubMaterials));
	static FName		MaterialInstanceName(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSubRendererMaterial, MaterialInstance));

	if (propertyChangedEvent.Property != null &&
		propertyChangedEvent.Property->GetFName() != MaterialInstanceName)
	{
		int32		matIndex = propertyChangedEvent.GetArrayIndex(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXRendererMaterial, SubMaterials));
		if (matIndex >= 0)
		{
			TriggerParticleRenderersModification();
		}
		else if (propertyChangedEvent.PropertyChain.GetActiveMemberNode() != null)
		{
			FProperty		*member =  propertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue();
			if (member != null && member->GetFName() == SubMaterialsName)
			{
				TriggerParticleRenderersModification();
			}
		}
	}
	Super::PostEditChangeChainProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXRendererMaterial::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXRendererMaterial::PostEditUndo()
{
	TriggerParticleRenderersModification();
	Super::PostEditUndo();
}

//----------------------------------------------------------------------------

void	UPopcornFXRendererMaterial::_AddRendererIndex(uint32 rIndex)
{
	PK_ASSERT(!RendererIndices.Contains(rIndex));
	RendererIndices.Add(rIndex);
}

//----------------------------------------------------------------------------

void	UPopcornFXRendererMaterial::TriggerParticleRenderersModification()
{
	UPopcornFXEffect		*effect = ParentEffect.Get();
	if (!PK_VERIFY(effect != null))
		return;

	ReloadInstances();

	effect->ReloadFile();
}

//----------------------------------------------------------------------------

bool	UPopcornFXRendererMaterial::_Setup(UPopcornFXEffect *parentEffect, const void *renderer /* PopcornFX::CRendererDataBase */, uint32 rIndex)
{
	ParentEffect = parentEffect;

	RendererIndices.Empty(RendererIndices.Num());
	_AddRendererIndex(rIndex);

	int32		rmId = -1;
	for (u32 rmi = 0; rmi < PK_ARRAY_COUNT(kRendererMaterialFuncs); ++rmi)
	{
		if (kRendererMaterialFuncs[rmi].m_SetupFunc(this, renderer))
		{
			rmId = rmi;
			for (int32 mati = 0; mati < SubMaterials.Num(); ++mati)
				SubMaterials[mati].m_RMId = rmId;
			break;
		}
	}

	if (rmId < 0)
	{
		SubMaterials.Empty();
		return false;
	}

	for (int32 mati = 0; mati < SubMaterials.Num(); ++mati)
		if (SubMaterials[mati].Material == null)
			SubMaterials[mati]._ResetDefaultMaterial_NoReload();
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXRendererMaterial::Setup(UPopcornFXEffect *parentEffect, const void *renderer /* PopcornFX::CRendererDataBase */, u32 rIndex)
{
	return _Setup(parentEffect, renderer, rIndex);
}

//----------------------------------------------------------------------------

bool	UPopcornFXRendererMaterial::Add(UPopcornFXEffect *parentEffect, u32 rIndex)
{
	check(ParentEffect.Get() == parentEffect);
	_AddRendererIndex(rIndex);
	return true;
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
//----------------------------------------------------------------------------

bool	UPopcornFXRendererMaterial::Contains(uint32 rIndex)
{
	for (int32 rendereri = 0; rendereri < RendererIndices.Num(); ++rendereri)
	{
		if (RendererIndices[rendereri] == rIndex)
			return true;
	}
	return false;
}

//----------------------------------------------------------------------------

bool	UPopcornFXRendererMaterial::CanBeMergedWith(const UPopcornFXRendererMaterial *other) const
{
	if (this == other)
		return true;
	if (SubMaterials.Num() != other->SubMaterials.Num())
		return false;
	for (int32 mati = 0; mati < SubMaterials.Num(); ++mati)
	{
		if (!SubMaterials[mati].CanBeMergedWith(other->SubMaterials[mati]))
			return false;
	}
	return true;
}

//----------------------------------------------------------------------------
#if WITH_EDITOR

bool	UPopcornFXRendererMaterial::_ReloadInstance(uint32 subMatId)
{
	if (!PK_VERIFY(subMatId < uint32(SubMaterials.Num())))
		return false;

	FPopcornFXSubRendererMaterial	&subMat = SubMaterials[subMatId];

	if (subMat.Material == null ||
		!PK_VERIFY(uint32(subMat.m_RMId) < PK_ARRAY_COUNT(kRendererMaterialFuncs)))
	{
		subMat.MaterialInstance = null;
		return false;
	}

	FMaterialUpdateContext			materialUpdateContext;

	UMaterialInstanceConstant		*newInstance = NewObject<UMaterialInstanceConstant>(this);
	newInstance->SetFlags(RF_Public);
	newInstance->SetParentEditorOnly(subMat.Material);

	kRendererMaterialFuncs[subMat.m_RMId].m_ParamsConstFunc(this, newInstance, subMatId);

	subMat.MaterialInstance = newInstance;

	materialUpdateContext.AddMaterialInstance(newInstance);
	return true;
}

//----------------------------------------------------------------------------

void	UPopcornFXRendererMaterial::ReloadInstances()
{
	for (int32 mati = 0; mati < SubMaterials.Num(); ++mati)
			_ReloadInstance(mati);
	PostEditChange();
}

//----------------------------------------------------------------------------

void	UPopcornFXRendererMaterial::RenameDependenciesIFN(UObject* oldAsset, UObject* newAsset)
{
	for (int32 mati = 0; mati < SubMaterials.Num(); ++mati)
		SubMaterials[mati].RenameDependenciesIFN(oldAsset, newAsset);
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

//UMaterialInstanceDynamic	*UPopcornFXRendererMaterial::GetInstance(uint32 subMatId, bool forRenderThread)
UMaterialInstanceConstant	*UPopcornFXRendererMaterial::GetInstance(uint32 subMatId, bool forRenderThread) const
{
	if (!PK_VERIFY(subMatId < uint32(SubMaterials.Num())))
		return null;
	const FPopcornFXSubRendererMaterial	&subMat = SubMaterials[subMatId];
	return subMat.MaterialInstance;
}

//----------------------------------------------------------------------------
//
// Billboards / Ribbons
//
//----------------------------------------------------------------------------

const EPopcornFXLegacyMaterialType		kLegacyTransparentBillboard_Material_ToUE[] =
{
	EPopcornFXLegacyMaterialType::Billboard_Additive,				//Additive
	EPopcornFXLegacyMaterialType::Billboard_Additive_NoAlpha,		//AdditiveNoAlpha
	EPopcornFXLegacyMaterialType::Billboard_AlphaBlend,			//AlphaBlend
	EPopcornFXLegacyMaterialType::Billboard_AlphaBlendAdditive,	//AlphaBlend_Additive
};
PK_STATIC_ASSERT(PK_ARRAY_COUNT(kLegacyTransparentBillboard_Material_ToUE) == PopcornFX::BasicRendererProperties::ETransparentType::__MaxTransparentType);

//----------------------------------------------------------------------------

const EPopcornFXLegacyMaterialType		kOpaqueBillboard_LegacyMaterial_ToUE[] =
{
	EPopcornFXLegacyMaterialType::Billboard_Solid,				// Solid
	EPopcornFXLegacyMaterialType::Billboard_Masked,				// Masked
};
PK_STATIC_ASSERT(PK_ARRAY_COUNT(kOpaqueBillboard_LegacyMaterial_ToUE) == PopcornFX::BasicRendererProperties::EOpaqueType::__MaxOpaqueType);

//----------------------------------------------------------------------------

const EPopcornFXDefaultMaterialType		kOpaqueBillboard_DefaultMaterial_ToUE[] =
{
	EPopcornFXDefaultMaterialType::Billboard_Solid,				// Solid
	EPopcornFXDefaultMaterialType::Billboard_Masked,			// Masked
};
PK_STATIC_ASSERT(PK_ARRAY_COUNT(kOpaqueBillboard_DefaultMaterial_ToUE) == PopcornFX::BasicRendererProperties::EOpaqueType::__MaxOpaqueType);

//----------------------------------------------------------------------------
//
// Mesh
//
//----------------------------------------------------------------------------

const EPopcornFXLegacyMaterialType		kOpaqueMesh_LegacyMaterial_ToUE[] =
{
	EPopcornFXLegacyMaterialType::Mesh_Solid,				// Solid,
	EPopcornFXLegacyMaterialType::Mesh_Masked,			// Masked,
};
PK_STATIC_ASSERT(PK_ARRAY_COUNT(kOpaqueMesh_LegacyMaterial_ToUE) == PopcornFX::BasicRendererProperties::EOpaqueType::__MaxOpaqueType);

const EPopcornFXDefaultMaterialType		kOpaqueMesh_DefaultMaterial_ToUE[] =
{
	EPopcornFXDefaultMaterialType::Mesh_Solid,				// Solid,
	EPopcornFXDefaultMaterialType::Mesh_Masked,			// Masked,
};
PK_STATIC_ASSERT(PK_ARRAY_COUNT(kOpaqueMesh_DefaultMaterial_ToUE) == PopcornFX::BasicRendererProperties::EOpaqueType::__MaxOpaqueType);

//----------------------------------------------------------------------------
//
// (Helper)
//
//----------------------------------------------------------------------------

UTexture2D		*LoadTexturePk(const PopcornFX::CString &pkPath)
{
	if (pkPath.Empty())
		return null;

	UObject			*obj = FPopcornFXPlugin::Get().LoadUObjectFromPkPath(pkPath, false);
	UTexture2D		*texture = Cast<UTexture2D>(obj);
	if (obj != null && texture == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Asset is not a texture: '%s'"), *ToUE(pkPath));
		return null;
	}
	if (texture == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Could not load texture '%s'"), *ToUE(pkPath));
		return null;
	}
	return texture;
}

//----------------------------------------------------------------------------

UPopcornFXTextureAtlas		*LoadAtlasPk(const PopcornFX::CString &pkPath)
{
	if (pkPath.Empty())
		return null;

	UObject					*obj = FPopcornFXPlugin::Get().LoadUObjectFromPkPath(pkPath, false);
	UPopcornFXTextureAtlas	*atlas = Cast<UPopcornFXTextureAtlas>(obj);
	if (obj != null && atlas == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Asset is not an atlas: '%s'"), *ToUE(pkPath));
		return null;
	}
	if (atlas == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Could not load atlas '%s'"), *ToUE(pkPath));
		return null;
	}
	return atlas;
}

//----------------------------------------------------------------------------

UStaticMesh		*LoadMeshPk(const PopcornFX::CString &pkPath)
{
	if (pkPath.Empty())
		return null;

	UObject			*obj = FPopcornFXPlugin::Get().LoadUObjectFromPkPath(pkPath, false);
	UStaticMesh		*mesh = Cast<UStaticMesh>(obj);
	if (obj != null && mesh == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Asset is not a mesh: '%s'"), *ToUE(pkPath));
		return null;
	}
	if (mesh == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Could not load mesh '%s'"), *ToUE(pkPath));
		return null;
	}
	return mesh;
}

//----------------------------------------------------------------------------

USkeletalMesh		*LoadSkelMeshPk(const PopcornFX::CString &pkPath)
{
	if (pkPath.Empty())
		return null;

	UObject			*obj = FPopcornFXPlugin::Get().LoadUObjectFromPkPath(pkPath, false);
	USkeletalMesh	*mesh = Cast<USkeletalMesh>(obj);
	if (obj != null && mesh == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Asset is not a skeletal mesh: '%s'"), *ToUE(pkPath));
		return null;
	}
	if (mesh == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Could not load skeletal mesh '%s'"), *ToUE(pkPath));
		return null;
	}
	return mesh;
}

//----------------------------------------------------------------------------

void	SetMaterialTextureParameter(UMaterialInstanceDynamic *mat, FName textureName, const PopcornFX::CString &pkTexturePath)
{
	mat->SetTextureParameterValue(textureName, LoadTexturePk(pkTexturePath));
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
