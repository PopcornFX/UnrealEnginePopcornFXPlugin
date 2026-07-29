//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
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
namespace EPopcornFXAttribSamplerShapeType
{
	enum	Type
	{
		Box = 0,
		Sphere,
		Ellipsoid,
		Cylinder,
		Capsule,
		Cone,
		StaticMesh,
		SkeletalMesh,
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
		Collection,
#endif
	};
}
enum { EPopcornFXAttribSamplerShapeType_Max = EPopcornFXAttribSamplerShapeType::SkeletalMesh + 1 };
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
enum { EPopcornFXAttribSamplerShapeType_Max = EPopcornFXAttribSamplerShapeType::Collection + 1 };
#endif

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

/** Sampling location option for skinned meshes */
UENUM()
namespace	EPopcornFXSkinnedTransforms
{
	enum	Type
	{
		/** Use Skinned Mesh Actor local transforms relative to its parent actor */
		SkinnedComponentRelativeTr,
		/** Use Skinned Mesh Actor world transforms */
		SkinnedComponentWorldTr,
		/** Use emitter local transforms relative to its parent actor */
		EmitterRelativeTr,
		/** Use emitter world transforms */
		EmitterSamplerWorldTr,
	};
}

/** Sampling location option for other meshes */
UENUM()
namespace	EPopcornFXShapeTransforms
{
	enum	Type
	{
		/** Use emitter local transforms relative to its parent actor */
		EmitterRelativeTr,
		/** Use emitter Actor world transforms */
		EmitterSamplerWorldTr,
	};
}

UENUM()
namespace EPopcornFXMeshSamplingMode
{
	enum	Type
	{
		Fast = 0,
		Uniform,
		Weighted
	};
}

UENUM()
namespace EPopcornFXColorChannel
{
	enum	Type
	{
		Red = 0,
		Green,
		Blue,
		Alpha
	};
}

USTRUCT(BlueprintType)
struct POPCORNFX_API FPopcornFXAttributeSamplerPropertiesShape : public FPopcornFXAttributeSamplerProperties
{
	GENERATED_USTRUCT_BODY()

public:
	//virtual void			CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other) override;
	/** Checks if properties set by the user are valid. For example, a Curve attribute sampler needs a Curve asset to be valid. */
	virtual bool			ArePropertiesSupported(UPopcornFXEmitterComponent *emitter, const FString &samplerName) override;
	/** Checks if properties set by the user are compatible with the emitter using it. For example, if an effect uses a 2D grid and the user sets a 3D grid, it's not compatible */
	virtual bool			ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const FString &samplerName, const PopcornFX::CResourceDescriptor *defaultSampler) override;

	USkinnedMeshComponent	*ResolveSkinnedMeshComponent(UPopcornFXEmitterComponent *emitter, const FString &samplerName);


	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EPopcornFXAttribSamplerShapeType::Type>		ShapeType;

	/** Weights sampling distribution when is a sub-Shape of a Shape Collection
	* (if CollectionUseShapeWeights is enabled in the Shape Collection).
	*/
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	float					Weight;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0"))
	FVector					BoxDimension;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float					Radius;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float					InnerRadius;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float					Height;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0"))
	FVector					Scale;

	/** Position of the mesh */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	FVector					Position;

	/** Rotation of the mesh */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	FRotator				Rotation;

#if 0 // To re-enable when shape collections are supported by PopcornFX v2
	/** Distribute sampling by the given CollectionSamplingHeuristic of sub-Shapes */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TEnumAsByte<EPopcornFXShapeCollectionSamplingHeuristic::Type>	CollectionSamplingHeuristic;
#endif

	/** Sampling mode to use for this AttributeSampler (Fast, Uniform, Weighted) */
	UPROPERTY(Category = "PopcornFX AttributeSampler", BlueprintReadWrite, EditAnywhere)
	TEnumAsByte<EPopcornFXMeshSamplingMode::Type>					ShapeSamplingMode;

	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
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

#if 0
	/** Collection sub-Shapes */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	TArray<class APopcornFXAttributeSamplerActor *>	Shapes;
#endif

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
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	uint32					bSkinPositions : 1;

	/** Enable this if you want to access this skinned mesh's Normals */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	uint32					bSkinNormals : 1;

	/** Enable this if you want to access this skinned mesh's Tangents */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	uint32					bSkinTangents : 1;

	/** Enable this if you want to access this skinned mesh's Colors */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	uint32					bBuildColors : 1;

	/** Enable this if you want to access this skinned mesh's UVs */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	uint32					bBuildUVs : 1;

	/** Enable this if you want to access this skinned mesh's Velocities */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	uint32					bComputeVelocities : 1;

	/** Enable this if you want to use the simulated cloth positions/normals (tangents unavailable). */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	uint32					bBuildClothData : 1;

	/** Enable this if you want to use scaled transforms */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	uint32					bApplyScale : 1;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere)
	uint32					bEditorBuildInitialPose : 1;
#endif // WITH_EDITORONLY_DATA

	/** Determines what transforms will be used for this attribute sampler */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EPopcornFXSkinnedTransforms::Type>	SkinnedTransforms;

	/** Determines what transforms will be used for this attribute sampler */
	UPROPERTY(Category = "PopcornFX AttributeSampler", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EPopcornFXShapeTransforms::Type>	ShapeTransforms;

#if WITH_EDITOR
	virtual void			SetupDefaults(const PopcornFX::CParticleAttributeSamplerDeclaration *const decl, bool updateUnlockedValues = false) override;
#endif

	FPopcornFXAttributeSamplerPropertiesShape()
	:	FPopcornFXAttributeSamplerProperties(EPopcornFXAttributeSamplerType::Shape)
	,	ShapeType(EPopcornFXAttribSamplerShapeType::Sphere)
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
	,	bSkinPositions(true)
	,	bSkinNormals(false)
	,	bSkinTangents(false)
	,	bBuildColors(false)
	,	bBuildUVs(false)
	,	bComputeVelocities(false)
	,	bBuildClothData(false)
	,	bApplyScale(false)
#if WITH_EDITORONLY_DATA
	,	bEditorBuildInitialPose(false)
#endif // WITH_EDITORONLY_DATA
	,	SkinnedTransforms(EPopcornFXSkinnedTransforms::EmitterSamplerWorldTr)
	,	ShapeTransforms(EPopcornFXShapeTransforms::EmitterSamplerWorldTr)
	{ }
};

/** Can override an Attribute Sampler **Shape** by a **UStaticMesh**. */
USTRUCT(meta=(BlueprintSpawnableComponent))
struct POPCORNFX_API FPopcornFXAttributeSamplerShape : public FPopcornFXAttributeSampler
{
	GENERATED_USTRUCT_BODY()

public:
	void					SetRadius(float radius);

	void					SetWeight(float height);

	void					SetBoxDimension(FVector boxDimensions);

	void					SetInnerRadius(float innerRadius);

	void					SetHeight(float height);

	void					SetScale(FVector scale);

	/**
		To manually call if the mesh is changed at runtime after a call to SetTargetActor or SetSkinnedMeshComponentName.
		Only needed if the target mesh is modified dynamically.
	*/
	bool					Rebuild(UPopcornFXEmitterComponent *emitter);

	FPopcornFXAttributeSamplerShape();

	// overrides
	virtual void			BeginDestroy() override;
	void					TickComponent(UPopcornFXEmitterComponent *emitter, float deltaTime, ELevelTick tickType, FActorComponentTickFunction *thisTickFunction);
#if WITH_EDITOR
	void					PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

#if 0 // To re-enable when shape collections are supported by PopcornFX v2
	inline bool						IsCollection() const { return ShapeType == EPopcornFXAttribSamplerShapeType::Collection; }
#endif
	inline bool						IsValid() const;
	bool							InitShape();
	PopcornFX::CShapeDescriptor		*GetShapeDescriptor() const;

#if WITH_EDITOR
	void							RenderShapeIFP(UPopcornFXEmitterComponent *emitter, bool isSelected) const;
#endif

	PopcornFX::CMeshSurfaceSamplerStructuresRandom	*SamplerSurface() const;
	PopcornFX::CMeshVolumeSamplerStructuresRandom	*SamplerVolume() const;

	// FPopcornFXAttributeSampler overrides
	const FPopcornFXAttributeSamplerProperties		*GetProperties() const override { return &Properties; }
#if WITH_EDITOR
	virtual void									CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other) override;
	virtual void									RefreshFromProperties(const FPopcornFXAttributeSamplerProperties *properties) override;
#endif
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, const FPopcornFXAttributeSamplerProperties *properties, const PopcornFX::CResourceDescriptor *defaultSampler) override;
	virtual void									_AttribSampler_PreUpdate(UPopcornFXEmitterComponent *owner, float deltaTime) override;

#if WITH_EDITOR
	virtual void									_AttribSampler_IndirectSelectedThisTick() override { m_IndirectSelectedThisTick = true; }
#endif
private:
	bool											CanUpdateShapeProperties(EPopcornFXAttribSamplerShapeType::Type newType);
	void											UpdateShapeProperties();

	bool											SetComponentTickingGroup(USkinnedMeshComponent *skinnedMesh);
	bool											BuildInitialPose(UPopcornFXEmitterComponent *emitter);
	bool											UpdateSkinning();
	void											UpdateTransforms(UPopcornFXEmitterComponent *emitter);
	void											FetchClothData(uint32 vertexStart, uint32 vertexCount);
	void											Clear();

	// (cannot use u32 here, use uint32)
	void											Skin_PreProcess(uint32 vertexStart, uint32 vertexCount, const PopcornFX::SSkinContext &ctx);
	void											Skin_PostProcess(uint32 vertexStart, uint32 vertexCount, const PopcornFX::SSkinContext &ctx);
	void											Skin_Finish(const PopcornFX::SSkinContext &ctx);

public:
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

UCLASS(meta = (BlueprintSpawnableComponent))
class POPCORNFX_API UPopcornFXAttributeSamplerShapeAsset : public UPopcornFXAttributeSamplerAsset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FPopcornFXAttributeSamplerPropertiesShape	Properties;

public:
	virtual const FPopcornFXAttributeSamplerProperties	*GetProperties() const override { return &Properties; }
	virtual FPopcornFXAttributeSamplerProperties		*GetProperties() override { return &Properties; }

};