//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplerVectorField.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_samplers_classes.h>
#include <pk_kernel/include/kr_refcounted_buffer.h>
#include <pk_render_helpers/include/frame_collector/rh_frame_data.h>

#include "PopcornFXPlugin.h"
#include "PopcornFXStats.h"
#include "PopcornFXAttributeList.h"
#include "Assets/PopcornFXEffect.h"
#include "Assets/PopcornFXEffectPriv.h"

#include "VectorField/VectorFieldStatic.h"
#include "DrawDebugHelpers.h"

#if WITH_EDITOR
#	include "Editor.h"
#	include "Engine/Selection.h"
#endif

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplerVectorField"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplerVectorField, Log, All);

//----------------------------------------------------------------------------
//
// FAttributeSamplerVectorFieldData
//
//----------------------------------------------------------------------------

struct FAttributeSamplerVectorFieldData
{
	PopcornFX::PParticleSamplerDescriptor_VectorField_Grid	m_Desc; // We only allow static vector field override for attribute sampler

	bool		m_NeedsReload = true;
#if WITH_EDITOR
	FVector3f	m_RealExtentUnscaled = FVector3f::ZeroVector;
#endif // WITH_EDITOR
};

UPopcornFXAttributeSamplerVectorField::UPopcornFXAttributeSamplerVectorField(const FObjectInitializer &PCIP)
	: Super(PCIP)
{
	bAutoActivate = true;
#if WITH_EDITOR
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	m_IndirectSelectedThisTick = false;
#if 0
	bDrawCells = false;
#endif // 0
#endif // WITH_EDITOR

	//	default values
	Properties.Intensity = 1.f;
	Properties.VectorField = null;
	Properties.VolumeDimensions = FVector(100.f, 100.f, 100.f);
	Properties.SamplingMode = EPopcornFXVectorFieldSamplingMode::Trilinear;
	Properties.WrapMode = EPopcornFXVectorFieldWrapMode::Wrap;

	// Guess here, turbulence sampler probably used by physics evolver or script evolver
	Properties.bUseRelativeTransform = false;

	m_TimeAccumulation = 0.f;
	m_SamplerType = EPopcornFXAttributeSamplerType::Turbulence;
	m_Data = new FAttributeSamplerVectorFieldData();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::BeginDestroy()
{
	if (m_Data != null)
	{
		if (m_Data->m_Desc != null)
		{
			m_Data->m_Desc->Clear();
			m_Data->m_Desc = null;
		}
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::_SetBounds()
{
	PK_ASSERT(Properties.VectorField != null);

	if (Properties.BoundsSource == EPopcornFXVectorFieldBounds::Source)
	{
		const PopcornFX::CAABB	bounds = PopcornFX::CAABB::FromMinMax(ToPk(Properties.VectorField->Bounds.Min), ToPk(Properties.VectorField->Bounds.Max));
		m_Data->m_Desc->SetBounds(bounds);
#if WITH_EDITOR
		m_Data->m_RealExtentUnscaled = ToUE(bounds.Extent() * FPopcornFXPlugin::GlobalScale());
#endif // WITH_EDITOR
	}
	else
	{
		PK_ASSERT(Properties.BoundsSource == EPopcornFXVectorFieldBounds::Custom);

		PopcornFX::CAABB	bounds;
		bounds.SetupFromHalfExtent(ToPk(Properties.VolumeDimensions) * 0.5f * FPopcornFXPlugin::GlobalScaleRcp());
		m_Data->m_Desc->SetBounds(bounds);

#if WITH_EDITOR
		m_Data->m_RealExtentUnscaled = ToUE(bounds.Extent() * FPopcornFXPlugin::GlobalScale());
#endif // WITH_EDITOR
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::_BuildVectorFieldFlags(uint32 &flags, uint32 &interpolation) const
{
	if (Properties.WrapMode == EPopcornFXVectorFieldWrapMode::Wrap)
	{
		flags |= PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Wrap_X;
		flags |= PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Wrap_Y;
		flags |= PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Wrap_Z;
	}
	PK_STATIC_ASSERT(EPopcornFXVectorFieldSamplingMode::Point == (u32)PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Interpolation_Point);
	PK_STATIC_ASSERT(EPopcornFXVectorFieldSamplingMode::Trilinear == (u32)PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::Interpolation_Trilinear);

	interpolation = static_cast<PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::EInterpolation>(Properties.SamplingMode.GetValue());
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

void	UPopcornFXAttributeSamplerVectorField::RenderVectorFieldShape(const FMatrix &transforms, const FQuat &rotation, bool isSelected)
{
	if (m_Data->m_RealExtentUnscaled == FVector3f::ZeroVector)
		return;
	const UWorld	*world = GetWorld();
	const FColor	debugColor = isSelected ? kSamplerShapesDebugColorSelected : kSamplerShapesDebugColor;

	const FVector	boxDims = FVector(m_Data->m_RealExtentUnscaled) * GetComponentTransform().GetScale3D() * 0.5f;
	DrawDebugBox(world, transforms.GetOrigin(), boxDims, rotation, debugColor);
#if 0
	if (!bDrawCells)
		return;

	const CFloat3	boxMin = ToPk(VectorField->Bounds.Min * VolumeDimensions);
	const CFloat3	boxMax = ToPk(VectorField->Bounds.Max * VolumeDimensions);
	const CFloat4	vectorFieldSize = CFloat4(VectorField->SizeX, VectorField->SizeY, VectorField->SizeZ, 1);

	const CFloat3	boxCellDim = (boxMax - boxMin) / vectorFieldSize.xyz();

	for (int32 zS = 0; zS <= vectorFieldSize.z(); ++zS)
	{
		for (int32 yS = 0; yS <= vectorFieldSize.y(); ++yS)
		{
			FVector	lineStart = FVector(boxMin.x(), boxMin.y() + boxCellDim.y() * yS, boxMin.z() + boxCellDim.z() * zS);
			FVector	lineEnd = FVector(boxMax.x(), boxMin.y() + boxCellDim.y() * yS, boxMin.z() + boxCellDim.z() * zS);

			lineStart = transforms.TransformFVector4(lineStart);
			lineEnd = transforms.TransformFVector4(lineEnd);

			DrawDebugLine(world, lineStart, lineEnd, debugColor);
		}
		for (int32 xS = 0; xS <= vectorFieldSize.x(); ++xS)
		{
			FVector	lineStart = FVector(boxMin.x() + boxCellDim.x() * xS, boxMin.y(), boxMin.z() + boxCellDim.z() * zS);
			FVector	lineEnd = FVector(boxMin.x() + boxCellDim.x() * xS, boxMax.y(), boxMin.z() + boxCellDim.z() * zS);

			lineStart = transforms.TransformFVector4(lineStart);
			lineEnd = transforms.TransformFVector4(lineEnd);

			DrawDebugLine(world, lineStart, lineEnd, debugColor);
		}
	}

	for (int32 yS = 0; yS <= vectorFieldSize.y(); ++yS)
	{
		for (int32 xS = 0; xS <= vectorFieldSize.x(); ++xS)
		{
			FVector	lineStart = FVector(boxMin.x() + boxCellDim.x() * xS, boxMin.y() + boxCellDim.y() * yS, boxMin.z());
			FVector	lineEnd = FVector(boxMin.x() + boxCellDim.x() * xS, boxMin.y() + boxCellDim.y() * yS, boxMax.z());

			lineStart = transforms.TransformFVector4(lineStart);
			lineEnd = transforms.TransformFVector4(lineEnd);

			DrawDebugLine(world, lineStart, lineEnd, debugColor);
		}
	}
#endif // 0
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.MemberProperty != null && m_Data->m_Desc != null)
	{
		const FString	propertyName = propertyChangedEvent.MemberProperty->GetName();

		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, VectorField))
		{
			m_Data->m_NeedsReload = true;
			if (Properties.VectorField == null)
			{
				m_Data->m_Desc->Clear();
				// Disable debug rendering, we don't have that info
				if (Properties.BoundsSource == EPopcornFXVectorFieldBounds::Source)
					m_Data->m_RealExtentUnscaled = FVector3f::ZeroVector;
			}
		}
		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, Intensity))
		{
			m_Data->m_Desc->SetStrength(Properties.Intensity);
		}
		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, BoundsSource) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, VolumeDimensions))
		{
			if (Properties.VectorField != null)
				_SetBounds();
		}
		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, WrapMode) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesVectorField, SamplingMode))
		{
			u32	flags = 0;
			u32	interpolation = 0;

			_BuildVectorFieldFlags(flags, interpolation);
			m_Data->m_Desc->SetFlags(flags, static_cast<PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::EInterpolation>(interpolation));
		}
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	const FPopcornFXAttributeSamplerPropertiesVectorField *newVectorFieldProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesVectorField *>(other->GetProperties());
	if (!PK_VERIFY(newVectorFieldProperties != null))
	{
		UE_LOG(LogPopcornFXAttributeSamplerVectorField, Error, TEXT("New properties are null or not VectorField properties"));
		return;
	}

	Super::CopyPropertiesFrom(other);

	if (m_Data->m_Desc == null)
	{
		return;
	}
	if (newVectorFieldProperties->VectorField != Properties.VectorField)
	{
		m_Data->m_NeedsReload = true;
		if (Properties.VectorField == null)
		{
			m_Data->m_Desc->Clear();
			// Disable debug rendering, we don't have that info
			if (Properties.BoundsSource == EPopcornFXVectorFieldBounds::Source)
				m_Data->m_RealExtentUnscaled = FVector3f::ZeroVector;
		}
	}
	if (newVectorFieldProperties->Intensity != Properties.Intensity)
	{
		m_Data->m_Desc->SetStrength(Properties.Intensity);
	}
	if (newVectorFieldProperties->BoundsSource != Properties.BoundsSource ||
		newVectorFieldProperties->VolumeDimensions != Properties.VolumeDimensions)
	{
		if (Properties.VectorField != null)
			_SetBounds();
	}
	if (newVectorFieldProperties->WrapMode != Properties.WrapMode ||
		newVectorFieldProperties->SamplingMode != Properties.SamplingMode)
	{
		u32	flags = 0;
		u32	interpolation = 0;

		_BuildVectorFieldFlags(flags, interpolation);
		m_Data->m_Desc->SetFlags(flags, static_cast<PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::EInterpolation>(interpolation));
	}

	Properties = *newVectorFieldProperties;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::SetupDefaults(UPopcornFXEffect *effect, const uint32 samplerIdx, bool updateUnlockedValues)
{
	Super::SetupDefaults(effect, samplerIdx, updateUnlockedValues);

	const PopcornFX::PCParticleAttributeList &attrListPtr = effect->Effect()->AttributeList();

	if (attrListPtr == null || *(attrListPtr->DefaultAttributes()) == null)
	{
		return;
	}

	PopcornFX::TMemoryView<const PopcornFX::CParticleAttributeSamplerDeclaration *const>	samplerList = attrListPtr->UniqueSamplerList();
	if (samplerList.Count() == 0)
	{
		return;
	}
	const PopcornFX::CParticleAttributeSamplerDeclaration *const	samplerDesc = attrListPtr->UniqueSamplerList()[samplerIdx];
	PK_ASSERT(samplerDesc != null);
	if (samplerDesc == null)
	{
		return;
	}
	const PopcornFX::PResourceDescriptor		defaultSampler = samplerDesc->AttribSamplerDefaultValue();

	const PopcornFX::CResourceDescriptor_VectorField *vectorField = PopcornFX::HBO::Cast<PopcornFX::CResourceDescriptor_VectorField>(defaultSampler.Get());
	if (vectorField != null)
	{
		if (updateUnlockedValues)
		{
			switch (vectorField->Filtering())
			{
			case PopcornFX::EGridInterpolator::GridInterpolator_Point:
				Properties.SamplingMode = EPopcornFXVectorFieldSamplingMode::Point;
				break;
			case PopcornFX::EGridInterpolator::GridInterpolator_Trilinear:
			default:
				Properties.SamplingMode = EPopcornFXVectorFieldSamplingMode::Point;
				break;
			}
			Properties.Intensity = vectorField->Strength();
			if (vectorField->WrapVertical() && vectorField->WrapSide() && vectorField->WrapDepth())
			{
				Properties.WrapMode = EPopcornFXVectorFieldWrapMode::Wrap;
			}
		}
	}
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerVectorField::ArePropertiesSupported()
{
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerVectorField::ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	return true;
}

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor *UPopcornFXAttributeSamplerVectorField::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerVectorField::Build Vectorfield", POPCORNFX_UE_PROFILER_COLOR);

	check(m_Data != null);
	const PopcornFX::CResourceDescriptor_VectorField *defaultTurbulenceSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_VectorField>(defaultSampler);
	if (!PK_VERIFY(defaultTurbulenceSampler != null))
		return null;
	if (Properties.VectorField == null)
	{
		FString	errorMsg = TEXT("Null vector field");
#if WITH_EDITOR
		m_UnsupportedProperties.Add(TEXT("VectorField"), errorMsg);
#endif
		UE_LOG_UNSUPPORTED_SAMPLER(LogPopcornFXAttributeSamplerVectorField, Error, vectorfield, this, errorMsg);
		return null;
	}

	if (m_Data->m_Desc == null)
	{
		m_Data->m_Desc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_VectorField_Grid()); // Full override
		if (!PK_VERIFY(m_Data->m_Desc != null))
			return null;
	}
	if (m_Data->m_NeedsReload)
	{
		m_Data->m_NeedsReload = false;

		const CFloat4	dimensions = CFloat4(Properties.VectorField->SizeX, Properties.VectorField->SizeY, Properties.VectorField->SizeZ, 1);
		const u32		elementCount = dimensions.x() * dimensions.y() * dimensions.z();
		PK_ASSERT(Properties.VectorField->SourceData.GetBulkDataSize() == elementCount * sizeof(FFloat16Color));

		PopcornFX::PRefCountedMemoryBuffer	gridRawData = PopcornFX::CRefCountedMemoryBuffer::AllocAligned(sizeof(PopcornFX::f16) * 3 * elementCount, PopcornFX::Memory::CacheLineSize);
		if (!PK_VERIFY(gridRawData != null))
			return null;

		const PopcornFX::f16 *srcValues = reinterpret_cast<PopcornFX::f16 *>(Properties.VectorField->SourceData.Lock(LOCK_READ_ONLY));
		if (!PK_VERIFY(srcValues != null))
			return null;
		PopcornFX::f16 *dstValues = gridRawData->Data<PopcornFX::f16>();
		const PopcornFX::f16 *endValue = dstValues + elementCount * 3;
		while (dstValues != endValue)
		{
			*dstValues++ = *srcValues++;
			*dstValues++ = *srcValues++;
			*dstValues++ = *srcValues++;
			srcValues++;
		}
		Properties.VectorField->SourceData.Unlock();

		u32	flags = 0;
		u32	interpolation = 0;
		_BuildVectorFieldFlags(flags, interpolation);

		if (!PK_VERIFY(m_Data->m_Desc->Setup(dimensions, Properties.VectorField->Intensity, CFloat4(0.0f), CFloat4(0.0f), gridRawData,
			PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::DataType_Fp16,
			Properties.Intensity, CFloat4x4::IDENTITY, flags,
			static_cast<PopcornFX::CParticleSamplerDescriptor_VectorField_Grid::EInterpolation>(interpolation))))
			return null;

		_SetBounds();
	}

	desc.m_NeedUpdate = true;
	_AttribSampler_PreUpdate(0.f);
	return m_Data->m_Desc.Get();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerVectorField::_AttribSampler_PreUpdate(float deltaTime)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerVectorField::Update transforms", POPCORNFX_UE_PROFILER_COLOR);

	FMatrix		transforms;
	if (Properties.RotationAnimation != FVector::ZeroVector)
	{
		m_TimeAccumulation += deltaTime;

		const FVector		animationRotation = Properties.RotationAnimation * m_TimeAccumulation;
		const FTransform	eulerAngles(FQuat::MakeFromEuler(animationRotation));
		if (Properties.bUseRelativeTransform)
			transforms = (eulerAngles * GetRelativeTransform()).ToMatrixWithScale();
		else
			transforms = (eulerAngles * GetComponentTransform()).ToMatrixWithScale();
	}
	else
	{
		if (Properties.bUseRelativeTransform)
			transforms = GetRelativeTransform().ToMatrixWithScale();
		else
			transforms = GetComponentTransform().ToMatrixWithScale();
	}

#if WITH_EDITOR
	PK_ASSERT(GetWorld() != null);
	if (!GetWorld()->IsGameWorld())
	{
		const USelection *selectedAssets = GEditor->GetSelectedActors();
		PK_ASSERT(selectedAssets != null);
		const bool			isSelected = selectedAssets->IsSelected(GetOwner());
		if (m_IndirectSelectedThisTick || isSelected)
		{
			const FQuat			eulerAngles = FQuat::MakeFromEuler(Properties.RotationAnimation * m_TimeAccumulation);
			const FMatrix		worldTransforms = (FTransform(eulerAngles) * GetComponentTransform()).ToMatrixWithScale();

			RenderVectorFieldShape(worldTransforms, eulerAngles, isSelected);
			m_IndirectSelectedThisTick = false;
		}
	}
#endif // WITH_EDITOR

	const float			invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
	if (m_Data->m_Desc != null)
	{
		transforms.M[3][0] *= invGlobalScale;
		transforms.M[3][1] *= invGlobalScale;
		transforms.M[3][2] *= invGlobalScale;

		m_Data->m_Desc->SetTransforms(ToPk(transforms));
	}
}

#undef LOCTEXT_NAMESPACE
