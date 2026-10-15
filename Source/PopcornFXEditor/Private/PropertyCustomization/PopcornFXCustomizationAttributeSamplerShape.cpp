//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#if WITH_EDITOR

#include "PopcornFXCustomizationAttributeSamplerShape.h"

#include "PopcornFXAttributeSamplerShape.h"

#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXCustomizationAttributeSamplerShape, Log, All);

//----------------------------------------------------------------------------

/** Makes a new instance of this detail layout class for a specific detail view requesting it */
TSharedRef<IPropertyTypeCustomization>	FPopcornFXCustomizationAttributeSamplerShape::MakeInstance()
{
	return MakeShareable(new FPopcornFXCustomizationAttributeSamplerShape);
}

//----------------------------------------------------------------------------

void FPopcornFXCustomizationAttributeSamplerShape::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder &ChildBuilder, IPropertyTypeCustomizationUtils &CustomizationUtils)
{
	if (!PropertyHandle->IsValidHandle())
	{
		return;
	}

	FPopcornFXCustomizationAttributeSampler::CustomizeChildren(PropertyHandle, ChildBuilder, CustomizationUtils);

	TSharedPtr<IPropertyHandle>	shapeTypePty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, ShapeType));
	shapeTypePty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSamplerShape::RebuildProperties));
	uint8	shapeType;
	shapeTypePty->GetValue(shapeType);
	AddErrorableProperty(PropertyHandle, "ShapeType", ChildBuilder, true, false);

	TSharedPtr<IPropertyHandle>	shapeSamplingModePty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, ShapeSamplingMode));
	shapeSamplingModePty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSamplerShape::RebuildProperties));
	uint8	shapeSamplingMode;
	shapeSamplingModePty->GetValue(shapeSamplingMode);
	AddErrorableProperty(PropertyHandle, "ShapeSamplingMode", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::StaticMesh || shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "DensityColorChannel", ChildBuilder,
		(shapeType == EPopcornFXAttribSamplerShapeType::StaticMesh || shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh)
		&& shapeSamplingMode == EPopcornFXMeshSamplingMode::Weighted, true);

	AddErrorableProperty(PropertyHandle, "Weight", ChildBuilder,
		shapeType != EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "BoxDimension", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::Box, true);

	AddErrorableProperty(PropertyHandle, "Radius", ChildBuilder,
		shapeType != EPopcornFXAttribSamplerShapeType::Box
		&& shapeType != EPopcornFXAttribSamplerShapeType::StaticMesh
		&& shapeType != EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "InnerRadius", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::Cylinder
			|| shapeType == EPopcornFXAttribSamplerShapeType::Capsule
			|| shapeType == EPopcornFXAttribSamplerShapeType::Ellipsoid
			|| shapeType == EPopcornFXAttribSamplerShapeType::Sphere, true);

	AddErrorableProperty(PropertyHandle, "Height", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::Cylinder || shapeType == EPopcornFXAttribSamplerShapeType::Capsule
			|| shapeType == EPopcornFXAttribSamplerShapeType::Cone, true);

	AddErrorableProperty(PropertyHandle, "Scale", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::StaticMesh, true);

	AddErrorableProperty(PropertyHandle, "StaticMesh", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::StaticMesh, true);

	AddErrorableProperty(PropertyHandle, "StaticMeshSubIndex", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::StaticMesh, true);

#if 0 // To re-enable when shape collections are supported by PopcornFX v2
	TSharedPtr<IPropertyHandle>	shapesPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, Shapes));
	ChildBuilder.AddProperty(shapesPty.ToSharedRef());
#endif

	AddErrorableProperty(PropertyHandle, "bUseRelativeTransform", ChildBuilder,
		shapeType != EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	// Skeletal mesh

	AddErrorableProperty(PropertyHandle, "TargetActor", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "SkinnedMeshComponentName", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "bPauseSkinning", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "bSkinPositions", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "bSkinNormals", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "bSkinTangents", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "bBuildColors", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "bBuildUVs", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "bComputeVelocities", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "bBuildClothData", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "bApplyScale", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "bEditorBuildInitialPose", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);

	AddErrorableProperty(PropertyHandle, "Transforms", ChildBuilder,
		shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh, true);
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
