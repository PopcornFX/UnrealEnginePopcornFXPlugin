//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "PopcornFXAttributeSamplerShape.h"

#include "PopcornFXAttributeSamplerSkinnedMesh.generated.h"

FWD_PK_API_BEGIN
struct SSkinContext;
class CParticleSamplerDescriptor;
class CShapeDescriptor_Mesh;
FWD_PK_API_END

struct FAttributeSamplerSkinnedMeshData;

UENUM()
namespace	EPopcornFXSkinnedTransforms
{
	enum	Type
	{
		/** Use Skinned Mesh Actor local transforms relative to it's parent actor */
		SkinnedComponentRelativeTr,
		/** Use Skinned Mesh Actor world transforms */
		SkinnedComponentWorldTr,
		/** Use Attribute Sampler Actor local transforms relative to it's parent actor */
		AttrSamplerRelativeTr,
		/** Use Attribute Sampler Actor world transforms */
		AttrSamplerWorldTr,
	};
}

/** Can override an Attribute Sampler **Shape** in a **USkeletalMesh**. */
UCLASS(EditInlineNew, meta=(BlueprintSpawnableComponent), ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXAttributeSamplerSkinnedMesh : public UPopcornFXAttributeSampler
{
	GENERATED_UCLASS_BODY()

public:
	/**
		To manually call if the mesh is changed at runtime after a call to SetTargetActor or SetSkinnedMeshComponentName.
		Only needed if the target mesh is modified dynamically.
	*/
	UFUNCTION(Category="PopcornFX AttributeSampler", BlueprintCallable)
	bool					Rebuild();

	/** Specifies which actors contains the target SkinnedMeshComponent */
	UPROPERTY(Category="PopcornFX AttributeSampler", BlueprintReadWrite, EditAnywhere)
	class AActor			*TargetActor;

	/**
		Use this property to specify the target skinned component name:
		- If TargetActor is specified, looks for a skinned mesh component that has this name in TargetActor
		- If TargetActor is specified, but no skinned mesh component has this name, fallbacks to TargetActor's RootComponent
		- If TargetActor isn't specified, looks for a skinned mesh component that has this name in this actor
		- If TargetActor isn't specified, but no skinned mesh component has this name, fallbacks to this actor's RootComponent
	*/
	UPROPERTY(Category="PopcornFX AttributeSampler", BlueprintReadWrite, EditAnywhere)
	FName					SkinnedMeshComponentName;

	/** Sampling mode to use for this AttributeSampler (Fast, Uniform, Weighted) */
	UPROPERTY(Category="PopcornFX AttributeSampler", BlueprintReadWrite, EditAnywhere)
	TEnumAsByte<EPopcornFXMeshSamplingMode::Type>	MeshSamplingMode;

	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	TEnumAsByte<EPopcornFXColorChannel::Type>		DensityColorChannel;

	/** Enable this if you want to pause the CPU Skinning */
	UPROPERTY(Category="PopcornFX AttributeSampler", BlueprintReadWrite, EditAnywhere)
	uint32					bPauseSkinning : 1;

	/** Enable this if you want to access this skinned mesh's Positions */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	uint32					bSkinPositions : 1;

	/** Enable this if you want to access this skinned mesh's Normals */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	uint32					bSkinNormals : 1;

	/** Enable this if you want to access this skinned mesh's Tangents */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	uint32					bSkinTangents : 1;

	/** Enable this if you want to access this skinned mesh's Colors */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	uint32					bBuildColors : 1;

	/** Enable this if you want to access this skinned mesh's UVs */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	uint32					bBuildUVs : 1;

	/** Enable this if you want to access this skinned mesh's Velocities */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	uint32					bComputeVelocities : 1;

	/** Enable this if you want to use the simulated cloth positions/normals (tangents unavailable). */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	uint32					bBuildClothData : 1;

	/** Determines what transforms will be used for this attribute sampler */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	TEnumAsByte<EPopcornFXSkinnedTransforms::Type>	Transforms;

	/** Enable this if you want to use scaled transforms */
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	uint32					bApplyScale : 1;

	UPROPERTY()
	uint32					bUseRelativeTransform_DEPRECATED : 1;

	UPROPERTY()
	uint32					bUseSkinnedMeshTransforms_DEPRECATED : 1;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Category="PopcornFX AttributeSampler", EditAnywhere)
	uint32					bEditorBuildInitialPose : 1;
#endif // WITH_EDITORONLY_DATA

	// overrides
	virtual void			BeginDestroy() override;
	virtual void			Serialize(FArchive& Ar) override;

#if WITH_EDITOR
	virtual void			PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR
	virtual void			TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction *thisTickFunction) override;

	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) override;
	virtual void									_AttribSampler_PreUpdate(CParticleScene *scene, float deltaTime, enum ELevelTick tickType) override;

private:
	USkinnedMeshComponent	*ResolveSkinnedMeshComponent();

	bool					SetComponentTickingGroup(USkinnedMeshComponent *skinnedMesh);
	bool					BuildInitialPose();
	bool					UpdateSkinning();
	void					UpdateTransforms();
	void					FetchClothData(uint32 vertexStart, uint32 vertexCount);
	void					Clear();

private:
	// (cannot use u32 here, use uint32)
	void					Skin_PreProcess(uint32 vertexStart, uint32 vertexCount, const PopcornFX::SSkinContext &ctx);
	void					Skin_PostProcess(uint32 vertexStart, uint32 vertexCount, const PopcornFX::SSkinContext &ctx);
	void					Skin_Finish(const PopcornFX::SSkinContext &ctx);

private:
	FAttributeSamplerSkinnedMeshData	*m_Data;

	uint64		m_LastFrameUpdate;
	FMatrix44f	m_WorldTr_Current;
	FMatrix44f	m_WorldTr_Previous;
	FVector3f	m_Angular_Velocity;
	FVector3f	m_Linear_Velocity;
};
