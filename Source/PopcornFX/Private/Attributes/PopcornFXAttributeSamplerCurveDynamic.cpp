//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplerCurveDynamic.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXAttributeList.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_samplers_classes.h>

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplerCurveDynamic"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplerCurveDynamic, Log, All);

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSamplerCurveDynamic
//
//----------------------------------------------------------------------------

struct FAttributeSamplerCurveDynamicData
{
	PopcornFX::PParticleSamplerDescriptor_Curve_Default	m_Desc;
	PopcornFX::CCurveDescriptor							*m_Curve0 = null;

	PopcornFX::TArray<float>	m_Times;
	PopcornFX::TArray<float>	m_Tangents;
	PopcornFX::TArray<float>	m_Values;
	bool						m_DirtyValues;

	~FAttributeSamplerCurveDynamicData()
	{
		PK_ASSERT(m_Curve0 == null);
	}
};

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerCurveDynamic::UPopcornFXAttributeSamplerCurveDynamic(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	bAutoActivate = true;

	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Curve;

	m_Data = new FAttributeSamplerCurveDynamicData();

	CurveDimension = EAttributeSamplerCurveDimension::Float1;
	CurveInterpolator = ECurveDynamicInterpolator::Linear;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurveDynamic::BeginDestroy()
{
	if (m_Data != null)
	{
		if (m_Data->m_Desc != null)
			m_Data->m_Desc->m_Curve0 = null;
		PK_SAFE_DELETE(m_Data->m_Curve0);
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::CreateCurveIFN()
{
	PK_ASSERT(m_Data != null);
	if (m_Data->m_Curve0 == null)
	{
		m_Data->m_Curve0 = PK_NEW(PopcornFX::CCurveDescriptor());
		if (PK_VERIFY(m_Data->m_Curve0 != null))
		{
			m_Data->m_DirtyValues = true;
			m_Data->m_Curve0->m_Order = (u32)CurveDimension;
			m_Data->m_Curve0->m_Interpolator = CurveInterpolator == ECurveDynamicInterpolator::Linear ?
				PopcornFX::CInterpolableVectorArray::Interpolator_Linear :
				PopcornFX::CInterpolableVectorArray::Interpolator_Hermite;

			//m_Data->m_Curve0->SetEvalLimits(CFloat4(0.0f), CFloat4(1.0f));
		}
	}
	return m_Data->m_Curve0 != null;
}

//----------------------------------------------------------------------------

template <class _Type>
bool	UPopcornFXAttributeSamplerCurveDynamic::SetValuesGeneric(const TArray<_Type> &values)
{
	const u32	valueCount = values.Num();
	if (valueCount < 2)
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurveDynamic, Warning, TEXT("Couldn't SetValues, Values should contain at least 2 items."));
		return false;
	}
	if ((m_Data->m_Values.Count() / (u32)CurveDimension) != values.Num())
	{
		if (!PK_VERIFY(m_Data->m_Values.Resize(valueCount * (u32)CurveDimension)))
			return false;
	}
	PopcornFX::Mem::Copy(m_Data->m_Values.RawDataPointer(), values.GetData(), sizeof(float) * (u32)CurveDimension * valueCount);
	m_Data->m_DirtyValues = true;
	return true;
}

//----------------------------------------------------------------------------

template <class _Type>
bool	UPopcornFXAttributeSamplerCurveDynamic::SetTangentsGeneric(const TArray<_Type> &arriveTangents, const TArray<_Type> &leaveTangents)
{
	if (arriveTangents.Num() != leaveTangents.Num())
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurveDynamic, Warning, TEXT("Couldn't SetTangents: ArriveTangents count differs from LeaveTangents"));
		return false;
	}
	const u32	tangentCount = arriveTangents.Num();
	if (tangentCount < 2)
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurveDynamic, Warning, TEXT("Couldn't SetValues, Values should contain at least 2 items."));
		return false;
	}
	const u32	dim = (u32)CurveDimension;
	if ((m_Data->m_Tangents.Count() / dim / 2) != tangentCount)
	{
		if (!PK_VERIFY(m_Data->m_Tangents.Resize(tangentCount * dim * 2)))
			return false;
	}
	const float	*prevTangents = reinterpret_cast<const float*>(arriveTangents.GetData());
	const float	*nextTangents = reinterpret_cast<const float*>(leaveTangents.GetData());
	float		*dstTangents = m_Data->m_Tangents.RawDataPointer();

	for (u32 iTangent = 0; iTangent < tangentCount; ++iTangent)
	{
		for (u32 iDim = 0; iDim < dim; ++iDim)
			*dstTangents++ = *prevTangents++;
		for (u32 iDim = 0; iDim < dim; ++iDim)
			*dstTangents++ = *nextTangents++;
	}
	m_Data->m_DirtyValues = true;
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetTimes(const TArray<float> &Times)
{
	PK_ASSERT(m_Data != null);

	PK_ONLY_IF_ASSERTS(
		for (s32 iTime = 0; iTime < Times.Num(); ++iTime)
			PK_ASSERT(Times[iTime] >= 0.0f && Times[iTime] <= 1.0f);
	);

	if (Times.Num() < 2)
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurveDynamic, Warning, TEXT("Couldn't SetTimes: Times should contain at least 2 items."));
		return false;
	}
	const u32	timesCount = Times.Num();
	if (PK_VERIFY(m_Data->m_Times.Resize(timesCount)))
	{
		m_Data->m_DirtyValues = true;
		PopcornFX::Mem::Copy(m_Data->m_Times.RawDataPointer(), Times.GetData(), sizeof(float) * timesCount);
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetValues1D(const TArray<float> &Values)
{
	PK_ASSERT(m_Data != null);
	if (CurveDimension != EAttributeSamplerCurveDimension::Float1)
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurveDynamic, Warning, TEXT("Couldn't SetValues1D: Curve doesn't have Float1 dimension"));
		return false;
	}
	return SetValuesGeneric<float>(Values);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetValues3D(const TArray<FVector> &Values)
{
	PK_ASSERT(m_Data != null);
	if (CurveDimension != EAttributeSamplerCurveDimension::Float3)
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurveDynamic, Warning, TEXT("Couldn't SetValues3D: Curve doesn't have Float3 dimension"));
		return false;
	}
	return SetValuesGeneric<FVector>(Values);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetValues4D(const TArray<FLinearColor> &Values)
{
	PK_ASSERT(m_Data != null);
	if (CurveDimension != EAttributeSamplerCurveDimension::Float4)
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurveDynamic, Warning, TEXT("Couldn't SetValues4D: Curve doesn't have Float4 dimension"));
		return false;
	}
	return SetValuesGeneric<FLinearColor>(Values);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetTangents1D(const TArray<float> &ArriveTangents, const TArray<float> &LeaveTangents)
{
	PK_ASSERT(m_Data != null);
	if (CurveInterpolator != ECurveDynamicInterpolator::Spline)
		return true; // No need to set tangents
	if (CurveDimension != EAttributeSamplerCurveDimension::Float1)
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurveDynamic, Warning, TEXT("Couldn't SetTangents1D: Curve doesn't have Float1 dimension"));
		return false;
	}
	return SetTangentsGeneric<float>(ArriveTangents, LeaveTangents);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetTangents3D(const TArray<FVector> &ArriveTangents, const TArray<FVector> &LeaveTangents)
{
	PK_ASSERT(m_Data != null);
	if (CurveInterpolator != ECurveDynamicInterpolator::Spline)
		return true; // No need to set tangents
	if (CurveDimension != EAttributeSamplerCurveDimension::Float3)
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurveDynamic, Warning, TEXT("Couldn't SetTangents3D: Curve doesn't have Float3 dimension"));
		return false;
	}
	return SetTangentsGeneric<FVector>(ArriveTangents, LeaveTangents);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurveDynamic::SetTangents4D(const TArray<FLinearColor> &ArriveTangents, const TArray<FLinearColor> &LeaveTangents)
{
	PK_ASSERT(m_Data != null);
	if (CurveInterpolator != ECurveDynamicInterpolator::Spline)
		return true; // No need to set tangents
	if (CurveDimension != EAttributeSamplerCurveDimension::Float4)
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurveDynamic, Warning, TEXT("Couldn't SetTangents4D: Curve doesn't have Float4 dimension"));
		return false;
	}
	return SetTangentsGeneric<FLinearColor>(ArriveTangents, LeaveTangents);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurveDynamic::_AttribSampler_PreUpdate(float deltaTime)
{
	PK_ASSERT(m_Data != null);

	PopcornFX::CCurveDescriptor	*curve = m_Data->m_Curve0;
	if (curve == null || !m_Data->m_DirtyValues)
		return;
	if (m_Data->m_Values.Empty())
		return;

	m_Data->m_DirtyValues = false;

	const bool	hasTimes = !m_Data->m_Times.Empty();
	const bool	hasTangents = !m_Data->m_Tangents.Empty();
	if (!hasTimes)
	{
		// Generate the times based on the value count (minus one: the first value is at time 0.0f, and the last is at time 1.0f)
		const u32	timesCount = m_Data->m_Values.Count() / (u32)CurveDimension;
		if (!PK_VERIFY(m_Data->m_Times.Resize(timesCount)))
			return;

		float		*times = m_Data->m_Times.RawDataPointer();
		const float	step = 1.0f / (timesCount - 1);
		for (u32 iTime = 0; iTime < timesCount; ++iTime)
			*times++ = step * iTime;
	}

	if (!CreateCurveIFN())
		return;
	const u32	valueCount = m_Data->m_Times.Count();
	if (!PK_VERIFY(curve->Resize(valueCount)))
		return;

	if ((hasTangents && m_Data->m_Tangents.Count() / 2 != m_Data->m_Values.Count()) ||
		(hasTimes && m_Data->m_Times.Count() != m_Data->m_Values.Count() / (u32)CurveDimension))
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurveDynamic, Warning, TEXT("Tangents/Times count differs from values. Make sure to set the same number of values/tangents/times"));
		return;
	}
	PopcornFX::Mem::Copy(curve->m_Times.RawDataPointer(), m_Data->m_Times.RawDataPointer(), sizeof(float) * valueCount);
	PopcornFX::Mem::Copy(curve->m_FloatValues.RawDataPointer(), m_Data->m_Values.RawDataPointer(), sizeof(float) * (u32)CurveDimension * valueCount);

	if (CurveInterpolator == ECurveDynamicInterpolator::Spline)
	{
		const u32	tangentsByteCount = sizeof(float) * (u32)CurveDimension * 2 * valueCount;

		if (hasTangents)
			PopcornFX::Mem::Copy(curve->m_FloatTangents.RawDataPointer(), m_Data->m_Tangents.RawDataPointer(), tangentsByteCount);
		else
			PopcornFX::Mem::Clear(curve->m_FloatTangents.RawDataPointer(), tangentsByteCount);
	}
}

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerCurveDynamic::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_ASSERT(m_Data != null);
	const PopcornFX::CResourceDescriptor_Curve			*defaultCurveSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Curve>(defaultSampler);
	const PopcornFX::CResourceDescriptor_DoubleCurve	*defaultDoubleCurveSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_DoubleCurve>(defaultSampler);
	if (!PK_VERIFY(defaultCurveSampler != null || defaultDoubleCurveSampler != null))
		return null;
	PK_ASSERT(defaultCurveSampler == null || defaultDoubleCurveSampler != null);

	const bool	defaultIsDoubleCurve = defaultDoubleCurveSampler != null;
	if (defaultIsDoubleCurve)
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurveDynamic, Warning, TEXT("Couldn't setup UPopcornFXAttributeSamplerCurveDynamic: Source curve is DoubleCurve, not supported by dynamic curve attr sampler."));
		return null;
	}
	if (!CreateCurveIFN())
		return null;
	if (m_Data->m_Desc == null)
		m_Data->m_Desc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_Curve_Default(m_Data->m_Curve0));
	PK_ASSERT(m_Data->m_Desc->m_Curve0 == m_Data->m_Curve0);
	desc.m_NeedUpdate = true;
	_AttribSampler_PreUpdate(0.f);
	return m_Data->m_Desc.Get();
}

#undef LOCTEXT_NAMESPACE
