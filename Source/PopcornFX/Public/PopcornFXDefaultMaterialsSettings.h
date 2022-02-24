//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#pragma once

#include "Runtime/Launch/Resources/Version.h"

#include "UObject/SoftObjectPath.h"

#include "PopcornFXDefaultMaterialsSettings.generated.h"

USTRUCT()
struct FPopcornFXDefaultMaterialsSettings
{
	GENERATED_USTRUCT_BODY()

	FPopcornFXDefaultMaterialsSettings() { }

	/** Additive Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Billboard_Additive;

	/** AlphaBlend Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Billboard_AlphaBlend;

	/** Lit AlphaBlend Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Billboard_AlphaBlend_Lit;

	/** AlphaBlend Additive Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Billboard_AlphaBlendAdditive;

	/** Lit AlphaBlend Additive Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Billboard_AlphaBlendAdditive_Lit;

	/** Distortion Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Billboard_Distortion;

	/** Solid Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Billboard_Solid;

	/** Lit Solid Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Billboard_Solid_Lit;

	/** Masked Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Billboard_Masked;

	/** Lit Masked Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Billboard_Masked_Lit;

	/** Volumetric Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Billboard_VolumetricFog;

	/** SixWayLightmap Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Billboard_SixWayLightmap;

	/** Additive Mesh Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_Additive;

	/** Distortion Mesh Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_Distortion;

	/** Solid Mesh Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_Solid;

	/** Lit Solid Mesh Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_Solid_Lit;

	/** Masked Mesh Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_Masked;

	/** Lit Masked Mesh Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_Masked_Lit;

	/** VAT Opaque Fluid Vertex Animation Texture Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_VAT_Opaque_Fluid;

	/** Lit VAT Opaque Fluid Vertex Animation Texture Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_VAT_Opaque_Fluid_Lit;

	/** VAT Masked Fluid Vertex Animation Texture Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_VAT_Masked_Fluid;

	/** Lit VAT Masked Fluid Vertex Animation Texture Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_VAT_Masked_Fluid_Lit;

	/** VAT Opaque Soft Vertex Animation Texture Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_VAT_Opaque_Soft;

	/** Lit VAT Opaque Soft Vertex Animation Texture Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_VAT_Opaque_Soft_Lit;

	/** VAT Masked Soft Vertex Animation Texture Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_VAT_Masked_Soft;

	/** Lit VAT Masked Soft Vertex Animation Texture Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_VAT_Masked_Soft_Lit;

	/** VAT Opaque Rigid Vertex Animation Texture Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_VAT_Opaque_Rigid;

	/** Lit VAT Opaque Rigid Vertex Animation Texture Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_VAT_Opaque_Rigid_Lit;

	/** VAT Masked Rigid Vertex Animation Texture Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_VAT_Masked_Rigid;

	/** Lit VAT Opaque Rigid Vertex Animation Texture Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Mesh_VAT_Masked_Rigid_Lit;
};
