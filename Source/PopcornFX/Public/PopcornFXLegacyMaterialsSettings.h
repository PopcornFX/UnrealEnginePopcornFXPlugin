//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#pragma once

#include "Runtime/Launch/Resources/Version.h"

#include "UObject/SoftObjectPath.h"

#include "PopcornFXLegacyMaterialsSettings.generated.h"

USTRUCT()
struct FPopcornFXLegacyMaterialsSettings
{
	GENERATED_USTRUCT_BODY()

	FPopcornFXLegacyMaterialsSettings() { }

	/** Additive Billboard Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Billboard_Additive;

	/** AlphaBlend Billboard Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Billboard_AlphaBlend;

	/** Lit AlphaBlend Billboard Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Billboard_AlphaBlend_Lit;

	/** AlphaBlend Additive Billboard Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Billboard_AlphaBlendAdditive;

	/** Lit AlphaBlend Additive Billboard Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Billboard_AlphaBlendAdditive_Lit;

	/** Distortion Billboard Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Billboard_Distortion;

	/** Solid Billboard Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Billboard_Solid;

	/** Lit Solid Billboard Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Billboard_Solid_Lit;

	/** Masked Billboard Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Billboard_Masked;

	/** Lit Masked Billboard Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Billboard_Masked_Lit;

	/** Volumetric Billboard Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Billboard_VolumetricFog;

	/** SixWayLightmap Billboard Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Billboard_SixWayLightmap;

	/** Additive Mesh Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_Additive;

	/** AlphaBlend Mesh Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_AlphaBlend;

	/** Distortion Mesh Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_Distortion;

	/** Solid Mesh Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_Solid;

	/** Lit Solid Mesh Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_Solid_Lit;

	/** Masked Mesh Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_Masked;

	/** Lit Masked Mesh Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_Masked_Lit;

	/** VAT Opaque Fluid Vertex Animation Texture Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_VAT_Opaque_Fluid;

	/** Lit VAT Opaque Fluid Vertex Animation Texture Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_VAT_Opaque_Fluid_Lit;

	/** VAT Masked Fluid Vertex Animation Texture Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_VAT_Masked_Fluid;

	/** Lit VAT Masked Fluid Vertex Animation Texture Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_VAT_Masked_Fluid_Lit;

	/** VAT Opaque Soft Vertex Animation Texture Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_VAT_Opaque_Soft;

	/** Lit VAT Opaque Soft Vertex Animation Texture Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_VAT_Opaque_Soft_Lit;

	/** VAT Masked Soft Vertex Animation Texture Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_VAT_Masked_Soft;

	/** Lit VAT Masked Soft Vertex Animation Texture Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_VAT_Masked_Soft_Lit;

	/** VAT Opaque Rigid Vertex Animation Texture Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_VAT_Opaque_Rigid;

	/** Lit VAT Opaque Rigid Vertex Animation Texture Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_VAT_Opaque_Rigid_Lit;

	/** VAT Masked Rigid Vertex Animation Texture Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_VAT_Masked_Rigid;

	/** Lit VAT Opaque Rigid Vertex Animation Texture Legacy Material. */
	UPROPERTY(Config, EditAnywhere, Category="Legacy Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Legacy_Mesh_VAT_Masked_Rigid_Lit;
};
