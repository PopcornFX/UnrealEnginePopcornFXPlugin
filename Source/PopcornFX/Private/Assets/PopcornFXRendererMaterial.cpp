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

#include "PopcornFXSDK.h"
#include <pk_base_object/include/hbo_handler.h>
#include <pk_particles/include/ps_effect.h>
#include <pk_particles/include/ps_event_map.h>
#include <pk_particles/include/ps_nodegraph_nodes_render.h>
#include <pk_particles/include/Renderers/ps_renderer_base.h>

#include "Render/PopcornFXRendererProperties.h"
#include <pk_render_helpers/include/basic_renderer_properties/rh_basic_renderer_properties.h>
#include <pk_render_helpers/include/basic_renderer_properties/rh_vertex_animation_renderer_properties.h>

#if WITH_EDITOR
#	include "Misc/MessageDialog.h"
#	include "Factories/FbxStaticMeshImportData.h"
#	include "Factories/ReimportFbxStaticMeshFactory.h"
#endif

#if (ENGINE_MAJOR_VERSION == 5)
#include "UObject/ObjectSaveContext.h"
#endif // (ENGINE_MAJOR_VERSION == 5)

#define LOCTEXT_NAMESPACE "PopcornFXRendererMaterial"

// Helpers

UTexture2D					*LoadTexturePk(const PopcornFX::CString &pkPath);
UPopcornFXTextureAtlas		*LoadAtlasPk(const PopcornFX::CString &pkPath);
UStaticMesh					*LoadMeshPk(const PopcornFX::CString &pkPath);
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
	// ..
	bool	_GetPropertyValueAsBool(const PopcornFX::SRendererDeclaration &decl, PopcornFX::CStringId propertyName)
	{
		const PopcornFX::SRendererFeaturePropertyValue	*prop = decl.FindProperty(propertyName);
		if (prop == null)
			return false;
		return prop->ValueB();
	}

	const PopcornFX::CString	_GetPropertyValueAsPath(const PopcornFX::SRendererDeclaration &decl, PopcornFX::CStringId propertyName)
	{
		const PopcornFX::SRendererFeaturePropertyValue	*prop = decl.FindProperty(propertyName);
		if (prop == null)
			return "";
		return prop->ValuePath();
	}

	float	_GetPropertyValueAsFloat(const PopcornFX::SRendererDeclaration &decl, PopcornFX::CStringId propertyName)
	{
		const PopcornFX::SRendererFeaturePropertyValue	*prop = decl.FindProperty(propertyName);
		if (prop == null)
			return 0.0f;
		return prop->ValueF().x();
	}

	CFloat2	_GetPropertyValueAsFloat2(const PopcornFX::SRendererDeclaration &decl, PopcornFX::CStringId propertyName)
	{
		const PopcornFX::SRendererFeaturePropertyValue	*prop = decl.FindProperty(propertyName);
		if (prop == null)
			return CFloat2::ZERO;
		return prop->ValueF().xy();
	}

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

	s32	_GetPropertyValueAsInt(const PopcornFX::SRendererDeclaration &decl, const PopcornFX::CStringId propertyName)
	{
		const PopcornFX::SRendererFeaturePropertyValue	*prop = decl.FindProperty(propertyName);
		if (prop == null)
			return 0;
		return prop->ValueI().x();
	}

	void	_SetStaticSwitch(FStaticParameterSet &params, FName name, bool value)
	{
		for (int32 i = 0; i < params.StaticSwitchParameters.Num(); ++i)
		{
			FStaticSwitchParameter		&param  = params.StaticSwitchParameters[i];

			if (param.ParameterInfo.Name == name)
			{
				param.Value = value;
				param.bOverride = true;
				// should be uniq !?
				return;
			}
		}
		new (params.StaticSwitchParameters) FStaticSwitchParameter(name, value, true, FGuid()); // FGuid ???
	}

	void	_SetStaticSwitches_Common(UMaterialInstanceConstant *material, FPopcornFXSubRendererMaterial &mat)
	{
		FStaticParameterSet			params = material->GetStaticParameters();

		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_SOFT_PARTICLES(), mat.SoftnessDistance > 0.f);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_SOFT_ANIM(), mat.SoftAnimBlending != 0);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_MOTION_VECTORS_BLENDING(), mat.MotionVectorsBlending != 0);
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_IS_LIT(), mat.Lit!= 0); // Currently used by VAT materials only
		_SetStaticSwitch(params, FPopcornFXPlugin::Name_POPCORNFX_HAS_NORMAL_TEXTURE(), mat.TextureNormal != null);
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
	bool		RM_Setup_Billboard(UPopcornFXRendererMaterial *self, const void *rendererBase)
	{
		if (static_cast<const PopcornFX::CRendererDataBase*>(rendererBase)->m_RendererType != PopcornFX::ERendererClass::Renderer_Billboard)
			return false;
		const PopcornFX::CRendererDataBillboard	*renderer = static_cast<const PopcornFX::CRendererDataBillboard*>(rendererBase);
		if (renderer == null)
			return false;
		self->SubMaterials.SetNum(1);
		FPopcornFXSubRendererMaterial	&mat = self->SubMaterials[0];

		const PopcornFX::SRendererFeaturePropertyValue	*prop = null;
		const PopcornFX::SRendererDeclaration			&decl = renderer->m_Declaration;

		const bool	legacyLit = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit());

		// Default values:
		mat.MaterialType = EPopcornFXMaterialType::Billboard_Additive;
		mat.Lit = legacyLit || _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Lit());
		mat.Raytraced = _GetPropertyValueAsBool(decl, PopcornFX::CStringId("Raytraced")); // tmp

		if (mat.Lit)
		{
			if (legacyLit)
			{
				mat.CastShadow = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit_CastShadows());
				mat.TextureNormal = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit_NormalMap()));
			}
			else
			{
				mat.CastShadow = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Lit_CastShadows());
				mat.TextureNormal = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Lit_NormalMap()));
			}
		}

		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Transparent()))
		{
			mat.MaterialType = kTransparentBillboard_Material_ToUE[_GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Transparent_Type())];
			mat.DrawOrder = _GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Transparent_GlobalSortOverride());
			if (_GetPropertyValueAsBool(decl, UERendererProperties::SID_SixWayLightmap()))
			{
				mat.CastShadow = true; // Tmp: Force cast shadows true when six way lightmap material is used
				mat.MaterialType = EPopcornFXMaterialType::Billboard_SixWayLightmap;
				mat.TextureSixWay_RLTS = LoadTexturePk(_GetPropertyValueAsPath(decl, UERendererProperties::SID_SixWayLightmap_RightLeftTopSmoke()));
				mat.TextureSixWay_BBF = LoadTexturePk(_GetPropertyValueAsPath(decl, UERendererProperties::SID_SixWayLightmap_BottomBackFront()));
			}
		}
		else if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Opaque()))
		{
			mat.MaterialType = kOpaqueBillboard_Material_ToUE[_GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Opaque_Type())];
			if (mat.MaterialType == EPopcornFXMaterialType::Billboard_Masked)
				mat.MaskThreshold = _GetPropertyValueAsFloat(decl, PopcornFX::BasicRendererProperties::SID_Opaque_MaskThreshold());
		}
		else if (_GetPropertyValueAsBool(decl, UERendererProperties::SID_VolumetricFog()))
		{
			mat.MaterialType = EPopcornFXMaterialType::Billboard_VolumetricFog;
			mat.TextureDiffuse = LoadTexturePk(_GetPropertyValueAsPath(decl, UERendererProperties::SID_VolumetricFog_AlbedoMap()));
			mat.SphereMaskHardness = _GetPropertyValueAsFloat(decl, UERendererProperties::SID_VolumetricFog_SphereMaskHardness());
		}

		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Emissive()))
			mat.TextureEmissive = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveMap()));
		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Diffuse()))
		{
			mat.TextureDiffuse = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseMap()));
			if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_DiffuseRamp()))
				mat.TextureDiffuseRamp = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_DiffuseRamp_RampMap()));
		}
		else if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Distortion()))
		{
			mat.TextureDiffuse = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Distortion_DistortionMap()));
			mat.MaterialType = EPopcornFXMaterialType::Billboard_Distortion;

			if (mat.TextureDiffuse != null &&
				mat.TextureDiffuse->CompressionSettings != TextureCompressionSettings::TC_Normalmap)
			{
				FText		title = LOCTEXT("import_distortion_texture_title", "PopcornFX: Compression Settings");
				const FText	message = FText::Format(LOCTEXT("import_distortion_texture_message",
					"Texture \"{0}\" is used for distortion but has incompatible compression settings. Do you want to import it using NormalMap compression settings instead?"),
					FText::FromString(mat.TextureDiffuse->GetPathName()));

				if (FMessageDialog::Open(EAppMsgType::YesNo, message, &title) == EAppReturnType::Yes)
				{
					mat.TextureDiffuse->Modify();
					mat.TextureDiffuse->CompressionSettings = TextureCompressionSettings::TC_Normalmap;
				}
			}
		}

		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Atlas()))
		{
			mat.TextureAtlas = LoadAtlasPk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Atlas_Definition()));

			const s32	blendingType = _GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Atlas_Blending()); // 1 = LinearAtlasBlending. 2 = MotionVectors blending (optical flow);
			mat.SoftAnimBlending = blendingType == 1;
			mat.MotionVectorsBlending = blendingType == 2;
			if (mat.MotionVectorsBlending)
			{
				mat.TextureMotionVectors = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Atlas_MotionVectorsMap()));

				const CFloat2	distortionStrength = _GetPropertyValueAsFloat2(decl, PopcornFX::BasicRendererProperties::SID_Atlas_DistortionStrength());
				mat.MVDistortionStrengthColumns = distortionStrength.x();
				mat.MVDistortionStrengthRows = distortionStrength.y();
			}
		}
		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_AlphaRemap()))
			mat.TextureAlphaRemapper = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_AlphaRemap_AlphaMap()));
		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_SoftParticles()))
			mat.SoftnessDistance = _GetPropertyValueAsFloat(decl, PopcornFX::BasicRendererProperties::SID_SoftParticles_SoftnessDistance());

		EPopcornFXMaterialType::Type	finalMatType = mat.MaterialType;
		switch (mat.MaterialType)
		{
		case	EPopcornFXMaterialType::Billboard_Additive_NoAlpha:
			finalMatType = EPopcornFXMaterialType::Billboard_Additive;
			mat.NoAlpha = true;
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		case	EPopcornFXMaterialType::Billboard_Additive:
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		case	EPopcornFXMaterialType::Billboard_AlphaBlend:
			if (mat.Lit)
				finalMatType = EPopcornFXMaterialType::Billboard_AlphaBlend_Lit;
			break;
		case	EPopcornFXMaterialType::Billboard_AlphaBlendAdditive:
			if (mat.Lit)
				finalMatType = EPopcornFXMaterialType::Billboard_AlphaBlendAdditive_Lit;
			break;
		case	EPopcornFXMaterialType::Billboard_Distortion:
			PK_ASSERT(!mat.Lit);
			break;
		case	EPopcornFXMaterialType::Billboard_Solid:
			if (mat.Lit)
				finalMatType = EPopcornFXMaterialType::Billboard_Solid_Lit;
			break;
		case	EPopcornFXMaterialType::Billboard_Masked:
			if (mat.Lit)
				finalMatType = EPopcornFXMaterialType::Billboard_Masked_Lit;
			break;
		case	EPopcornFXMaterialType::Billboard_VolumetricFog:
		case	EPopcornFXMaterialType::Billboard_SixWayLightmap:
			finalMatType = mat.MaterialType;
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			return false;
		}
		mat.MaterialType = finalMatType;

		// Not correct, this should be derived from the material Blend Mode as it can be overriden
		mat.SortIndices =	(mat.MaterialType >= EPopcornFXMaterialType::Billboard_AlphaBlend && mat.MaterialType <= EPopcornFXMaterialType::Billboard_AlphaBlendAdditive_Lit) ||
							mat.MaterialType == EPopcornFXMaterialType::Billboard_SixWayLightmap;

		return true;
	}

	bool		RM_Params_Billboard(UPopcornFXRendererMaterial *self, UMaterialInstanceDynamic *material, uint32 subMaterialIndex)
	{
		check(subMaterialIndex < uint32(self->SubMaterials.Num()));
		FPopcornFXSubRendererMaterial		&mat = self->SubMaterials[subMaterialIndex];
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_DiffuseTexture(), mat.TextureDiffuse);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_DiffuseTextureRamp(), mat.TextureDiffuseRamp);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_EmissiveTexture(), mat.TextureEmissive);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_NormalTexture(), mat.TextureNormal);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_SpecularTexture(), mat.TextureSpecular);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_AlphaRemapper(), mat.TextureAlphaRemapper);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_MotionVectorsTexture(), mat.TextureMotionVectors);

		material->SetTextureParameterValue(FPopcornFXPlugin::Name_SixWayLightmap_RLTSTexture(), mat.TextureSixWay_RLTS);
		material->SetTextureParameterValue(FPopcornFXPlugin::Name_SixWayLightmap_BBFTexture(), mat.TextureSixWay_BBF);

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
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_SpecularTexture(), mat.TextureSpecular);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_AlphaRemapper(), mat.TextureAlphaRemapper);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_MotionVectorsTexture(), mat.TextureMotionVectors);

		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_SixWayLightmap_RLTSTexture(), mat.TextureSixWay_RLTS);
		material->SetTextureParameterValueEditorOnly(FPopcornFXPlugin::Name_SixWayLightmap_BBFTexture(), mat.TextureSixWay_BBF);

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
	bool		RM_Setup_Ribbon(UPopcornFXRendererMaterial *self, const void *rendererBase)
	{
		if (static_cast<const PopcornFX::CRendererDataBase*>(rendererBase)->m_RendererType != PopcornFX::ERendererClass::Renderer_Ribbon)
			return false;
		const PopcornFX::CRendererDataRibbon	*renderer = static_cast<const PopcornFX::CRendererDataRibbon*>(rendererBase);
		if (renderer == null)
			return false;
		self->SubMaterials.SetNum(1);
		FPopcornFXSubRendererMaterial	&mat = self->SubMaterials[0];

		const PopcornFX::SRendererDeclaration		&decl = renderer->m_Declaration;

		const bool	legacyLit = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit());

		// Default values:
		mat.MaterialType = EPopcornFXMaterialType::Billboard_Additive;
		mat.TextureNormal = null;
		mat.TextureDiffuse = null;
		mat.TextureDiffuseRamp = null;
		mat.TextureEmissive = null;
		mat.SoftnessDistance = 0.0f;
		mat.MaskThreshold = 0.0f;
		mat.TextureAlphaRemapper = null;
		mat.TextureAtlas = null;
		mat.Lit = legacyLit || _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Lit());
		mat.SoftAnimBlending = false;
		mat.MotionVectorsBlending = false;
		mat.TextureMotionVectors = null;
		mat.MVDistortionStrengthColumns = 1.0f;
		mat.MVDistortionStrengthRows = 1.0f;
		mat.CastShadow = false;
		mat.CorrectDeformation = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_CorrectDeformation());

		if (mat.Lit)
		{
			if (legacyLit)
			{
				mat.CastShadow = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit_CastShadows());
				mat.TextureNormal = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit_NormalMap()));
			}
			else
			{
				mat.CastShadow = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Lit_CastShadows());
				mat.TextureNormal = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Lit_NormalMap()));
			}
		}

		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Transparent()))
		{
			mat.MaterialType = kTransparentBillboard_Material_ToUE[_GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Transparent_Type())];
			mat.DrawOrder = _GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Transparent_GlobalSortOverride());
		}
		else if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Opaque()))
		{
			mat.MaterialType = kOpaqueBillboard_Material_ToUE[_GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Opaque_Type())];
			if (mat.MaterialType == EPopcornFXMaterialType::Billboard_Masked)
				mat.MaskThreshold = _GetPropertyValueAsFloat(decl, PopcornFX::BasicRendererProperties::SID_Opaque_MaskThreshold());
		}

		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Emissive()))
			mat.TextureEmissive = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveMap()));
		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Diffuse()))
		{
			mat.TextureDiffuse = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseMap()));
			if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_DiffuseRamp()))
				mat.TextureDiffuseRamp = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_DiffuseRamp_RampMap()));
		}
		else if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Distortion()))
		{
			mat.TextureDiffuse = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Distortion_DistortionMap()));
			mat.MaterialType = EPopcornFXMaterialType::Billboard_Distortion;

			if (mat.TextureDiffuse != null &&
				mat.TextureDiffuse->CompressionSettings != TextureCompressionSettings::TC_Normalmap)
			{
				FText		title = LOCTEXT("import_distortion_texture_title", "PopcornFX: Compression Settings");
				const FText	message = FText::Format(LOCTEXT("import_distortion_texture_message",
					"Texture \"{0}\" is used for distortion but has incompatible compression settings. Do you want to import it using NormalMap compression settings instead?"),
					FText::FromString(mat.TextureDiffuse->GetPathName()));

				if (FMessageDialog::Open(EAppMsgType::YesNo, message, &title) == EAppReturnType::Yes)
				{
					mat.TextureDiffuse->Modify();
					mat.TextureDiffuse->CompressionSettings = TextureCompressionSettings::TC_Normalmap;
				}
			}
		}

		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Atlas()))
		{
			mat.TextureAtlas = LoadAtlasPk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Atlas_Definition()));

			const s32	blendingType = _GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Atlas_Blending()); // 1 = LinearAtlasBlending. 2 = MotionVectors blending (optical flow);
			mat.SoftAnimBlending = blendingType == 1;
			mat.MotionVectorsBlending = blendingType == 2;
			if (mat.MotionVectorsBlending)
			{
				mat.TextureMotionVectors = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Atlas_MotionVectorsMap()));

				const CFloat2	distortionStrength = _GetPropertyValueAsFloat2(decl, PopcornFX::BasicRendererProperties::SID_Atlas_DistortionStrength());
				mat.MVDistortionStrengthColumns = distortionStrength.x();
				mat.MVDistortionStrengthRows = distortionStrength.y();
			}
		}
		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_AlphaRemap()))
			mat.TextureAlphaRemapper = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_AlphaRemap_AlphaMap()));
		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_SoftParticles()))
			mat.SoftnessDistance = _GetPropertyValueAsFloat(decl, PopcornFX::BasicRendererProperties::SID_SoftParticles_SoftnessDistance());

		//if (_GetPropertyValueAsBool(decl, "Opaque"))
		//{
		//	const PopcornFX::SRendererFeaturePropertyValue	*type = decl.FindProperty("Opaque.Type");
		//}

		EPopcornFXMaterialType::Type	finalMatType = mat.MaterialType;
		switch (mat.MaterialType)
		{
		case	EPopcornFXMaterialType::Billboard_Additive_NoAlpha:
		{
			finalMatType = EPopcornFXMaterialType::Billboard_Additive;
			mat.NoAlpha = true;
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		}
		case	EPopcornFXMaterialType::Billboard_Additive:
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		case	EPopcornFXMaterialType::Billboard_AlphaBlend:
			break;
		case	EPopcornFXMaterialType::Billboard_AlphaBlendAdditive:
			if (mat.Lit)
				finalMatType = EPopcornFXMaterialType::Billboard_AlphaBlendAdditive_Lit;
			break;
		case	EPopcornFXMaterialType::Billboard_Distortion:
			PK_ASSERT(!mat.Lit);
			break;
		case	EPopcornFXMaterialType::Billboard_Solid:
			if (mat.Lit)
				finalMatType = EPopcornFXMaterialType::Billboard_Solid_Lit;
			break;
		case	EPopcornFXMaterialType::Billboard_Masked:
			if (mat.Lit)
				finalMatType = EPopcornFXMaterialType::Billboard_Masked_Lit;
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			return false;
		}
		mat.MaterialType = finalMatType;
		mat.SortIndices = mat.MaterialType >= EPopcornFXMaterialType::Billboard_AlphaBlend && mat.MaterialType <= EPopcornFXMaterialType::Billboard_AlphaBlendAdditive_Lit;

		return true;
	}

	bool		RM_Setup_Mesh(UPopcornFXRendererMaterial *self, const void *rendererBase)
	{
		if (static_cast<const PopcornFX::CRendererDataBase*>(rendererBase)->m_RendererType != PopcornFX::ERendererClass::Renderer_Mesh)
			return false;
		const PopcornFX::CRendererDataMesh	*renderer = static_cast<const PopcornFX::CRendererDataMesh*>(rendererBase);
		if (renderer == null)
			return false;

		// TODO: Remove sub materials
		self->SubMaterials.SetNum(1);
		FPopcornFXSubRendererMaterial	&mat = self->SubMaterials[0];

		const PopcornFX::SRendererDeclaration		&decl = renderer->m_Declaration;

		const bool	legacyLit = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit());

		// Default values:
		mat.MaterialType = EPopcornFXMaterialType::Mesh_Solid;
		mat.StaticMesh = LoadMeshPk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Mesh()));
		mat.StaticMeshLOD = 0; // TODO
		mat.Lit = legacyLit || _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Lit());
		mat.MeshAtlas = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_MeshAtlas());

		mat.Raytraced = _GetPropertyValueAsBool(decl, PopcornFX::CStringId("Raytraced")); // tmp

		const bool		fluidVAT = _GetPropertyValueAsBool(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid());
		const bool		softVAT = _GetPropertyValueAsBool(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft());
		const bool		rigidVAT = _GetPropertyValueAsBool(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid());
		const bool		normalizedVAT = _GetPropertyValueAsBool(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_NormalizedData());

		if (mat.Lit)
		{
			if (legacyLit)
			{
				mat.CastShadow = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit_CastShadows());
				mat.TextureSpecular = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit_SpecularMap()));
				mat.TextureNormal = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit_NormalMap()));
			}
			else
			{
				mat.CastShadow = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Lit_CastShadows());
				mat.TextureNormal = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Lit_NormalMap()));
			}
		}

		if (fluidVAT)
		{
			mat.VATTexturePosition = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_PositionMap()));
			mat.VATTextureNormal = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_NormalMap()));
			mat.VATTextureColor = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_ColorMap()));
			mat.VATNumFrames = _GetPropertyValueAsInt(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_NumFrames());
			mat.VATPackedData = _GetPropertyValueAsBool(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Fluid_PackedData());
		}
		else if (softVAT)
		{
			mat.VATTexturePosition = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft_PositionMap()));
			mat.VATTextureNormal = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft_NormalMap()));
			mat.VATNumFrames = _GetPropertyValueAsInt(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft_NumFrames());
			mat.VATPackedData = _GetPropertyValueAsBool(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Soft_PackedData());
		}
		else if (rigidVAT)
		{
			mat.VATTexturePosition = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid_PositionMap()));
			mat.VATTextureRotation = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid_RotationMap()));
			mat.VATNumFrames = _GetPropertyValueAsInt(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid_NumFrames());

			const float		globalScale = FPopcornFXPlugin::GlobalScale();
			const CFloat2	pivotBounds = _GetPropertyValueAsFloat2(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_Rigid_BoundsPivot());
			mat.VATPivotBoundsMin = pivotBounds.x() * globalScale;
			mat.VATPivotBoundsMax = pivotBounds.y() * globalScale;
		}
		if ((fluidVAT || softVAT || rigidVAT) &&
			normalizedVAT)
		{
			mat.VATNormalizedData = true;

			const float		globalScale = FPopcornFXPlugin::GlobalScale();
			const CFloat2	positionBounds = _GetPropertyValueAsFloat2(decl, PopcornFX::VertexAnimationRendererProperties::SID_VertexAnimation_BoundsPosition());
			mat.VATPositionBoundsMin = positionBounds.x() * globalScale;
			mat.VATPositionBoundsMax = positionBounds.y() * globalScale;
		}

		if (fluidVAT || softVAT || rigidVAT)
		{
			const FText	title = LOCTEXT("import_VAT_asset_title", "PopcornFX: Asset used for vertex animation");
			if (mat.VATTexturePosition != null &&
				!_IsVATTextureConfigured(mat.VATTexturePosition))
			{
				const FText	message = FText::Format(LOCTEXT("import_VAT_asset_message", "Texture \"{0}\" is used for vertex animation. Do you want to import it using VAT compatible settings?"), FText::FromString(mat.VATTexturePosition->GetPathName()));
				if (FMessageDialog::Open(EAppMsgType::YesNo, message, &title) == EAppReturnType::Yes)
				{
					mat.VATTexturePosition->Modify();
					mat.VATTexturePosition->Filter = TF_Nearest;
				}
			}
			if (mat.VATTextureNormal != null &&
				!_IsVATTextureConfigured(mat.VATTextureNormal))
			{
				const FText	message = FText::Format(LOCTEXT("import_VAT_asset_message", "Texture \"{0}\" is used for vertex animation. Do you want to import it using VAT compatible settings?"), FText::FromString(mat.VATTextureNormal->GetPathName()));
				if (FMessageDialog::Open(EAppMsgType::YesNo, message, &title) == EAppReturnType::Yes)
				{
					mat.VATTextureNormal->Modify();
					mat.VATTextureNormal->Filter = TF_Nearest;
				}
			}
			if (mat.VATTextureColor != null &&
				!_IsVATTextureConfigured(mat.VATTextureColor))
			{
				const FText	message = FText::Format(LOCTEXT("import_VAT_asset_message", "Texture \"{0}\" is used for vertex animation. Do you want to import it using VAT compatible settings?"), FText::FromString(mat.VATTextureColor->GetPathName()));
				if (FMessageDialog::Open(EAppMsgType::YesNo, message, &title) == EAppReturnType::Yes)
				{
					mat.VATTextureColor->Modify();
					mat.VATTextureColor->Filter = TF_Nearest;
				}
			}
			if (mat.VATTextureRotation != null &&
				!_IsVATTextureConfigured(mat.VATTextureRotation))
			{
				const FText	message = FText::Format(LOCTEXT("import_VAT_asset_message", "Texture \"{0}\" is used for vertex animation. Do you want to import it using VAT compatible settings?"), FText::FromString(mat.VATTextureRotation->GetPathName()));
				if (FMessageDialog::Open(EAppMsgType::YesNo, message, &title) == EAppReturnType::Yes)
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
					if (FMessageDialog::Open(EAppMsgType::YesNo, message, &title) == EAppReturnType::Yes)
					{
						mat.StaticMesh->Modify();
						mat.StaticMesh->GetSourceModel(0).BuildSettings.bUseFullPrecisionUVs = true;
						mat.StaticMesh->GetSourceModel(0).BuildSettings.bRemoveDegenerates = false;
						mat.StaticMesh->Build();
					}
				}
			}
		}

		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Opaque()))
		{
			// * 2 because unlit/lit pair per enum value
			if (fluidVAT)
				mat.MaterialType = static_cast<EPopcornFXMaterialType::Type>(EPopcornFXMaterialType::Mesh_VAT_Opaque_Fluid + _GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Opaque_Type()) * 2);
			else if (softVAT)
				mat.MaterialType = static_cast<EPopcornFXMaterialType::Type>(EPopcornFXMaterialType::Mesh_VAT_Opaque_Soft + _GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Opaque_Type()) * 2);
			else if (rigidVAT)
				mat.MaterialType = static_cast<EPopcornFXMaterialType::Type>(EPopcornFXMaterialType::Mesh_VAT_Opaque_Rigid + _GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Opaque_Type()) * 2);
			else
				mat.MaterialType = kOpaqueMesh_Material_ToUE[_GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Opaque_Type())];

			if (mat.Lit)
				mat.MaterialType = static_cast<EPopcornFXMaterialType::Type>(mat.MaterialType + 1); // Next enum index

			if (mat.MaterialType == EPopcornFXMaterialType::Mesh_Masked)
				mat.MaskThreshold = _GetPropertyValueAsFloat(decl, PopcornFX::BasicRendererProperties::SID_Opaque_MaskThreshold());
		}
		else if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Transparent()))
		{
			if (fluidVAT || softVAT || rigidVAT)
				return false; // unsupported
			else
				mat.MaterialType = EPopcornFXMaterialType::Mesh_Additive;

			mat.DrawOrder = _GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Transparent_GlobalSortOverride());
			mat.Lit = false;
		}
		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Diffuse()))
		{
			mat.TextureDiffuse = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseMap()));
			if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_DiffuseRamp()))
				mat.TextureDiffuseRamp = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_DiffuseRamp_RampMap()));
		}
		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Emissive()))
			mat.TextureEmissive = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveMap()));
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
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_SoftnessDistance(), mat.SoftnessDistance * FPopcornFXPlugin::GlobalScale());
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_MaskThreshold(), mat.MaskThreshold);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_MVDistortionStrengthColumns(), mat.MVDistortionStrengthColumns);
		material->SetScalarParameterValueEditorOnly(FPopcornFXPlugin::Name_MVDistortionStrengthRows(), mat.MVDistortionStrengthRows);

		_SetStaticSwitches_Common(material, mat);

		// !!
		material->PostEditChange();
		return true;
	}

	bool		RM_Setup_Triangle(UPopcornFXRendererMaterial *self, const void *rendererBase)
	{
		if (static_cast<const PopcornFX::CRendererDataBase*>(rendererBase)->m_RendererType != PopcornFX::ERendererClass::Renderer_Triangle)
			return false;

		const PopcornFX::CRendererDataTriangle	*renderer = static_cast<const PopcornFX::CRendererDataTriangle*>(rendererBase);
		if (renderer == null)
			return false;

		self->SubMaterials.SetNum(1);
		FPopcornFXSubRendererMaterial	&mat = self->SubMaterials[0];

		const PopcornFX::SRendererDeclaration		&decl = renderer->m_Declaration;
		const bool	legacyLit = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit());

		mat.MaterialType = EPopcornFXMaterialType::Billboard_Additive;
		mat.TextureNormal = null;
		mat.TextureDiffuse = null;
		mat.TextureDiffuseRamp = null;
		mat.TextureEmissive = null;
		mat.SoftnessDistance = 0.0f;
		mat.MaskThreshold = 0.0f;
		mat.TextureAlphaRemapper = null;
		mat.TextureAtlas = null;
		mat.Lit = legacyLit || _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Lit());
		mat.SoftAnimBlending = false;
		mat.MotionVectorsBlending = false;
		mat.TextureMotionVectors = null;
		mat.MVDistortionStrengthColumns = 1.0f;
		mat.MVDistortionStrengthRows = 1.0f;

		if (legacyLit)
		{
			mat.CastShadow = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit_CastShadows());
			mat.TextureNormal = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_LegacyLit_NormalMap()));
		}
		else if (mat.Lit)
		{
			mat.CastShadow = _GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Lit_CastShadows());
			mat.TextureNormal = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Lit_NormalMap()));
		}

		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Transparent()))
		{
			mat.MaterialType = kTransparentBillboard_Material_ToUE[_GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Transparent_Type())];
			mat.DrawOrder = _GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Transparent_GlobalSortOverride());
		}
		else if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Opaque()))
		{
			mat.MaterialType = kOpaqueBillboard_Material_ToUE[_GetPropertyValueAsInt(decl, PopcornFX::BasicRendererProperties::SID_Opaque_Type())];
			if (mat.MaterialType == EPopcornFXMaterialType::Billboard_Masked)
				mat.MaskThreshold = _GetPropertyValueAsFloat(decl, PopcornFX::BasicRendererProperties::SID_Opaque_MaskThreshold());
		}

		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Emissive()))
			mat.TextureEmissive = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Emissive_EmissiveMap()));
		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Diffuse()))
		{
			mat.TextureDiffuse = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Diffuse_DiffuseMap()));
			if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_DiffuseRamp()))
				mat.TextureDiffuseRamp = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_DiffuseRamp_RampMap()));
		}
		else if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_Distortion()))
		{
			mat.TextureDiffuse = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_Distortion_DistortionMap()));
			mat.MaterialType = EPopcornFXMaterialType::Mesh_Distortion;

			if (mat.TextureDiffuse != null &&
				mat.TextureDiffuse->CompressionSettings != TextureCompressionSettings::TC_Normalmap)
			{
				FText		title = LOCTEXT("import_distortion_texture_title", "PopcornFX: Compression Settings");
				const FText	message = FText::Format(LOCTEXT("import_distortion_texture_message",
					"Texture \"{0}\" is used for distortion but has incompatible compression settings. Do you want to import it using NormalMap compression settings instead?"),
					FText::FromString(mat.TextureDiffuse->GetPathName()));

				if (FMessageDialog::Open(EAppMsgType::YesNo, message, &title) == EAppReturnType::Yes)
				{
					mat.TextureDiffuse->Modify();
					mat.TextureDiffuse->CompressionSettings = TextureCompressionSettings::TC_Normalmap;
				}
			}
		}

		if (_GetPropertyValueAsBool(decl, PopcornFX::BasicRendererProperties::SID_AlphaRemap()))
			mat.TextureAlphaRemapper = LoadTexturePk(_GetPropertyValueAsPath(decl, PopcornFX::BasicRendererProperties::SID_AlphaRemap_AlphaMap()));

		EPopcornFXMaterialType::Type	finalMatType = mat.MaterialType;
		switch (mat.MaterialType)
		{
		case	EPopcornFXMaterialType::Billboard_Additive_NoAlpha:
		{
			finalMatType = EPopcornFXMaterialType::Billboard_Additive;
			mat.NoAlpha = true;
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		}
		case	EPopcornFXMaterialType::Billboard_Additive:
			if (mat.Lit)
			{
				// Warn user LIT activated on an additive material ?
			}
			break;
		case	EPopcornFXMaterialType::Billboard_AlphaBlend:
			break;
		case	EPopcornFXMaterialType::Billboard_AlphaBlendAdditive:
			if (mat.Lit)
				finalMatType = EPopcornFXMaterialType::Billboard_AlphaBlendAdditive_Lit;
			break;
		case	EPopcornFXMaterialType::Billboard_Distortion:
			PK_ASSERT(!mat.Lit);
			break;
		case	EPopcornFXMaterialType::Billboard_Solid:
			if (mat.Lit)
				finalMatType = EPopcornFXMaterialType::Billboard_Solid_Lit;
			break;
		case	EPopcornFXMaterialType::Billboard_Masked:
			if (mat.Lit)
				finalMatType = EPopcornFXMaterialType::Billboard_Masked_Lit;
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			return false;
		}
		mat.MaterialType = finalMatType;
		mat.SortIndices = mat.MaterialType >= EPopcornFXMaterialType::Billboard_AlphaBlend && mat.MaterialType <= EPopcornFXMaterialType::Billboard_AlphaBlendAdditive_Lit;

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
	: Material(null)
	, MaterialType()
	, TextureDiffuse(null)
	, TextureDiffuseRamp(null)
	, TextureEmissive(null)
	, TextureNormal(null)
	, TextureSpecular(null)
	, TextureAlphaRemapper(null)
	, TextureMotionVectors(null)
	, TextureSixWay_RLTS(null)
	, TextureSixWay_BBF(null)
	, VATTexturePosition(null)
	, VATTextureNormal(null)
	, VATTextureColor(null)
	, VATTextureRotation(null)
	, TextureAtlas(null)
	, NoAlpha(false)
	, MeshAtlas(false)
	, Raytraced(false)
	, SoftAnimBlending(false)
	, MotionVectorsBlending(false)
	, CastShadow(false)
	, Lit(false)
	, SortIndices(false)
	, CorrectDeformation(false)
	, SoftnessDistance(0)
	, SphereMaskHardness(100.0f)
	, MVDistortionStrengthColumns(1.0f)
	, MVDistortionStrengthRows(1.0f)
	, VATNumFrames(0)
	, VATPackedData(false)
	, VATPivotBoundsMin(0.0f)
	, VATPivotBoundsMax(0.0f)
	, VATNormalizedData(false)
	, VATPositionBoundsMin(0.0f)
	, VATPositionBoundsMax(0.0f)
	, MaskThreshold(0.0f)
	, DrawOrder(0)
	, StaticMesh(null)
	, StaticMeshLOD(0)
	, m_RMId(-1)
	, MaterialInstance(null)
{

}

//----------------------------------------------------------------------------
#if WITH_EDITOR

bool	FPopcornFXSubRendererMaterial::_ResetDefaultMaterial_NoReload()
{
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
	UMaterialInterface		*defaultMaterial = FPopcornFXPlugin::Get().Settings()->GetConfigDefaultMaterial(MaterialType.GetValue());
	return defaultMaterial;
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
	PK_RENAME_MAT_PROP_IFN(TextureSpecular, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(TextureAtlas, UPopcornFXTextureAtlas);
	PK_RENAME_MAT_PROP_IFN(TextureMotionVectors, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(TextureSixWay_RLTS, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(TextureSixWay_BBF, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(VATTexturePosition, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(VATTextureNormal, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(VATTextureColor, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(VATTextureRotation, UTexture2D);
	PK_RENAME_MAT_PROP_IFN(StaticMesh, UStaticMesh);

#undef PK_RENAME_MAT_PROP_IFN
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

bool	FPopcornFXSubRendererMaterial::CanBeMergedWith(const FPopcornFXSubRendererMaterial &other) const
{
	return
		m_RMId == other.m_RMId &&
		MaterialType == other.MaterialType &&
		TextureDiffuse == other.TextureDiffuse &&
		TextureDiffuseRamp == other.TextureDiffuseRamp &&
		TextureEmissive == other.TextureEmissive &&
		TextureNormal == other.TextureNormal &&
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
		VATNumFrames == other.VATNumFrames &&
		VATPackedData == other.VATPackedData &&
		VATPivotBoundsMin == other.VATPivotBoundsMin &&
		VATPivotBoundsMax == other.VATPivotBoundsMax &&
		VATNormalizedData == other.VATNormalizedData &&
		VATPositionBoundsMin == other.VATPositionBoundsMin &&
		VATPositionBoundsMax == other.VATPositionBoundsMax &&
		CastShadow == other.CastShadow &&
		SoftnessDistance == other.SoftnessDistance &&
		MaskThreshold == other.MaskThreshold &&
		DrawOrder == other.DrawOrder &&
		StaticMesh == other.StaticMesh &&
		StaticMeshLOD == other.StaticMeshLOD;
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

	// ReloadInstances();
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

const EPopcornFXMaterialType::Type		kTransparentBillboard_Material_ToUE[] =
{
	EPopcornFXMaterialType::Billboard_Additive,				//Additive
	EPopcornFXMaterialType::Billboard_Additive_NoAlpha,		//AdditiveNoAlpha
	EPopcornFXMaterialType::Billboard_AlphaBlend,			//AlphaBlend
	EPopcornFXMaterialType::Billboard_AlphaBlendAdditive,	//AlphaBlend_Additive
};
PK_STATIC_ASSERT(PK_ARRAY_COUNT(kTransparentBillboard_Material_ToUE) == PopcornFX::BasicRendererProperties::ETransparentType::__MaxTransparentType);

//----------------------------------------------------------------------------

const EPopcornFXMaterialType::Type		kOpaqueBillboard_Material_ToUE[] =
{
	EPopcornFXMaterialType::Billboard_Solid,				// Solid
	EPopcornFXMaterialType::Billboard_Masked,				// Masked
};
PK_STATIC_ASSERT(PK_ARRAY_COUNT(kOpaqueBillboard_Material_ToUE) == PopcornFX::BasicRendererProperties::EOpaqueType::__MaxOpaqueType);

//----------------------------------------------------------------------------
//
// Mesh
//
//----------------------------------------------------------------------------

const EPopcornFXMaterialType::Type		kOpaqueMesh_Material_ToUE[] =
{
	EPopcornFXMaterialType::Mesh_Solid,				// Solid,
	EPopcornFXMaterialType::Mesh_Masked,			// Masked,
};
PK_STATIC_ASSERT(PK_ARRAY_COUNT(kOpaqueMesh_Material_ToUE) == PopcornFX::BasicRendererProperties::EOpaqueType::__MaxOpaqueType);

//----------------------------------------------------------------------------
//
// (Helper)
//
//----------------------------------------------------------------------------

UTexture2D		*LoadTexturePk(const PopcornFX::CString &pkPath)
{
	if (pkPath.Empty())
		return null;

	UObject			*obj = FPopcornFXPlugin::Get().LoadUObjectFromPkPath(pkPath.Data(), false);
	UTexture2D		*texture = Cast<UTexture2D>(obj);
	if (obj != null && texture == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Asset is not a texture: '%s'"), ANSI_TO_TCHAR(pkPath.Data()));
		return null;
	}
	if (texture == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Could not load texture '%s'"), ANSI_TO_TCHAR(pkPath.Data()));
		return null;
	}
	return texture;
}

//----------------------------------------------------------------------------

UPopcornFXTextureAtlas		*LoadAtlasPk(const PopcornFX::CString &pkPath)
{
	if (pkPath.Empty())
		return null;

	UObject					*obj = FPopcornFXPlugin::Get().LoadUObjectFromPkPath(pkPath.Data(), false);
	UPopcornFXTextureAtlas	*atlas = Cast<UPopcornFXTextureAtlas>(obj);
	if (obj != null && atlas == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Asset is not an atlas: '%s'"), ANSI_TO_TCHAR(pkPath.Data()));
		return null;
	}
	if (atlas == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Could not load atlas '%s'"), ANSI_TO_TCHAR(pkPath.Data()));
		return null;
	}
	return atlas;
}

//----------------------------------------------------------------------------

UStaticMesh		*LoadMeshPk(const PopcornFX::CString &pkPath)
{
	if (pkPath.Empty())
		return null;

	UObject			*obj = FPopcornFXPlugin::Get().LoadUObjectFromPkPath(pkPath.Data(), false);
	UStaticMesh		*mesh = Cast<UStaticMesh>(obj);
	if (obj != null && mesh == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Asset is not a mesh: '%s'"), ANSI_TO_TCHAR(pkPath.Data()));
		return null;
	}
	if (mesh == null)
	{
		UE_LOG(LogPopcornFXRendererMaterial, Warning, TEXT("Could not load mesh '%s'"), ANSI_TO_TCHAR(pkPath.Data()));
		return null;
	}
	return mesh;
}

//----------------------------------------------------------------------------

void	SetMaterialTextureParameter(UMaterialInstanceDynamic *mat, FName textureName, const PopcornFX::CString &pkTexturePath)
{
	mat->SetTextureParameterValue(textureName, LoadTexturePk(pkTexturePath.Data()));
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
