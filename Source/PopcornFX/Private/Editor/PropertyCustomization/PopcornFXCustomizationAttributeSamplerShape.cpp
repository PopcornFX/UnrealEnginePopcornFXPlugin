//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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
	ChildBuilder.AddProperty(shapeTypePty.ToSharedRef());
	shapeTypePty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSamplerShape::RebuildProperties));
	uint8	shapeType;
	shapeTypePty->GetValue(shapeType);

	TSharedPtr<IPropertyHandle>	shapeSamplingModePty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, ShapeSamplingMode));
	ChildBuilder.AddProperty(shapeSamplingModePty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::StaticMesh || shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
		FOnBooleanValueChanged()).
		EditConditionHides(true);
	shapeSamplingModePty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeSamplerShape::RebuildProperties));
	uint8	shapeSamplingMode;
	shapeSamplingModePty->GetValue(shapeSamplingMode);

	TSharedPtr<IPropertyHandle>	densityColorChannelPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, DensityColorChannel));
	ChildBuilder.AddProperty(densityColorChannelPty.ToSharedRef()).
		EditCondition((shapeType == EPopcornFXAttribSamplerShapeType::StaticMesh || shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh)
			&& shapeSamplingMode == EPopcornFXMeshSamplingMode::Weighted,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	weightPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, Weight));
	ChildBuilder.AddProperty(weightPty.ToSharedRef()).
		EditCondition(shapeType != EPopcornFXAttribSamplerShapeType::SkeletalMesh, FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	boxDimensionPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, BoxDimension));
	ChildBuilder.AddProperty(boxDimensionPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::Box, FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	radiusPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, Radius));
	ChildBuilder.AddProperty(radiusPty.ToSharedRef()).
		EditCondition(shapeType != EPopcornFXAttribSamplerShapeType::Box
			&& shapeType != EPopcornFXAttribSamplerShapeType::StaticMesh
			&& shapeType != EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	innerRadiusPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, InnerRadius));
	ChildBuilder.AddProperty(innerRadiusPty.ToSharedRef()).
		EditCondition((shapeType == EPopcornFXAttribSamplerShapeType::Cylinder
			|| shapeType == EPopcornFXAttribSamplerShapeType::Capsule
			|| shapeType == EPopcornFXAttribSamplerShapeType::Ellipsoid
			|| shapeType == EPopcornFXAttribSamplerShapeType::Sphere),
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	heightPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, Height));
	ChildBuilder.AddProperty(heightPty.ToSharedRef()).
		EditCondition((shapeType == EPopcornFXAttribSamplerShapeType::Cylinder || shapeType == EPopcornFXAttribSamplerShapeType::Capsule
			|| shapeType == EPopcornFXAttribSamplerShapeType::Cone),
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	scalePty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, Scale));
	ChildBuilder.AddProperty(scalePty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::StaticMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	staticMeshPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, StaticMesh));
	ChildBuilder.AddProperty(staticMeshPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::StaticMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	staticMeshSubIndexPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, StaticMeshSubIndex));
	ChildBuilder.AddProperty(staticMeshSubIndexPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::StaticMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

#if 0 // To re-enable when shape collections are supported by PopcornFX v2
	TSharedPtr<IPropertyHandle>	shapesPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, Shapes));
	ChildBuilder.AddProperty(shapesPty.ToSharedRef());
#endif

	TSharedPtr<IPropertyHandle>	useRelativeTransformPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bUseRelativeTransform));
	ChildBuilder.AddProperty(useRelativeTransformPty.ToSharedRef()).
		EditCondition(shapeType != EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	// Skeletal mesh

	TSharedPtr<IPropertyHandle>	skeletalMeshActorPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, TargetActor));
	ChildBuilder.AddProperty(skeletalMeshActorPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	skinnedMeshComponentNamePty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, SkinnedMeshComponentName));
	ChildBuilder.AddProperty(skinnedMeshComponentNamePty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	pauseSkinningPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bPauseSkinning));
	ChildBuilder.AddProperty(pauseSkinningPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	skinPositionsPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bSkinPositions));
	ChildBuilder.AddProperty(skinPositionsPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	skinNormalsPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bSkinNormals));
	ChildBuilder.AddProperty(skinNormalsPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	skinTangentsPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bSkinTangents));
	ChildBuilder.AddProperty(skinTangentsPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	buildColorsPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bBuildColors));
	ChildBuilder.AddProperty(buildColorsPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	bBuildUVsPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bBuildUVs));
	ChildBuilder.AddProperty(bBuildUVsPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	bComputeVelocitiesPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bComputeVelocities));
	ChildBuilder.AddProperty(bComputeVelocitiesPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	bBuildClothDataPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bBuildClothData));
	ChildBuilder.AddProperty(bBuildClothDataPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	bApplyScalePty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bApplyScale));
	ChildBuilder.AddProperty(bApplyScalePty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	bEditorBuildInitialPosePty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, bEditorBuildInitialPose));
	ChildBuilder.AddProperty(bEditorBuildInitialPosePty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);

	TSharedPtr<IPropertyHandle>	transformsPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesShape, Transforms));
	ChildBuilder.AddProperty(transformsPty.ToSharedRef()).
		EditCondition(shapeType == EPopcornFXAttribSamplerShapeType::SkeletalMesh,
			FOnBooleanValueChanged()).
		EditConditionHides(true);
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
