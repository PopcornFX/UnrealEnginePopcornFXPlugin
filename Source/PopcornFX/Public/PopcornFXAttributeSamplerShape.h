//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "PopcornFXAttributeSampler.h"

#include "PopcornFXAttributeSamplerShape.generated.h"

struct FAttributeSamplerShapeData;

FWD_PK_API_BEGIN
struct  SSkinContext;
class	CShapeDescriptor;
class	CParticleNodeSamplerData_Shape;
class	CParticleSamplerDescriptor;
class	CShapeDescriptor_Mesh;
FWD_PK_API_END
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

UENUM()
namespace EPopcornFXShapeCollectionSamplingHeuristic
{
	// pk_geometrics/include/ge_shapes.h
	enum	Type
	{
		NoWeight = 0,
		WeightWithVolume,
		WeightWithSurface,
	};
}

UENUM()
namespace	EPopcornFXSkinnedTransforms
{
	enum	Type
	{
		/** Use Skinned Mesh Actor local transform relative to its parent actor */
		SkinnedComponentRelativeTr,
		/** Use Skinned Mesh Actor world transform */
		SkinnedComponentWorldTr,
		/** Use Attribute Sampler Actor local transform relative to its parent actor */
		AttrSamplerRelativeTr,
		/** Use Attribute Sampler Actor world transform */
		AttrSamplerWorldTr,
	};
}

USTRUCT(BlueprintType)
struct POPCORNFX_API FPopcornFXAttributeSamplerPropertiesShape : public FPopcornFXAttributeSamplerProperties
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TEnumAsByte<EPopcornFXAttribSamplerShapeType::Type>		ShapeType;

	/** Weights sampling distribution when is a sub-Shape of a Shape Collection
	* (if CollectionUseShapeWeights is enabled in the Shape Collection).
	*/
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadOnly)
	float					Weight;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
	FVector					BoxDimension;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float					Radius;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float					InnerRadius;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float					Height;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
	FVector					Scale;

#if 0 // To re-enable when shape collections are supported by PopcornFX v2
	/** Distribute sampling by the given CollectionSamplingHeuristic of sub-Shapes */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TEnumAsByte<EPopcornFXShapeCollectionSamplingHeuristic::Type>	CollectionSamplingHeuristic;
#endif

	/** Sampling mode to use for this AttributeSampler (Fast, Uniform, Weighted) */
	UPROPERTY(Category = "PopcornFX AttributeSampler", BlueprintReadWrite, EditAnywhere)
	TEnumAsByte<EPopcornFXMeshSamplingMode::Type>					ShapeSamplingMode;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TEnumAsByte<EPopcornFXColorChannel::Type>						DensityColorChannel;

#if 0 // To re-enable when shape collections are supported by PopcornFX v2
	/** Distribute sampling by the sub-Shapes own Weights (in addition to CollectionSamplingHeuristic) */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	uint32					CollectionUseShapeWeights : 1;
#endif

	/** Modifying that property dynamically requires to restart all emitters referencing this attribute sampler */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	class UStaticMesh		*StaticMesh;

	/** Modifying that property dynamically requires to restart all emitters referencing this attribute sampler */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	int32					StaticMeshSubIndex;

	/** Collection sub-Shapes */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TArray<class APopcornFXAttributeSamplerActor *>	Shapes;

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

	/** Enable this if you want to pause the CPU Skinning */
	UPROPERTY(Category = "PopcornFX AttributeSampler", BlueprintReadWrite, EditAnywhere)
	uint32					bPauseSkinning : 1;

	/** Enable this if you want to access this skinned mesh's Positions */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	uint32					bSkinPositions : 1;

	/** Enable this if you want to access this skinned mesh's Normals */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	uint32					bSkinNormals : 1;

	/** Enable this if you want to access this skinned mesh's Tangents */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	uint32					bSkinTangents : 1;

	/** Enable this if you want to access this skinned mesh's Colors */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	uint32					bBuildColors : 1;

	/** Enable this if you want to access this skinned mesh's UVs */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	uint32					bBuildUVs : 1;

	/** Enable this if you want to access this skinned mesh's Velocities */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	uint32					bComputeVelocities : 1;

	/** Enable this if you want to use the simulated cloth positions/normals (tangents unavailable). */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	uint32					bBuildClothData : 1;

	/** Enable this if you want to use scaled transforms */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	uint32					bApplyScale : 1;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	uint32					bEditorBuildInitialPose : 1;
#endif // WITH_EDITORONLY_DATA

	/** Determines what transforms will be used for this attribute sampler */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TEnumAsByte<EPopcornFXSkinnedTransforms::Type>	Transforms;

	/** Relative Transforms will be used if activated.
	* Enable if sampled in SpawnerScript's Eval(), so shape will be sampled locally to the Emitter.
	* Disable if sampled in SpawnerScript's **Post**Eval(), so the shape will be sampled world space.
	*/
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	uint32					bUseRelativeTransform : 1;

	FPopcornFXAttributeSamplerPropertiesShape()
	:	ShapeType(EPopcornFXAttribSamplerShapeType::Sphere)
	,	Weight(1.0f)
	,	BoxDimension(FVector(100.0f))
	,	Radius(100.0f)
	,	InnerRadius(0.0f)
	,	Height(100.0f)
	,	Scale(FVector::OneVector)
	,	ShapeSamplingMode()
	,	DensityColorChannel()
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
	,	CollectionSamplingHeuristic(EPopcornFXShapeCollectionSamplingHeuristic::NoWeight)
	,	CollectionUseShapeWeights(1)
#endif
	,	StaticMesh()
	,	StaticMeshSubIndex()
	,	TargetActor()
	,	SkinnedMeshComponentName()
	,	bPauseSkinning(false)
	,	bSkinPositions(false)
	,	bSkinNormals(false)
#if WITH_EDITORONLY_DATA
	,	bEditorBuildInitialPose(false)
#endif // WITH_EDITORONLY_DATA
	,	Transforms()
	,	bUseRelativeTransform(true)
	{ }
};


/** Can override an Attribute Sampler **Shape** by a **UStaticMesh**. */
UCLASS(EditInlineNew, meta=(BlueprintSpawnableComponent), ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXAttributeSamplerShape : public UPopcornFXAttributeSampler
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(Category="PopcornFX AttributeSampler", BlueprintCallable)
	void					SetRadius(float radius);

	UFUNCTION(Category = "PopcornFX AttributeSampler", BlueprintCallable)
	void					SetWeight(float weight);

	UFUNCTION(Category = "PopcornFX AttributeSampler", BlueprintCallable)
	void					SetBoxDimension(FVector boxDimensions);

	UFUNCTION(Category = "PopcornFX AttributeSampler", BlueprintCallable)
	void					SetInnerRadius(float innerRadius);

	UFUNCTION(Category = "PopcornFX AttributeSampler", BlueprintCallable)
	void					SetHeight(float height);

	UFUNCTION(Category = "PopcornFX AttributeSampler", BlueprintCallable)
	void					SetScale(FVector scale);

	/**
		To manually call if the mesh is changed at runtime after a call to SetTargetActor or SetSkinnedMeshComponentName.
		Only needed if the target mesh is modified dynamically.
	*/
	UFUNCTION(Category = "PopcornFX AttributeSampler", BlueprintCallable)
	bool					Rebuild();

	// overrides
	virtual void			BeginDestroy() override;
#if WITH_EDITOR
	void					PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
	void					TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction *thisTickFunction) override;
#endif // WITH_EDITOR

#if 0 // To re-enable when shape collections are supported by PopcornFX v2
	inline bool						IsCollection() const { return ShapeType == EPopcornFXAttribSamplerShapeType::Collection; }
#endif
	inline bool						IsValid() const;
	bool							InitShape();
	PopcornFX::CShapeDescriptor		*GetShapeDescriptor() const;

#if WITH_EDITOR
	void							RenderShapeIFP(bool isSelected) const;
#endif

	PopcornFX::CMeshSurfaceSamplerStructuresRandom	*SamplerSurface() const;

	const FPopcornFXAttributeSamplerProperties		*GetProperties() const override { return &Properties; }
#if WITH_EDITOR
	virtual void									CopyPropertiesFrom(const UPopcornFXAttributeSampler *other) override;
#endif

private:
	bool											CanUpdateShapeProperties();
	void											UpdateShapeProperties();

	USkinnedMeshComponent							*ResolveSkinnedMeshComponent();

	bool											SetComponentTickingGroup(USkinnedMeshComponent *skinnedMesh);
	bool											BuildInitialPose();
	bool											UpdateSkinning();
	void											UpdateTransforms();
	void											FetchClothData(uint32 vertexStart, uint32 vertexCount);
	void											Clear();

	// (cannot use u32 here, use uint32)
	void											Skin_PreProcess(uint32 vertexStart, uint32 vertexCount, const PopcornFX::SSkinContext &ctx);
	void											Skin_PostProcess(uint32 vertexStart, uint32 vertexCount, const PopcornFX::SSkinContext &ctx);
	void											Skin_Finish(const PopcornFX::SSkinContext &ctx);

	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) override;
	virtual void									_AttribSampler_PreUpdate(float deltaTime) override;

#if WITH_EDITOR
	virtual void									_AttribSampler_IndirectSelectedThisTick() override { m_IndirectSelectedThisTick = true; }
#endif

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler")
	FPopcornFXAttributeSamplerPropertiesShape	Properties;

private:
	FAttributeSamplerShapeData	*m_Data;
	uint64						m_LastFrameUpdate;
	FMatrix44f					m_WorldTr_Current;
	FMatrix44f					m_WorldTr_Previous;
	FVector3f					m_Angular_Velocity;
	FVector3f					m_Linear_Velocity;

#if WITH_EDITOR
	bool						m_IndirectSelectedThisTick;
#endif // WITH_EDITOR
};
