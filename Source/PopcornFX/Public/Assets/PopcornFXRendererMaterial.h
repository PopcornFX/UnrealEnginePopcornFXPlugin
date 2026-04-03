//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "RenderCommandFence.h"
#include "Runtime/Launch/Resources/Version.h"

#include "PopcornFXRendererMaterial.generated.h"

namespace PopcornFX
{
	struct	SRendererDeclaration;
	class	CRendererDataBase;
}

class	UMaterialInterface;
class	UMaterialInstanceConstant;
class	UStaticMesh;
class	USkeletalMesh;
class	UTexture;
class	UTexture2D;

class	UPopcornFXEffect;
class	UPopcornFXTextureAtlas;

#define EXEC_X_POPCORNFX_LEGACY_MATERIAL_TYPE()								\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_Additive)				\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_AlphaBlend)				\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_AlphaBlend_Lit)			\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_AlphaBlendAdditive)		\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_AlphaBlendAdditive_Lit)	\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_Distortion)				\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_Solid)					\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_Solid_Lit)				\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_Masked)					\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_Masked_Lit)				\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_Dithered)				\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_Dithered_Lit)			\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_VolumetricFog)			\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_SixWayLightmap)			\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Additive)						\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_AlphaBlend)					\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Distortion)					\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Solid)						\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Solid_Lit)					\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Masked)						\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Masked_Lit)					\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Dithered)						\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Dithered_Lit)					\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Opaque_Fluid)				\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Masked_Fluid)				\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Opaque_Fluid_Lit)			\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Masked_Fluid_Lit)			\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Opaque_Soft)				\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Masked_Soft)				\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Opaque_Soft_Lit)			\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Masked_Soft_Lit)			\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Opaque_Rigid)				\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Masked_Rigid)				\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Opaque_Rigid_Lit)			\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Masked_Rigid_Lit)			\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Decal)								\

#define EXEC_X_POPCORNFX_MATERIAL_TYPE()									\
		X_POPCORNFX_MATERIAL_TYPE(Billboard_Additive)						\
		X_POPCORNFX_MATERIAL_TYPE(Billboard_AlphaBlend)						\
		X_POPCORNFX_MATERIAL_TYPE(Billboard_AlphaBlend_Lit)					\
		X_POPCORNFX_MATERIAL_TYPE(Billboard_AlphaBlendAdditive)				\
		X_POPCORNFX_MATERIAL_TYPE(Billboard_AlphaBlendAdditive_Lit)			\
		X_POPCORNFX_MATERIAL_TYPE(Billboard_Distortion)						\
		X_POPCORNFX_MATERIAL_TYPE(Billboard_Solid)							\
		X_POPCORNFX_MATERIAL_TYPE(Billboard_Solid_Lit)						\
		X_POPCORNFX_MATERIAL_TYPE(Billboard_Masked)							\
		X_POPCORNFX_MATERIAL_TYPE(Billboard_Masked_Lit)						\
		X_POPCORNFX_MATERIAL_TYPE(Billboard_Dithered)						\
		X_POPCORNFX_MATERIAL_TYPE(Billboard_Dithered_Lit)					\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_Additive)							\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_AlphaBlend)							\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_Solid)								\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_Solid_Lit)							\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_Masked)								\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_Masked_Lit)							\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_Dithered)							\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_Dithered_Lit)						\
		X_POPCORNFX_MATERIAL_TYPE(Decal)									\


UENUM()
enum class	EPopcornFXLegacyMaterialType
{
	Billboard_Additive,
	Billboard_AlphaBlend,
	Billboard_AlphaBlend_Lit,
	Billboard_AlphaBlendAdditive,
	Billboard_AlphaBlendAdditive_Lit,
	Billboard_Distortion,
	Billboard_Solid,
	Billboard_Solid_Lit,
	Billboard_Masked,
	Billboard_Masked_Lit,
	Billboard_Dithered,
	Billboard_Dithered_Lit,

	Billboard_VolumetricFog,
	Billboard_SixWayLightmap,

	Mesh_Additive,
	Mesh_AlphaBlend,
	Mesh_Distortion,
	Mesh_Solid,
	Mesh_Solid_Lit,
	Mesh_Masked,
	Mesh_Masked_Lit,
	Mesh_Dithered,
	Mesh_Dithered_Lit,
	Mesh_VAT_Opaque_Fluid,
	Mesh_VAT_Opaque_Fluid_Lit,
	Mesh_VAT_Masked_Fluid,
	Mesh_VAT_Masked_Fluid_Lit,
	Mesh_VAT_Opaque_Soft,
	Mesh_VAT_Opaque_Soft_Lit,
	Mesh_VAT_Masked_Soft,
	Mesh_VAT_Masked_Soft_Lit,
	Mesh_VAT_Opaque_Rigid,
	Mesh_VAT_Opaque_Rigid_Lit,
	Mesh_VAT_Masked_Rigid,
	Mesh_VAT_Masked_Rigid_Lit,

	Decal,

	// Put them in the end so that the material remap can work..
	Billboard_Additive_NoAlpha,
	Mesh_Additive_NoAlpha,
	__Max UMETA(Hidden)
};

UENUM()
enum class	EPopcornFXDefaultMaterialType
{
	Billboard_Additive,
	Billboard_AlphaBlend,
	Billboard_AlphaBlend_Lit,
	Billboard_AlphaBlendAdditive,
	Billboard_AlphaBlendAdditive_Lit,
	Billboard_Distortion,
	Billboard_Solid,
	Billboard_Solid_Lit,
	Billboard_Masked,
	Billboard_Masked_Lit,
	Billboard_Dithered,
	Billboard_Dithered_Lit,

	Mesh_Additive,
	Mesh_AlphaBlend,
	Mesh_Solid,
	Mesh_Solid_Lit,
	Mesh_Masked,
	Mesh_Masked_Lit,
	Mesh_Dithered,
	Mesh_Dithered_Lit,

	Decal,

	__Max UMETA(Hidden)
};

USTRUCT()
struct FPopcornFXRendererDesc
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString	Name;
	UPROPERTY()
	FString	ParentLayerName;
	UPROPERTY()
	uint32	ParentLayerID;
	UPROPERTY()
	int		Type;

	FPopcornFXRendererDesc()
		: Name(), ParentLayerName(), ParentLayerID(), Type()
	{
	}

	FPopcornFXRendererDesc(const FString &Name, const FString &ParentLayerName, const uint32 ParentLayerID, const int &Type)
		: Name(Name), ParentLayerName(ParentLayerName), ParentLayerID(ParentLayerID), Type(Type)
	{
	}

	bool operator == (const FPopcornFXRendererDesc &other) const
	{
		return Name == other.Name && ParentLayerName == other.ParentLayerName && ParentLayerID == other.ParentLayerID && Type == other.Type;
	}

	bool operator != (const FPopcornFXRendererDesc &other) const
	{
		return Name != other.Name || ParentLayerName != other.ParentLayerName || ParentLayerID != other.ParentLayerID || Type != other.Type;
	}

	FPopcornFXRendererDesc &operator = (const FPopcornFXRendererDesc &other)
	{
		Name = other.Name;
		ParentLayerName = other.ParentLayerName;
		ParentLayerID = other.ParentLayerID;
		Type = other.Type;
		return *this;
	}
};

extern const EPopcornFXLegacyMaterialType		kLegacyTransparentBillboard_Material_ToUE[4];
extern const EPopcornFXLegacyMaterialType		kOpaqueBillboard_LegacyMaterial_ToUE[3];
extern const EPopcornFXDefaultMaterialType		kOpaqueBillboard_DefaultMaterial_ToUE[3];
extern const EPopcornFXLegacyMaterialType		kOpaqueMesh_LegacyMaterial_ToUE[3];
extern const EPopcornFXDefaultMaterialType		kOpaqueMesh_DefaultMaterial_ToUE[3];

// Every UE asset used by a PopcornFX material, I.E. parameters that the user can edit in the effect window
USTRUCT()
struct FPopcornFXEditableMaterialProperties
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere, meta=(DisplayThumbnail="true"))
	UMaterialInterface*				Material;

	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*TextureDiffuse;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*TextureDiffuseRamp;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*TextureEmissive;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*TextureEmissiveRamp;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*TextureNormal;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*TextureRoughMetal;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*TextureSpecular;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*TextureAlphaRemapper;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*TextureMotionVectors;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*TextureSixWay_RLTS;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*TextureSixWay_BBF;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*VATTexturePosition;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*VATTextureNormal;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*VATTextureColor;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*VATTextureRotation;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UTexture2D						*TextureSkeletalAnimation;

	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UPopcornFXTextureAtlas			*TextureAtlas;

	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	USkeletalMesh					*SkeletalMesh;
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere)
	UStaticMesh						*StaticMesh;

	FPopcornFXEditableMaterialProperties()
:	Material()
,	TextureDiffuse(nullptr)
,	TextureDiffuseRamp(nullptr)
,	TextureEmissive(nullptr)
,	TextureEmissiveRamp(nullptr)
,	TextureNormal(nullptr)
,	TextureRoughMetal(nullptr)
,	TextureSpecular(nullptr)
,	TextureAlphaRemapper(nullptr)
,	TextureMotionVectors(nullptr)
,	TextureSixWay_RLTS(nullptr)
,	TextureSixWay_BBF(nullptr)
,	VATTexturePosition(nullptr)
,	VATTextureNormal(nullptr)
,	VATTextureColor(nullptr)
,	VATTextureRotation(nullptr)
,	TextureSkeletalAnimation(nullptr)
,	TextureAtlas(nullptr)
,	SkeletalMesh(nullptr)
,	StaticMesh(nullptr)
	{
	}
};

// Set of features that the effect using this will have
USTRUCT(BlueprintType)
struct FPopcornFXSubRendererMaterial
{
public:
	GENERATED_USTRUCT_BODY()

	// Actual properties used by the effect
	UPROPERTY(Category = "PopcornFX RendererMaterial", EditAnywhere)
	FPopcornFXEditableMaterialProperties	EditableProperties;

	// Default properties used by the effect, I.E. what was exported from Popcorn
	UPROPERTY()
	FPopcornFXEditableMaterialProperties	EditablePropertiesDefault;

	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	EPopcornFXLegacyMaterialType	LegacyMaterialType;

	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	EPopcornFXDefaultMaterialType	DefaultMaterialType;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							NoAlpha : 1;

	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							MeshAtlas : 1;

	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							Raytraced : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							SoftAnimBlending : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							MotionVectorsBlending : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							CastShadow : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							Lit : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							SortIndices : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							CorrectDeformation : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							Roughness;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							Metalness;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							SoftnessDistance;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							SphereMaskHardness;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							AtlasSubDivX;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							AtlasSubDivY;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							MVDistortionStrengthColumns;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							MVDistortionStrengthRows;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							VATNumFrames;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							VATPackedData : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							VATPivotBoundsMin;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							VATPivotBoundsMax;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							VATNormalizedData : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							VATPositionBoundsMin;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							VATPositionBoundsMax;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							SkeletalAnimationCount;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	FVector							SkeletalAnimationPosBoundsMin;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	FVector							SkeletalAnimationPosBoundsMax;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							SkeletalAnimationLinearInterpolate : 1;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							SkeletalAnimationLinearInterpolateTracks : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float							MaskThreshold;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	int32							DrawOrder;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							DynamicParameterMask;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							StaticMeshLOD;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							PerParticleLOD : 1;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							MotionBlur : 1;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32							IsLegacy : 1;

	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureDiffuse = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureDiffuseRamp = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureEmissive = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureEmissiveRamp = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureNormal = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureRoughMetal = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureSpecular = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureAlphaRemapper = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureMotionVectors = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureSixWay_RLTS = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureSixWay_BBF = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasVATTexturePosition = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasVATTextureNormal = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasVATTextureColor = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasVATTextureRotation = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureSkeletalAnimation = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasStaticMesh = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasSkeletalMesh = false;
	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	bool							bHasTextureAtlas = false;
	
	UPROPERTY()
	TArray<uint32>					m_SkeletalMeshBoneIndicesReorder;

	UPROPERTY()
	int32							m_RMId;

	/* Renderer that uses this material */
	UPROPERTY(VisibleAnywhere)
	FPopcornFXRendererDesc			Renderer;

	UPROPERTY(Instanced)
	UMaterialInstanceConstant		*MaterialInstance; // what is this

	FPopcornFXSubRendererMaterial();
	~FPopcornFXSubRendererMaterial();

#if WITH_EDITOR
	bool				_ResetDefaultMaterial_NoReload();
	UMaterialInterface	*FindDefaultMaterial() const;
	UMaterialInterface	*FindLegacyMaterial() const;
	bool				BuildSkelMeshBoneIndicesReorder();
	void				BuildDynamicParameterMask(const PopcornFX::CRendererDataBase *renderer, const PopcornFX::SRendererDeclaration &decl);
	void				RenameDependenciesIFN(UObject* oldAsset, UObject* newAsset);
#endif

	bool				CanBeMergedWith(const FPopcornFXSubRendererMaterial &other) const;

	inline bool			RenderThread_SameMaterial_Billboard(const FPopcornFXSubRendererMaterial &other) const
	{
		return
			EditableProperties.Material == other.EditableProperties.Material &&
			EditableProperties.TextureDiffuse == other.EditableProperties.TextureDiffuse &&
			EditableProperties.TextureDiffuseRamp == other.EditableProperties.TextureDiffuseRamp &&
			EditableProperties.TextureEmissive == other.EditableProperties.TextureEmissive &&
			EditableProperties.TextureNormal == other.EditableProperties.TextureNormal &&
			EditableProperties.TextureSpecular == other.EditableProperties.TextureSpecular &&
			EditableProperties.TextureAlphaRemapper == other.EditableProperties.TextureAlphaRemapper &&
			EditableProperties.TextureAtlas == other.EditableProperties.TextureAtlas &&
			EditableProperties.TextureMotionVectors == other.EditableProperties.TextureMotionVectors &&
			EditableProperties.TextureSixWay_RLTS == other.EditableProperties.TextureSixWay_RLTS &&
			EditableProperties.TextureSixWay_BBF == other.EditableProperties.TextureSixWay_BBF &&
			NoAlpha == other.NoAlpha &&
			MeshAtlas == other.MeshAtlas &&
			Raytraced == other.Raytraced &&
			SoftAnimBlending == other.SoftAnimBlending &&
			SphereMaskHardness == other.SphereMaskHardness &&
			MVDistortionStrengthColumns == other.MVDistortionStrengthColumns &&
			MVDistortionStrengthRows == other.MVDistortionStrengthRows &&
			SoftnessDistance == other.SoftnessDistance &&
			DrawOrder == other.DrawOrder &&
			DynamicParameterMask == other.DynamicParameterMask &&
			CastShadow == other.CastShadow &&
			Lit == other.Lit &&
			SortIndices == other.SortIndices &&
			IsLegacy == other.IsLegacy;
	}

	inline bool			RenderThread_SameMaterial_Mesh(const FPopcornFXSubRendererMaterial &other) const
	{
		// caller should have check that beforehand
		ensure(EditableProperties.StaticMesh == other.EditableProperties.StaticMesh);
		ensure(EditableProperties.SkeletalMesh == other.EditableProperties.SkeletalMesh);
		ensure(StaticMeshLOD == other.StaticMeshLOD);
		ensure(PerParticleLOD == other.PerParticleLOD);
		ensure(MotionBlur == other.MotionBlur);
		ensure(m_SkeletalMeshBoneIndicesReorder.Num() == other.m_SkeletalMeshBoneIndicesReorder.Num());
		return
			EditableProperties.Material == other.EditableProperties.Material && // Mandatory
			EditableProperties.TextureDiffuse == other.EditableProperties.TextureDiffuse &&
			EditableProperties.TextureDiffuseRamp == other.EditableProperties.TextureDiffuseRamp &&
			EditableProperties.TextureEmissive == other.EditableProperties.TextureEmissive &&
			EditableProperties.TextureNormal == other.EditableProperties.TextureNormal &&
			EditableProperties.TextureSpecular == other.EditableProperties.TextureSpecular &&
			EditableProperties.TextureAlphaRemapper == other.EditableProperties.TextureAlphaRemapper &&
			EditableProperties.TextureAtlas == other.EditableProperties.TextureAtlas && // not a parameter, yet ?
			EditableProperties.TextureMotionVectors == other.EditableProperties.TextureMotionVectors &&
			EditableProperties.VATTexturePosition == other.EditableProperties.VATTexturePosition &&
			EditableProperties.VATTextureNormal == other.EditableProperties.VATTextureNormal &&
			EditableProperties.VATTextureColor == other.EditableProperties.VATTextureColor &&
			EditableProperties.VATTextureRotation == other.EditableProperties.VATTextureRotation &&
			VATNumFrames == other.VATNumFrames &&
			VATPackedData == other.VATPackedData &&
			VATPivotBoundsMin == other.VATPivotBoundsMin &&
			VATPivotBoundsMax == other.VATPivotBoundsMax &&
			VATNormalizedData == other.VATNormalizedData &&
			VATPositionBoundsMin == other.VATPositionBoundsMin &&
			VATPositionBoundsMax == other.VATPositionBoundsMax &&
			EditableProperties.TextureSkeletalAnimation == other.EditableProperties.TextureSkeletalAnimation &&
			SkeletalAnimationCount == other.SkeletalAnimationCount &&
			SkeletalAnimationPosBoundsMin == other.SkeletalAnimationPosBoundsMin &&
			SkeletalAnimationPosBoundsMax == other.SkeletalAnimationPosBoundsMax &&
			SkeletalAnimationLinearInterpolate == other.SkeletalAnimationLinearInterpolate &&
			SkeletalAnimationLinearInterpolateTracks == other.SkeletalAnimationLinearInterpolateTracks &&
			NoAlpha == other.NoAlpha &&
			MeshAtlas == other.MeshAtlas &&
			Raytraced == other.Raytraced &&
			SoftAnimBlending == other.SoftAnimBlending &&
			SoftnessDistance == other.SoftnessDistance &&
			SphereMaskHardness == other.SphereMaskHardness &&
			MVDistortionStrengthColumns == other.MVDistortionStrengthColumns &&
			MVDistortionStrengthRows == other.MVDistortionStrengthRows &&
			DrawOrder == other.DrawOrder &&
			DynamicParameterMask == other.DynamicParameterMask &&
			CastShadow == other.CastShadow &&
			Lit == other.Lit &&
			SortIndices == other.SortIndices &&
			IsLegacy == other.IsLegacy;
	}

	inline bool			RenderThread_SameMaterial_Decal(const FPopcornFXSubRendererMaterial &other) const
	{
		return
			EditableProperties.Material == other.EditableProperties.Material &&
			EditableProperties.TextureDiffuse == other.EditableProperties.TextureDiffuse &&
			EditableProperties.TextureDiffuseRamp == other.EditableProperties.TextureDiffuseRamp &&
			EditableProperties.TextureEmissive == other.EditableProperties.TextureEmissive &&
			EditableProperties.TextureEmissiveRamp == other.EditableProperties.TextureEmissiveRamp &&
			EditableProperties.TextureAlphaRemapper == other.EditableProperties.TextureAlphaRemapper &&
			EditableProperties.TextureAtlas == other.EditableProperties.TextureAtlas &&
			EditableProperties.TextureMotionVectors == other.EditableProperties.TextureMotionVectors &&
			AtlasSubDivX == other.AtlasSubDivX &&
			AtlasSubDivY == other.AtlasSubDivY &&
			MVDistortionStrengthColumns == other.MVDistortionStrengthColumns &&
			MVDistortionStrengthRows == other.MVDistortionStrengthRows &&
			Raytraced == other.Raytraced &&
			DynamicParameterMask == other.DynamicParameterMask &&
			SortIndices == other.SortIndices &&
			IsLegacy == other.IsLegacy;
	}

	inline bool NeedsSorting()
	{
		if (IsLegacy)
		{
			if ((LegacyMaterialType >= EPopcornFXLegacyMaterialType::Billboard_AlphaBlend && LegacyMaterialType <= EPopcornFXLegacyMaterialType::Billboard_AlphaBlendAdditive_Lit) ||
 				LegacyMaterialType == EPopcornFXLegacyMaterialType::Billboard_SixWayLightmap)
				return true;
		}
		else
		{
			if (DefaultMaterialType >= EPopcornFXDefaultMaterialType::Billboard_AlphaBlend && DefaultMaterialType <= EPopcornFXDefaultMaterialType::Billboard_AlphaBlendAdditive_Lit)
				return true;
		}
		return false;
	}

	bool operator == (const FPopcornFXSubRendererMaterial& other) const
	{
		if (IsLegacy)
		{
			if (LegacyMaterialType != other.LegacyMaterialType)
			{
				return false;
			}
			if (LegacyMaterialType == EPopcornFXLegacyMaterialType::Decal)
			{
				return RenderThread_SameMaterial_Decal(other);
			}
			else if (LegacyMaterialType >= EPopcornFXLegacyMaterialType::Mesh_Additive && LegacyMaterialType <= EPopcornFXLegacyMaterialType::Mesh_VAT_Masked_Rigid_Lit)
			{
				return RenderThread_SameMaterial_Mesh(other);
			}
			else
			{
				return RenderThread_SameMaterial_Billboard(other);
			}
		}
		else
		{
			if (DefaultMaterialType != other.DefaultMaterialType)
			{
				return false;
			}
			if (DefaultMaterialType == EPopcornFXDefaultMaterialType::Decal)
			{
				return RenderThread_SameMaterial_Decal(other);
			}
			else if (DefaultMaterialType >= EPopcornFXDefaultMaterialType::Mesh_Additive && DefaultMaterialType <= EPopcornFXDefaultMaterialType::Mesh_Dithered_Lit)
			{
				return RenderThread_SameMaterial_Mesh(other);
			}
			else
			{
				return RenderThread_SameMaterial_Billboard(other);
			}
		}
	}
};

/** Save the Materials needed to Render the PopcornFX Particles of a PopcornFXEffect.
* A PopcornFXRendererMaterial instance match a specific set of PopcornFX CParticleNodeRenderer*
* batched together for Rendering.
*/
UCLASS(MinimalAPI)
class UPopcornFXRendererMaterial : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY()
	TWeakObjectPtr<UPopcornFXEffect>			ParentEffect;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	TArray<uint32>								RendererIndices;

	// Actual material and feature setup. There can be multiple ones for distortion effects etc
	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere, EditFixedSize, NoClear)
	TArray<FPopcornFXSubRendererMaterial>		SubMaterials;

	// SRendererDeclaration::m_RendererUID cache
	UPROPERTY()
	uint32										UID;

	virtual void		BeginDestroy() override;
	virtual bool		IsReadyForFinishDestroy() override;

	virtual void		Serialize(FArchive& Ar) override;
	virtual void		PostLoad() override;

#if WITH_EDITOR
	// overrides UObject
	virtual void		PreEditChange(FProperty *propertyThatWillChange) override;
	virtual void		PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;
	virtual void		PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void		PostEditUndo();
#endif // WITH_EDITOR

	virtual void		PreSave(FObjectPreSaveContext SaveContext) override;

#if WITH_EDITOR
	void				TriggerParticleRenderersModification();
	bool				Setup(UPopcornFXEffect *parentEffect, const void *renderer, uint32 rIndex, const FPopcornFXRendererDesc &rendererData);
	void				SetupIndex(uint32 rIndex);
	bool				Add(UPopcornFXEffect *parentEffect, uint32 rIndex);
	void				ReloadInstances();
	void				RenameDependenciesIFN(UObject *oldAsset, UObject *newAsset);
#endif // WITH_EDITOR

	bool				Contains(uint32 rIndex);

	bool				CanBeMergedWith(const UPopcornFXRendererMaterial *other) const;

	FPopcornFXSubRendererMaterial		*GetSubMaterial(uint32 index) { if (index >= uint32(SubMaterials.Num())) return nullptr; return &(SubMaterials[index]); }
	const FPopcornFXSubRendererMaterial	*GetSubMaterial(uint32 index) const { if (index >= uint32(SubMaterials.Num())) return nullptr; return &(SubMaterials[index]); }
	uint32								SubMaterialCount() const { return SubMaterials.Num(); }

	UMaterialInstanceConstant			*GetInstance(uint32 subMatId, bool forRenderThread) const;

private:
#if WITH_EDITOR
	bool				_ReloadInstance(uint32 subMatId);
	bool				_Setup(UPopcornFXEffect *parentEffect, const void *rendererBase, uint32 rIndex, const FPopcornFXRendererDesc &rendererData);
	void				_AddRendererIndex(uint32 rIndex);
#endif

	FRenderCommandFence		m_DetachFence;
};
