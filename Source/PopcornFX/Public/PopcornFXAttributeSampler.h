//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "Components/SceneComponent.h"

#include "PopcornFXAttributeSampler.generated.h"

FWD_PK_API_BEGIN
class	CParticleSamplerDescriptor;
class	CResourceDescriptor;
class	CMeshSurfaceSamplerStructuresRandom;
class	CMeshVolumeSamplerStructuresRandom;
FWD_PK_API_END
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

class	CParticleScene;
struct	FPopcornFXSamplerDesc;

UENUM()
namespace EPopcornFXAttributeSamplerComponentType
{
	enum	Type
	{
		Shape,
		SkinnedMesh,
		Image,
		Curve,
		AnimTrack,
		Turbulence,
		Text,
	};
}

UENUM()
namespace EPopcornFXAttributeSamplerType
{
	enum	Type
	{
		None = 0,
		Shape,
		Image,
		Curve,
		AnimTrack,
		Turbulence,
		Text,
	};
}
enum { EPopcornFXAttributeSamplerType_Max = EPopcornFXAttributeSamplerType::Text + 1 };

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
		Mesh,
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
		Collection,
#endif
	};
}
enum { EPopcornFXAttribSamplerShapeType_Max = EPopcornFXAttribSamplerShapeType::Mesh + 1 };
#if 0 // To re-enable when shape collections are supported by PopcornFX v2
enum { EPopcornFXAttribSamplerShapeType_Max = EPopcornFXAttribSamplerShapeType::Collection + 1 };
#endif

UENUM()
namespace EPopcornFXMeshSamplingMode
{
	enum	Type
	{
		Uniform = 0,
		Fast,
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

UCLASS(Abstract, EditInlineNew)
class POPCORNFX_API UPopcornFXAttributeSampler : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:
	static UClass									*SamplerComponentClass(EPopcornFXAttributeSamplerComponentType::Type type);
	EPopcornFXAttributeSamplerType::Type			SamplerType() const { return m_SamplerType; }

	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) { return nullptr; }
	virtual void									_AttribSampler_PreUpdate(float deltaTime) { return; }

#if WITH_EDITOR
	virtual void									_AttribSampler_IndirectSelectedThisTick() {}
#endif

protected:
	EPopcornFXAttributeSamplerType::Type			m_SamplerType;
};
