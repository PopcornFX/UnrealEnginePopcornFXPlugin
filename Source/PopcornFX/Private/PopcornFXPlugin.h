//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "IPopcornFXPlugin.h"
#include "PopcornFXSettingsEditor.h"
#include "PopcornFXSettings.h"
#include "RendererInterface.h"

#if (ENGINE_MAJOR_VERSION == 5)
#	include "UObject/ObjectSaveContext.h"
#endif // (ENGINE_MAJOR_VERSION == 5)

#include "PopcornFXSDK.h"
#include <pk_base_object/include/hbo_file.h>
#include <pk_base_object/include/hbo_object.h>

#if (PK_GPU_D3D11 != 0)
#	include <pk_particles/include/Storage/D3D11/storage_d3d11_stream.h>
#endif // (PK_GPU_D3D11 != 0)

FWD_PK_API_BEGIN
class	CParticleSceneInterface;
PK_FORWARD_DECLARE(FilePack);
FWD_PK_API_END
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

class	UPopcornFXFile;
class	FPopcornFXDependencyModule;
class	CPopcornFXProfiler;
#if WITH_EDITOR
class	UPopcornFXSettingsEditor;
#endif

DECLARE_MULTICAST_DELEGATE(FOnPopcornFXSettingChange);

#if 0
struct	SUEViewUniformData
{
	CFloat4		m_InvDeviceZToWorldZTransform;
	CFloat4x4	m_TranslatedViewProjectionMatrix;
	CFloat4x4	m_ScreenToWorld;
	CFloat4		m_PreViewTranslation;
	CFloat4		m_ScreenPositionScaleBias;
};
#endif

class	FPopcornFXPlugin : public IPopcornFXPlugin
{
private:
	static FPopcornFXPlugin				*s_Self;
	static float						s_GlobalScale;
	static float						s_GlobalScaleRcp;
	static FString						s_PluginVersionString;
	static FString						s_PopornFXRuntimeVersionString;
	static uint32						s_PopornFXRuntimeRevID;
	static uint16						s_PopornFXRuntimeMajMinPatch;

	static FString						s_URLDocumentation;
	static FString						s_URLPluginWiki;
	static FString						s_URLDiscord;

public:
	static FPopcornFXPlugin				&Get() { check(s_Self != null); return *s_Self; }
	static float						GlobalScale() { return s_GlobalScale; }
	static float						GlobalScaleRcp() { return s_GlobalScaleRcp; }

	static const FString				&PluginVersionString() { return s_PluginVersionString; }
	static const FString				&PopornFXRuntimeVersionString() { return s_PopornFXRuntimeVersionString; }
	static const uint32					&PopornFXRuntimeRevID() { return s_PopornFXRuntimeRevID; }
	static const uint16					&PopornFXRuntimeMajMinPatch() { return s_PopornFXRuntimeMajMinPatch; }

	static const FString				&DocumentationURL() { return s_URLDocumentation; }
	static const FString				&PluginWikiURL() { return s_URLPluginWiki; }
	static const FString				&DiscordURL() { return s_URLDiscord; }

	static int32						TotalParticleCount();
	static void							IncTotalParticleCount(s32 newTotalParticleCount);

	static void							RegisterRenderThreadIFN();
	static void							RegisterCurrentThreadAsUserIFN();

	static bool							IsMainThread();

#if WITH_EDITOR
	static const FString				&PopcornFXPluginRoot();
	static const FString				&PopcornFXPluginContentDir();
#endif // WITH_EDITOR

public:
	FOnPopcornFXSettingChange			m_OnSettingsChanged;

public:
	FPopcornFXPlugin();

	virtual void						StartupModule() override;
	virtual void						ShutdownModule() override;

	static PopcornFX::SEngineVersion	PopcornFXBuildVersion();

	const UPopcornFXSettings			*Settings();
#if WITH_EDITOR
	const UPopcornFXSettingsEditor		*SettingsEditor();

	class PopcornFX::HBO::CContext		*BakeContext();
#endif // WITH_EDITOR
	PopcornFX::PFilePack				FilePack();

	/**
	*	Take a PopcornFX file path (virtual or not)
	*	return the corresponding path for Unreal assets
	*/
	FString								BuildPathFromPkPath(const char *pkPath, bool prependPackPath);

	PopcornFX::PBaseObjectFile			FindPkFile(const UPopcornFXFile *file);
	PopcornFX::PBaseObjectFile			LoadPkFile(const UPopcornFXFile *file, bool reload = false);
	void								UnloadPkFile(const UPopcornFXFile *file);

	UPopcornFXFile						*GetPopcornFXFile(const PopcornFX::CBaseObjectFile *file);

	UObject								*LoadUObjectFromPkPath(const char *pkPath, bool notVirtual);

	void								NotifyObjectChanged(UObject *object);

#if WITH_EDITOR
	virtual TSharedRef<class IPopcornFXEffectEditor>	CreateEffectEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, class UPopcornFXEffect* Effect) override;
#endif

	bool								HandleSettingsModified();

	static FName						Name_DiffuseTexture() { static FName n(TEXT("DiffuseTexture")); return n; }
	static FName						Name_DiffuseTextureRamp() { static FName n(TEXT("DiffuseTextureRamp")); return n; }
	static FName						Name_EmissiveTexture() { static FName n(TEXT("EmissiveTexture")); return n; }
	static FName						Name_NormalTexture() { static FName n(TEXT("NormalTexture")); return n; }
	static FName						Name_SpecularTexture() { static FName n(TEXT("SpecularTexture")); return n; }

	static FName						Name_VATPositionTexture() { static FName n(TEXT("VATPositionTexture")); return n; }
	static FName						Name_VATNormalTexture() { static FName n(TEXT("VATNormalTexture")); return n; }
	static FName						Name_VATColorTexture() { static FName n(TEXT("VATColorTexture")); return n; }
	static FName						Name_VATRotationTexture() { static FName n(TEXT("VATRotationTexture")); return n; }
	static FName						Name_VATNumFrames() { static FName n(TEXT("VATNumFrames")); return n; }
	static FName						Name_VATPackedData() { static FName n(TEXT("VATPackedData")); return n; }
	static FName						Name_VATPivotBoundsMin() { static FName n(TEXT("VATPivotBoundsMin")); return n; }
	static FName						Name_VATPivotBoundsMax() { static FName n(TEXT("VATPivotBoundsMax")); return n; }
	static FName						Name_VATPositionBoundsMin() { static FName n(TEXT("VATPositionBoundsMin")); return n; }
	static FName						Name_VATPositionBoundsMax() { static FName n(TEXT("VATPositionBoundsMax")); return n; }

	static FName						Name_AlphaRemapper() { static FName n(TEXT("AlphaRemapper")); return n; }
	static FName						Name_MotionVectorsTexture() { static FName n(TEXT("MotionVectorsTexture")); return n; }
	static FName						Name_SixWayLightmap_RLTSTexture() { static FName n(TEXT("PopcornFX_SixWay_RLTSTexture")); return n; }
	static FName						Name_SixWayLightmap_BBFTexture() { static FName n(TEXT("PopcornFX_SixWay_BBFTexture")); return n; }
	static FName						Name_SoftnessDistance() { static FName n(TEXT("PopcornFX_SoftnessDistance")); return n; }
	static FName						Name_SphereMaskHardness() { static FName n(TEXT("PopcornFX_SphereMaskHardness")); return n; }
	static FName						Name_MaskThreshold() { static FName n(TEXT("PopcornFX_MaskThreshold")); return n; }
	static FName						Name_MVDistortionStrengthColumns() { static FName n(TEXT("PopcornFX_MVDistortionStrengthColumns")); return n; }
	static FName						Name_MVDistortionStrengthRows() { static FName n(TEXT("PopcornFX_MVDistortionStrengthRows")); return n; }

	static FName						Name_POPCORNFX_IS_SOFT_PARTICLES() { static FName n(TEXT("POPCORNFX_IS_SOFT_PARTICLES")); return n; }
	static FName						Name_POPCORNFX_IS_SOFT_ANIM() { static FName n(TEXT("POPCORNFX_IS_SOFT_ANIM")); return n; }
	static FName						Name_POPCORNFX_IS_MOTION_VECTORS_BLENDING() { static FName n(TEXT("POPCORNFX_IS_MOTION_VECTORS_BLENDING")); return n; }
	static FName						Name_POPCORNFX_IS_LIT() { static FName n(TEXT("POPCORNFX_IS_LIT")); return n; }
	static FName						Name_POPCORNFX_HAS_NORMAL_TEXTURE() { static FName n(TEXT("POPCORNFX_HAS_NORMAL_TEXTURE")); return n; }
	static FName						Name_POPCORNFX_HAS_ALPHA_REMAPPER() { static FName n(TEXT("POPCORNFX_HAS_ALPHA_REMAPPER")); return n; }
	static FName						Name_POPCORNFX_HAS_DIFFUSE_RAMP() { static FName n(TEXT("POPCORNFX_HAS_DIFFUSE_RAMP")); return n; }
	static FName						Name_POPCORNFX_HAS_EMISSIVE_TEXTURE() { static FName n(TEXT("POPCORNFX_HAS_EMISSIVE_TEXTURE")); return n; }
	static FName						Name_POPCORNFX_HAS_SIXWAYLIGHTMAP_TEXTURES() { static FName n(TEXT("POPCORNFX_HAS_SIXWAYLIGHTMAP_TEXTURES")); return n; }
	static FName						Name_POPCORNFX_IS_NO_ALPHA() { static FName n(TEXT("POPCORNFX_IS_NO_ALPHA")); return n; }
	static FName						Name_POPCORNFX_CAST_SHADOW() { static FName n(TEXT("POPCORNFX_CAST_SHADOW")); return n; }
	static FName						Name_POPCORNFX_HAS_VAT_NORMALIZEDDATA() { static FName n(TEXT("POPCORNFX_HAS_VAT_NORMALIZEDDATA")); return n; }
	static FName						Name_POPCORNFX_HAS_VAT_PACKEDDATA() { static FName n(TEXT("POPCORNFX_HAS_VAT_PACKEDDATA")); return n; }
	static FName						Name_POPCORNFX_HAS_SKELMESH_ANIM() { static FName n(TEXT("POPCORNFX_HAS_SKELMESH_ANIM")); return n; }

	static FName						Name_POPCORNFX_IS_BILLBOARD() { static FName n(TEXT("POPCORNFX_IS_BILLBOARD")); return n; }
	static FName						Name_POPCORNFX_IS_RIBBON() { static FName n(TEXT("POPCORNFX_IS_RIBBON")); return n; }
	static FName						Name_POPCORNFX_IS_MESH() { static FName n(TEXT("POPCORNFX_IS_MESH")); return n; }
	static FName						Name_POPCORNFX_IS_TRIANGLE() { static FName n(TEXT("POPCORNFX_IS_TRIANGLE")); return n; }

	static const FLinearColor			&Color_PopcornFX() { static const FLinearColor c(0.011765f, 0.501961f, 0.839216f, 1.0f); return c; }

#if WITH_EDITOR
	static void							SetAskImportAssetDependencies_YesAll(bool yesAll);
	static bool							AskImportAssetDependencies_YesAll();
	static void							SetAskBuildMeshData_YesAll(bool yesAll);
	static bool							AskBuildMeshData_YesAll();
#endif

#if 0
	class FRHITexture2D						*SceneDepthTexture() { return m_SceneDepthTexture; }
	class FRHITexture2D						*SceneNormalTexture() { return m_SceneNormalTexture; }
#if (PK_GPU_D3D11 != 0)
	PopcornFX::SParticleStreamBuffer_D3D11	&ViewConstantBuffer_D3D11() { return m_ViewConstantBuffer_D3D11; }
#endif // (PK_GPU_D3D11 != 0)
	void									SetViewUniformData(const SUEViewUniformData &viewUniformData) { m_ViewUniformData = viewUniformData; }
	const SUEViewUniformData				&ViewUniformData() const { return m_ViewUniformData; }
#endif

	CPopcornFXProfiler					*Profiler();
	void								ActivateEffectsProfiler(bool activate);
	bool								EffectsProfilerActive() const;

private:
	bool								LoadSettingsAndPackIFN();
#if 0
	void								PostOpaqueRender(class FPostOpaqueRenderParameters &params);
#endif
	void								OnPostEngineInit();

private:
	bool								m_RootPackLoaded;
	bool								m_LaunchedPopcornFX;

	const UPopcornFXSettings			*m_Settings;
#if WITH_EDITORONLY_DATA
	const UPopcornFXSettingsEditor		*m_SettingsEditor;
	bool								m_PackIsUpToDate;

	struct SBakeContext					*m_BakeContext;
#endif // WITH_EDITORONLY_DATA
	FString								m_PackPath;
	PopcornFX::PFilePack				m_FilePack;
	FString								m_FilePackPath;

#if 0
	// Main
	class FRHITexture2D						*m_SceneDepthTexture;
	class FRHITexture2D						*m_SceneNormalTexture;
#if (PK_GPU_D3D11 != 0)
	PopcornFX::SParticleStreamBuffer_D3D11	m_ViewConstantBuffer_D3D11;
#endif // (PK_GPU_D3D11 != 0)
	SUEViewUniformData						m_ViewUniformData;
	FPostOpaqueRenderDelegate				m_PostOpaqueRenderDelegate;
#endif

#if WITH_EDITOR
	// Global Editor callbacks
	FDelegateHandle						m_OnObjectSavedDelegateHandle;
#if (ENGINE_MAJOR_VERSION == 5)
	void								_OnObjectSaved(UObject *object, FObjectPreSaveContext context);
#else
	void								_OnObjectSaved(UObject *object);
#endif // (ENGINE_MAJOR_VERSION == 5)
	void								_UpdateSimInterfaceBindings(const FString &libraryDir);
#endif // WITH_EDITOR

	CPopcornFXProfiler					*m_Profiler;
	bool								m_EffectsProfilerActive = false;

	bool								_LinkSimInterfaces();
};
