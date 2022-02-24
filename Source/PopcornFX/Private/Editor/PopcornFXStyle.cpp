//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXStyle.h"

#include "PopcornFXPlugin.h"

#include "Styling/SlateStyleRegistry.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( m_StyleSet->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__ )
#define IMAGE_PLUGIN_BRUSH( RelativePath, ... ) FSlateImageBrush( FPopcornFXStyle::InContent(RelativePath, ".png"), __VA_ARGS__ )

//----------------------------------------------------------------------------

TSharedPtr<FSlateStyleSet>	FPopcornFXStyle::m_StyleSet;

//----------------------------------------------------------------------------

FString	FPopcornFXStyle::InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
	return (FPopcornFXPlugin::PopcornFXPluginContentDir() / RelativePath) + Extension;
}

//----------------------------------------------------------------------------

void	FPopcornFXStyle::Initialize()
{
	if (!m_StyleSet.IsValid())
	{
		m_StyleSet = MakeShareable(new FSlateStyleSet(TEXT("PopcornFXStyle")));
		m_StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
		m_StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

		const FVector2D		Icon16x16(16.0f, 16.0f);
		const FVector2D		Icon20x20(20.0f, 20.0f);
		const FVector2D		Icon32x32(32.0f, 32.0f);
		const FVector2D		Icon40x40(40.0f, 40.0f);
		const FVector2D		Icon64x64(64.0f, 64.0f);

		m_StyleSet->Set("ClassIcon.PopcornFXFile", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_16x"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXFile", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_64x"), Icon64x64));

		m_StyleSet->Set("ClassIcon.PopcornFXEffect", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_pkfx_16x"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXEffect", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_pkfx_64x"), Icon64x64));

		m_StyleSet->Set("ClassIcon.PopcornFXTextureAtlas", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_pkat_16x"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXTextureAtlas", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_pkat_64x"), Icon64x64));

		m_StyleSet->Set("ClassIcon.PopcornFXAnimTrack", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_Pkan_16x"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXAnimTrack", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_Pkan_64x"), Icon64x64));

		m_StyleSet->Set("ClassIcon.PopcornFXSimulationCache", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_Pksc_16x"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXSimulationCache", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_Pksc_64x"), Icon64x64));

		m_StyleSet->Set("ClassIcon.PopcornFXFont", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_Pkfm_16x"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXFont", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_Pkfm_64x"), Icon64x64));

		m_StyleSet->Set("ClassIcon.PopcornFXEmitter", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_16x"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXEmitter", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_64x"), Icon64x64));

		m_StyleSet->Set("ClassIcon.PopcornFXSceneActor", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_16x"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXSceneActor", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_64x"), Icon64x64));

		m_StyleSet->Set("ClassIcon.PopcornFXAudioSamplerActor", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_16x"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXAudioSamplerActor", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_64x"), Icon64x64));

		m_StyleSet->Set("ClassIcon.PopcornFXWaitForScene", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_16x"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXWaitForScene", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_64x"), Icon64x64));

		m_StyleSet->Set("ClassIcon.PopcornFXEmitterComponent", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_16x"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXEmitterComponent", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_64x"), Icon64x64));

		m_StyleSet->Set("ClassIcon.PopcornFXAttributeSampler", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/CParticleSamplerShape.med"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXAttributeSampler", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/CParticleSamplerShape.med"), Icon64x64));

		m_StyleSet->Set("ClassIcon.PopcornFXAttributeSamplerActor", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/CParticleSamplerShape.med"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXAttributeSamplerActor", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/CParticleSamplerShape.med"), Icon64x64));

		m_StyleSet->Set("PopcornFX.BadIcon32", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/BadIcon"), Icon32x32));

		// Abstract, should not be used:
		m_StyleSet->Set("ClassIcon.PopcornFXAttributeSampler", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/BadIcon"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXAttributeSampler", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/BadIcon"), Icon64x64));
		m_StyleSet->Set("ClassIcon.PopcornFXAttributeSamplerActor", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/BadIcon"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXAttributeSamplerActor", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/BadIcon"), Icon64x64));

#define ATTRIBSAMPLER_STYLE(__name, __image)																				\
		m_StyleSet->Set("ClassIcon." __name "", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/") TEXT(__image) TEXT(".med"), Icon16x16));				\
		m_StyleSet->Set("ClassThumbnail." __name "", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/") TEXT(__image) TEXT(".med"), Icon64x64));		\
		m_StyleSet->Set("ClassIcon." __name "Actor", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/") TEXT(__image) TEXT(".med"), Icon16x16));		\
		m_StyleSet->Set("ClassThumbnail." __name "Actor", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/") TEXT(__image) TEXT(".med"), Icon64x64));

		ATTRIBSAMPLER_STYLE("PopcornFXAttributeSamplerShape", "CParticleSamplerShape");
		ATTRIBSAMPLER_STYLE("PopcornFXAttributeSamplerImage", "CParticleSamplerTexture");
		ATTRIBSAMPLER_STYLE("PopcornFXAttributeSamplerAnimTrack", "CParticleSamplerAnimTrack");
		ATTRIBSAMPLER_STYLE("PopcornFXAttributeSamplerCurve", "CParticleSamplerCurve");
		ATTRIBSAMPLER_STYLE("PopcornFXAttributeSamplerText", "CParticleSamplerText");
		ATTRIBSAMPLER_STYLE("PopcornFXAttributeSamplerVectorField", "CParticleSamplerProceduralTurbulence");
		ATTRIBSAMPLER_STYLE("PopcornFXAttributeSamplerSkinnedMesh", "CParticleSamplerSkinnedMesh");

#undef ATTRIBSAMPLER_STYPE

		// No actor, is a component only
		m_StyleSet->Set("ClassIcon.PopcornFXAttributeSamplerCurveDynamic", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/CParticleSamplerCurve.med"), Icon16x16));
		m_StyleSet->Set("ClassThumbnail.PopcornFXAttributeSamplerCurveDynamic", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/CParticleSamplerCurve.med"), Icon64x64));

		m_StyleSet->Set("PopcornFX.Attribute.F1", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/AttributeF1"), Icon32x32));
		m_StyleSet->Set("PopcornFX.Attribute.F2", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/AttributeF2"), Icon32x32));
		m_StyleSet->Set("PopcornFX.Attribute.F3", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/AttributeF3"), Icon32x32));
		m_StyleSet->Set("PopcornFX.Attribute.F4", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/AttributeF4"), Icon32x32));

		m_StyleSet->Set("PopcornFX.Attribute.I1", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/AttributeI1"), Icon32x32));
		m_StyleSet->Set("PopcornFX.Attribute.I2", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/AttributeI2"), Icon32x32));
		m_StyleSet->Set("PopcornFX.Attribute.I3", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/AttributeI3"), Icon32x32));
		m_StyleSet->Set("PopcornFX.Attribute.I4", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/AttributeI4"), Icon32x32));

		m_StyleSet->Set("PopcornFX.Attribute.B1", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/AttributeB1"), Icon32x32));
		m_StyleSet->Set("PopcornFX.Attribute.B2", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/AttributeB2"), Icon32x32));
		m_StyleSet->Set("PopcornFX.Attribute.B3", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/AttributeB3"), Icon32x32));
		m_StyleSet->Set("PopcornFX.Attribute.B4", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/AttributeB4"), Icon32x32));

		m_StyleSet->Set("PopcornFXSequencer.StartEmitter", new IMAGE_BRUSH("Icons/generic_play_16x", Icon16x16));
		m_StyleSet->Set("PopcornFXSequencer.StopEmitter", new IMAGE_BRUSH("Icons/generic_stop_16x", Icon16x16));

		m_StyleSet->Set("PopcornFXEffectEditor.TogglePreviewGrid", new IMAGE_BRUSH("Icons/icon_MatEd_Grid_40x", Icon32x32));
		m_StyleSet->Set("PopcornFXEffectEditor.TogglePreviewGrid.Small", new IMAGE_BRUSH("Icons/icon_MatEd_Grid_40x", Icon16x16));

		m_StyleSet->Set("PopcornFXEffectEditor.ToggleDrawBounds", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/EffectEditor_Bounds"), Icon16x16));
		m_StyleSet->Set("PopcornFXEffectEditor.ToggleDrawBounds.Small", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/EffectEditor_Bounds"), Icon16x16));

		m_StyleSet->Set("PopcornFXEffectEditor.ResetEmitter", new IMAGE_BRUSH("Icons/icon_Cascade_RestartInLevel_40x", Icon40x40));
		m_StyleSet->Set("PopcornFXEffectEditor.ResetEmitter.Small", new IMAGE_BRUSH("Icons/icon_Cascade_RestartInLevel_40x", Icon20x20));

		m_StyleSet->Set("PopcornFXEffectEditor.ReimportEffect", new IMAGE_BRUSH("Icons/icon_TextureEd_Reimport_40x", Icon40x40));
		m_StyleSet->Set("PopcornFXEffectEditor.ReimportEffect.Small", new IMAGE_BRUSH("Icons/icon_TextureEd_Reimport_40x", Icon20x20));

		m_StyleSet->Set("PopcornFXEffectEditor.OpenInPopcornFXEditor", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_64x"), Icon40x40));
		m_StyleSet->Set("PopcornFXEffectEditor.OpenInPopcornFXEditor.Small", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/icon_PopcornFX_Logo_16x"), Icon20x20));

		m_StyleSet->Set("PopcornFXLevelEditor.OpenSourcePack", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/LevelEditor_OpenSourcePack_40x"), Icon40x40));
		m_StyleSet->Set("PopcornFXLevelEditor.OpenSourcePack.Small", new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/LevelEditor_OpenSourcePack_20x"), Icon20x20));

#define STYLE_PKFX_NODE(__node)		\
		m_StyleSet->Set("PopcornFX.Node." __node, new IMAGE_PLUGIN_BRUSH(TEXT("SlateBrushes/") TEXT(__node) TEXT(".med"), Icon32x32))

		STYLE_PKFX_NODE("CParticleSamplerCurve");
		STYLE_PKFX_NODE("CParticleSamplerProceduralTurbulence");
		STYLE_PKFX_NODE("CParticleSamplerShape");
		STYLE_PKFX_NODE("CParticleSamplerTexture");
		STYLE_PKFX_NODE("CParticleSamplerAnimTrack");
		STYLE_PKFX_NODE("CParticleSamplerText");
		STYLE_PKFX_NODE("CShapeDescriptor_Box");
		STYLE_PKFX_NODE("CShapeDescriptor_Capsule");
		STYLE_PKFX_NODE("CShapeDescriptor_Collection");
		STYLE_PKFX_NODE("CShapeDescriptor_Ellipsoid");
		STYLE_PKFX_NODE("CShapeDescriptor_Cone");
		STYLE_PKFX_NODE("CShapeDescriptor_Cylinder");
		STYLE_PKFX_NODE("CShapeDescriptor_Mesh");
		STYLE_PKFX_NODE("CShapeDescriptor_Sphere");

#undef STYLE_PKFX_NODE

		FSlateStyleRegistry::RegisterSlateStyle(*m_StyleSet.Get());
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXStyle::Shutdown()
{
	if (m_StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*m_StyleSet.Get());

		ensure(m_StyleSet.IsUnique());
		m_StyleSet.Reset();
	}
}

#endif // WITH_EDITOR
