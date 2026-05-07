//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_VolumetricFog)			\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Billboard_SixWayLightmap)			\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Additive)						\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_AlphaBlend)					\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Distortion)					\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Solid)						\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Solid_Lit)					\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Masked)						\
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_Masked_Lit)					\
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
		X_POPCORNFX_LEGACY_MATERIAL_TYPE(Mesh_VAT_Masked_Rigid_Lit)	

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
		X_POPCORNFX_MATERIAL_TYPE(Mesh_Additive)							\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_AlphaBlend)							\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_Solid)								\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_Solid_Lit)							\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_Masked)								\
		X_POPCORNFX_MATERIAL_TYPE(Mesh_Masked_Lit)							\


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

	Billboard_VolumetricFog,
	Billboard_SixWayLightmap,

	Mesh_Additive,
	Mesh_AlphaBlend,
	Mesh_Distortion,
	Mesh_Solid,
	Mesh_Solid_Lit,
	Mesh_Masked,
	Mesh_Masked_Lit,
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

	Mesh_Additive,
	Mesh_AlphaBlend,
	Mesh_Solid,
	Mesh_Solid_Lit,
	Mesh_Masked,
	Mesh_Masked_Lit,

	__Max UMETA(Hidden)
};

extern const EPopcornFXLegacyMaterialType		kLegacyTransparentBillboard_Material_ToUE[4];
extern const EPopcornFXLegacyMaterialType		kOpaqueBillboard_LegacyMaterial_ToUE[2];
extern const EPopcornFXDefaultMaterialType		kOpaqueBillboard_DefaultMaterial_ToUE[2];
extern const EPopcornFXLegacyMaterialType		kOpaqueMesh_LegacyMaterial_ToUE[2];
extern const EPopcornFXDefaultMaterialType		kOpaqueMesh_DefaultMaterial_ToUE[2];

USTRUCT(BlueprintType)
struct FPopcornFXSubRendererMaterial
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail="true"))
	UMaterialInterface*			Material;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	EPopcornFXLegacyMaterialType		LegacyMaterialType;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	EPopcornFXDefaultMaterialType		DefaultMaterialType;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*TextureDiffuse;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*TextureDiffuseRamp;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*TextureEmissive;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*TextureNormal;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*TextureRoughMetal;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*TextureSpecular;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*TextureAlphaRemapper;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*TextureMotionVectors;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*TextureSixWay_RLTS;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*TextureSixWay_BBF;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*VATTexturePosition;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*VATTextureNormal;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*VATTextureColor;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*VATTextureRotation;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UTexture2D					*TextureSkeletalAnimation;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UPopcornFXTextureAtlas		*TextureAtlas;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						NoAlpha : 1;

	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						MeshAtlas : 1;

	UPROPERTY(Category = "PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						Raytraced : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						SoftAnimBlending : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						MotionVectorsBlending : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						CastShadow : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						Lit : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						SortIndices : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						CorrectDeformation : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float						Roughness;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float						Metalness;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float						SoftnessDistance;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float						SphereMaskHardness;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float						MVDistortionStrengthColumns;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float						MVDistortionStrengthRows;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						VATNumFrames;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						VATPackedData : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float						VATPivotBoundsMin;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float						VATPivotBoundsMax;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						VATNormalizedData : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float						VATPositionBoundsMin;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float						VATPositionBoundsMax;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						SkeletalAnimationCount;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	FVector						SkeletalAnimationPosBoundsMin;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	FVector						SkeletalAnimationPosBoundsMax;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						SkeletalAnimationLinearInterpolate : 1;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						SkeletalAnimationLinearInterpolateTracks : 1;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	float						MaskThreshold;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	int32						DrawOrder;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						DynamicParameterMask;

	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	USkeletalMesh				*SkeletalMesh;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	UStaticMesh					*StaticMesh;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						StaticMeshLOD;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						PerParticleLOD : 1;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						MotionBlur : 1;
	UPROPERTY(Category="PopcornFX RendererMaterial", VisibleAnywhere)
	uint32						IsLegacy : 1;
	

	UPROPERTY()
	TArray<uint32>				m_SkeletalMeshBoneIndicesReorder;

	UPROPERTY()
	int32						m_RMId;


	UPROPERTY(Instanced)
	UMaterialInstanceConstant	*MaterialInstance;

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
			Material == other.Material &&
			TextureDiffuse == other.TextureDiffuse &&
			TextureDiffuseRamp == other.TextureDiffuseRamp &&
			TextureEmissive == other.TextureEmissive &&
			TextureNormal == other.TextureNormal &&
			TextureSpecular == other.TextureSpecular &&
			TextureAlphaRemapper == other.TextureAlphaRemapper &&
			TextureAtlas == other.TextureAtlas &&
			TextureMotionVectors == other.TextureMotionVectors &&
			TextureSixWay_RLTS == other.TextureSixWay_RLTS &&
			TextureSixWay_BBF == other.TextureSixWay_BBF &&
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
		ensure(StaticMesh == other.StaticMesh);
		ensure(SkeletalMesh == other.SkeletalMesh);
		ensure(StaticMeshLOD == other.StaticMeshLOD);
		ensure(PerParticleLOD == other.PerParticleLOD);
		ensure(MotionBlur == other.MotionBlur);
		ensure(m_SkeletalMeshBoneIndicesReorder.Num() == other.m_SkeletalMeshBoneIndicesReorder.Num());
		return
			Material == other.Material && // Mandatory
			TextureDiffuse == other.TextureDiffuse &&
			TextureDiffuseRamp == other.TextureDiffuseRamp &&
			TextureEmissive == other.TextureEmissive &&
			TextureNormal == other.TextureNormal &&
			TextureSpecular == other.TextureSpecular &&
			TextureAlphaRemapper == other.TextureAlphaRemapper &&
			TextureAtlas == other.TextureAtlas && // not a parameter, yet ?
			TextureMotionVectors == other.TextureMotionVectors &&
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
			TextureSkeletalAnimation == other.TextureSkeletalAnimation &&
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

	UPROPERTY(Category="PopcornFX RendererMaterial", EditAnywhere, EditFixedSize, NoClear)
	TArray<FPopcornFXSubRendererMaterial>		SubMaterials;

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

#if (ENGINE_MAJOR_VERSION == 5)
	virtual void		PreSave(FObjectPreSaveContext SaveContext) override;
#else
	virtual void		PreSave(const class ITargetPlatform* TargetPlatform) override;
#endif // (ENGINE_MAJOR_VERSION == 5)

#if WITH_EDITOR
	void				TriggerParticleRenderersModification();
	bool				Setup(UPopcornFXEffect *parentEffect, const void *renderer, uint32 rIndex);
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
	bool				_Setup(UPopcornFXEffect *parentEffect, const void *rendererBase, uint32 rIndex);
	void				_AddRendererIndex(uint32 rIndex);
#endif

	FRenderCommandFence		m_DetachFence;
};
