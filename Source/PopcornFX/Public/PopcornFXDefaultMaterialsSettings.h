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
	FSoftObjectPath			Material_Default_Billboard_Additive;

	/** AlphaBlend Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Billboard_AlphaBlend;

	/** Lit AlphaBlend Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Billboard_AlphaBlend_Lit;

	/** AlphaBlend Additive Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Billboard_AlphaBlendAdditive;

	/** Lit AlphaBlend Additive Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Billboard_AlphaBlendAdditive_Lit;

	/** Solid Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Billboard_Solid;

	/** Lit Solid Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Billboard_Solid_Lit;

	/** Masked Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Billboard_Masked;

	/** Lit Masked Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Billboard_Masked_Lit;
	
	/** Distortion Billboard Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Billboard_Distortion; 

	/** Additive Mesh Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Mesh_Additive;

	/** AlphaBlend Mesh Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Mesh_AlphaBlend;

	/** Solid Mesh Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Mesh_Solid;

	/** Lit Solid Mesh Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Mesh_Solid_Lit;

	/** Masked Mesh Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Mesh_Masked;

	/** Lit Masked Mesh Default Material. */
	UPROPERTY(Config, EditAnywhere, Category="Default Materials", meta=(AllowedClasses="MaterialInterface"))
	FSoftObjectPath			Material_Default_Mesh_Masked_Lit;

};
