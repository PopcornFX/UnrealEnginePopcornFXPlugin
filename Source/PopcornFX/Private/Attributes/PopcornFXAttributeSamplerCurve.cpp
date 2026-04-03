//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplerCurve.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_samplers_classes.h>
#include <pk_render_helpers/include/frame_collector/rh_frame_data.h>

#include "PopcornFXStats.h"
#include "Assets/PopcornFXEffect.h"
#include "Assets/PopcornFXEffectPriv.h"

#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveLinearColor.h"
#include "Curves/RichCurve.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplerCurve"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplerCurve, Log, All);

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSamplerCurve
//
//----------------------------------------------------------------------------

#define CURVE_MINIMUM_DELTA	0.01f

//----------------------------------------------------------------------------

struct FAttributeSamplerCurveData
{
	PopcornFX::PParticleSamplerDescriptor_Curve_Default			m_CurveDesc;
	PopcornFX::PParticleSamplerDescriptor_DoubleCurve_Default	m_DoubleCurveDesc;
	PopcornFX::CCurveDescriptor *m_Curve0 = null;
	PopcornFX::CCurveDescriptor *m_Curve1 = null;

	bool	m_NeedsReload;

	~FAttributeSamplerCurveData()
	{
		PK_ASSERT(m_Curve0 == null);
		PK_ASSERT(m_Curve1 == null);
	}
};

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurve::SetCurveDimension(TEnumAsByte<EAttributeSamplerCurveDimension::Type> InCurveDimension)
{
	if (Properties.CurveDimension == InCurveDimension)
		return;
	Properties.SecondCurve1D = null;
	//SecondCurve2D = null;
	Properties.SecondCurve3D = null;
	Properties.SecondCurve4D = null;
	Properties.Curve1D = null;
	//Curve2D = null;
	Properties.Curve3D = null;
	Properties.Curve4D = null;
	Properties.CurveDimension = InCurveDimension;
	m_Data->m_NeedsReload = true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurve::SetCurve(class UCurveBase *InCurve, bool InIsSecondCurve)
{
	if (!PK_VERIFY(InCurve != null))
		return false;
	if (InIsSecondCurve)
	{
		Properties.SecondCurve1D = null;
		//SecondCurve2D = null;
		Properties.SecondCurve3D = null;
		Properties.SecondCurve4D = null;
	}
	else
	{
		Properties.Curve1D = null;
		//Curve2D = null;
		Properties.Curve3D = null;
		Properties.Curve4D = null;
	}
	bool			ok = false;
	switch (Properties.CurveDimension)
	{
	case	EAttributeSamplerCurveDimension::Float1:
		if (InIsSecondCurve)
			ok = ((Properties.SecondCurve1D = Cast<UCurveFloat>(InCurve)) != null);
		else
			ok = ((Properties.Curve1D = Cast<UCurveFloat>(InCurve)) != null);
		break;
	case	EAttributeSamplerCurveDimension::Float2:
		PK_ASSERT_NOT_REACHED();
		ok = false;
		break;
	case	EAttributeSamplerCurveDimension::Float3:
		if (InIsSecondCurve)
			ok = ((Properties.SecondCurve3D = Cast<UCurveVector>(InCurve)) != null);
		else
			ok = ((Properties.Curve3D = Cast<UCurveVector>(InCurve)) != null);
		break;
	case	EAttributeSamplerCurveDimension::Float4:
		if (InIsSecondCurve)
			ok = ((Properties.SecondCurve4D = Cast<UCurveLinearColor>(InCurve)) != null);
		else
			ok = ((Properties.Curve4D = Cast<UCurveLinearColor>(InCurve)) != null);
		break;
	}
	m_Data->m_NeedsReload = true;
	return ok;
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerCurve::UPopcornFXAttributeSamplerCurve(const FObjectInitializer &PCIP)
	: Super(PCIP)
{
	bAutoActivate = true;
	Properties.bIsDoubleCurve = false;
	Properties.CurveDimension = EAttributeSamplerCurveDimension::Float1;

	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Curve;

	Properties.Curve1D = null;
	Properties.Curve3D = null;
	Properties.Curve4D = null;

	m_Data = new FAttributeSamplerCurveData();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurve::BeginDestroy()
{
	if (m_Data != null)
	{
		if (m_Data->m_DoubleCurveDesc != null)
		{
			m_Data->m_DoubleCurveDesc->m_Curve0 = null;
			m_Data->m_DoubleCurveDesc->m_Curve1 = null;
		}
		else if (m_Data->m_CurveDesc != null)
			m_Data->m_CurveDesc->m_Curve0 = null;
		PK_SAFE_DELETE(m_Data->m_Curve0);
		PK_SAFE_DELETE(m_Data->m_Curve1);
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

void	UPopcornFXAttributeSamplerCurve::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.Property != null)
	{
		// Always rebuild for now
		m_Data->m_NeedsReload = true;
	}
	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurve::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	const FPopcornFXAttributeSamplerPropertiesCurve *newCurveProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesCurve *>(other->GetProperties());
	if (!PK_VERIFY(newCurveProperties != null))
	{
		UE_LOG(LogPopcornFXAttributeSamplerCurve, Error, TEXT("New properties are null or not curve properties"));
		return;
	}

	Super::CopyPropertiesFrom(other);

	// Always rebuild for now
	m_Data->m_NeedsReload = true;

	Properties = *newCurveProperties;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurve::SetupDefaults(UPopcornFXEffect *effect, const uint32 samplerIdx, bool updateUnlockedValues)
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
	const PopcornFX::CParticleAttributeSamplerDeclaration * const	samplerDesc = attrListPtr->UniqueSamplerList()[samplerIdx];
	PK_ASSERT(samplerDesc != null);
	if (samplerDesc == null)
	{
		return;
	}
	const PopcornFX::PResourceDescriptor		defaultSampler = samplerDesc->AttribSamplerDefaultValue();

	const PopcornFX::CResourceDescriptor_Curve	*curve = PopcornFX::HBO::Cast<PopcornFX::CResourceDescriptor_Curve>(defaultSampler.Get());
	if (curve != null)
	{
		Properties.CurveDimension = static_cast<EAttributeSamplerCurveDimension::Type>(curve->ValueType());
	}
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurve::FetchCurveData(const FRichCurve *curve, PopcornFX::CCurveDescriptor *curveDescriptor, uint32 axis)
{
	const PopcornFX::TArray<float, PopcornFX::TArrayAligned16> &times = curveDescriptor->m_Times;
	PopcornFX::TArray<float, PopcornFX::TArrayAligned16> &values = curveDescriptor->m_FloatValues;
	PopcornFX::TArray<float, PopcornFX::TArrayAligned16> &tangents = curveDescriptor->m_FloatTangents;
	TArray<FRichCurveKey>::TConstIterator						keyIt = curve->GetKeyIterator();

	bool		isLinear = false;
	float		previousTangent = 0.0f;
	float		previousValue = keyIt ? (*keyIt).Value - CURVE_MINIMUM_DELTA : 0.0f;
	const u32	keyCount = times.Count();
	for (u32 i = 0; i < keyCount; ++i)
	{
		const int32	index = i * Properties.CurveDimension;
		const int32	tangentIndex = index * 2 + axis;
		const float	delta = i > 0 ? times[i] - times[i - 1] : 0.0f;

		if (keyIt && times[i] == (*keyIt).Time)
		{
			const FRichCurveKey &key = *keyIt++;

			values[index + axis] = key.Value;
			if (key.InterpMode == ERichCurveInterpMode::RCIM_Linear)
			{
				const float	nextValue = i + 1 < keyCount ? curve->Eval(times[i + 1]) : key.Value;
				const float	arriveTangent = key.Value - previousValue;
				const float	leaveTangent = nextValue - key.Value;

				tangents[tangentIndex] = isLinear ? arriveTangent : key.ArriveTangent * delta;
				tangents[tangentIndex + Properties.CurveDimension] = leaveTangent;
				previousTangent = leaveTangent;
				previousValue = key.Value;
			}
			else
			{
				const float	nextSampledValue = curve->Eval(times[i] + CURVE_MINIMUM_DELTA);
				const float	prevSampledValue = curve->Eval(times[i] - CURVE_MINIMUM_DELTA);
				const float	nextTime = i + 1 < times.Count() ? times[i + 1] : times[i] + CURVE_MINIMUM_DELTA;
				const float	nextDelta = nextTime - times[i];

				tangents[tangentIndex] = isLinear ? previousTangent : ((nextSampledValue - prevSampledValue) / (2.0f * CURVE_MINIMUM_DELTA)) * delta;
				tangents[tangentIndex + Properties.CurveDimension] = key.LeaveTangent * nextDelta;
			}
			isLinear = key.InterpMode == ERichCurveInterpMode::RCIM_Linear;
		}
		else
		{
			const float	value = curve->Eval(times[i]);

			// We do have to eval the key value because each axis can have independant
			// key count and times..
			values[index + axis] = value;
			if (isLinear)
			{
				const float	nextValue = i + 1 < keyCount ? curve->Eval(times[i + 1]) : value;
				const float	arriveTangent = value - previousValue;
				const float	leaveTangent = nextValue - value;

				tangents[tangentIndex] = arriveTangent;
				tangents[tangentIndex + Properties.CurveDimension] = leaveTangent;
				previousTangent = leaveTangent;
				previousValue = value;
			}
			else
			{
				const float	nextSampledValue = curve->Eval(times[i] + CURVE_MINIMUM_DELTA);
				const float	prevSampledValue = curve->Eval(times[i] - CURVE_MINIMUM_DELTA);
				const float	nextTime = i + 1 < times.Count() ? times[i + 1] : times[i] + CURVE_MINIMUM_DELTA;
				const float	nextDelta = nextTime - times[i];

				tangents[tangentIndex] = ((nextSampledValue - prevSampledValue) / (2.0f * CURVE_MINIMUM_DELTA)) * delta;
				tangents[tangentIndex + Properties.CurveDimension] = ((nextSampledValue - prevSampledValue) / (2.0f * CURVE_MINIMUM_DELTA)) * nextDelta;
			}
		}
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerCurve::GetAssociatedCurves(UCurveBase *&curve0, UCurveBase *&curve1)
{
	switch (Properties.CurveDimension)
	{
	case	EAttributeSamplerCurveDimension::Float1:
	{
		curve0 = Properties.Curve1D;
		curve1 = Properties.SecondCurve1D;
		break;
	}
	case	EAttributeSamplerCurveDimension::Float2:
	{
		break;
	}
	case	EAttributeSamplerCurveDimension::Float3:
	{
		curve0 = Properties.Curve3D;
		curve1 = Properties.SecondCurve3D;
		break;
	}
	case	EAttributeSamplerCurveDimension::Float4:
	{
		curve0 = Properties.Curve4D;
		curve1 = Properties.SecondCurve4D;
		break;
	}
	}
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurve::SetupCurve(PopcornFX::CCurveDescriptor *curveDescriptor, UCurveBase *curve)
{
	static const float	kMaximumKey = 1.0f - CURVE_MINIMUM_DELTA;
	static const float	kMinimumKey = CURVE_MINIMUM_DELTA;

	TArray<FRichCurveEditInfo>	curves = curve->GetCurves();
	if (!PK_VERIFY(curves.Num() == Properties.CurveDimension))
		return false;

	curveDescriptor->m_Order = (u32)Properties.CurveDimension;
	curveDescriptor->m_Interpolator = PopcornFX::CInterpolableVectorArray::Interpolator_Hermite;

	TArray<float>	collectedTimes;
	// Fetch every independant keys
	for (int32 i = 0; i < Properties.CurveDimension; ++i)
	{
		const FRichCurve *fcurve = static_cast<const FRichCurve *>(curves[i].CurveToEdit);
		PK_ASSERT(fcurve != null);
		for (auto keyIt = fcurve->GetKeyIterator(); keyIt; ++keyIt)
			collectedTimes.AddUnique((*keyIt).Time);
	}
	collectedTimes.RemoveAll([](const float &time) { return time > 1.0f; });
	collectedTimes.Sort();

	// Not enough keys: return (should handle this ? init curve at 0 ?)
	if (collectedTimes.Num() <= 0)
		return false;
	if (collectedTimes.Last() < kMaximumKey)
		collectedTimes.Add(1.0f);
	if (collectedTimes[0] > kMinimumKey)
		collectedTimes.Insert(0.0f, 0);

	// Resize the curve if necessary
	const u32	keyCount = collectedTimes.Num();
	if (curveDescriptor->m_Times.Count() != keyCount)
	{
		if (!PK_VERIFY(curveDescriptor->Resize(keyCount)))
			return false;
	}
	PopcornFX::TArray<float, PopcornFX::TArrayAligned16> &times = curveDescriptor->m_Times;
	PK_ASSERT(times.Count() == collectedTimes.Num());
	PK_ASSERT(times.RawDataPointer() != null);
	PK_ASSERT(collectedTimes.GetData() != null);
	memcpy(times.RawDataPointer(), collectedTimes.GetData(), keyCount * sizeof(float));

	float	minValues[4] = { 0 };
	float	maxValues[4] = { 0 };
	for (int32 i = 0; i < Properties.CurveDimension; ++i)
	{
		const FRichCurve *fcurve = static_cast<const FRichCurve *>(curves[i].CurveToEdit);
		PK_ASSERT(fcurve != null);

		fcurve->GetValueRange(minValues[i], maxValues[i]);
		FetchCurveData(fcurve, curveDescriptor, i);
	}
	curveDescriptor->m_MinEvalLimits = CFloat4(minValues[0], minValues[1], minValues[2], minValues[3]);
	curveDescriptor->m_MaxEvalLimits = CFloat4(maxValues[0], maxValues[1], maxValues[2], maxValues[3]);
	curveDescriptor->m_Interpolator = PopcornFX::CInterpolableVectorArray::Interpolator_Hermite;
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurve::RebuildCurvesData()
{
	UCurveBase *curve0 = null;
	UCurveBase *curve1 = null;

	GetAssociatedCurves(curve0, curve1);
	if (curve0 == null || (Properties.bIsDoubleCurve && curve1 == null))
		return false;
	bool	success = true;

	success &= SetupCurve(m_Data->m_Curve0, curve0);
	success &= !Properties.bIsDoubleCurve || SetupCurve(m_Data->m_Curve1, curve1);
	return success;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurve::ArePropertiesSupported()
{
	if (Properties.CurveDimension == EAttributeSamplerCurveDimension::Float2)
	{
		FString errorMsg = TEXT("2D Curves are not supported");
#if WITH_EDITOR
		m_UnsupportedProperties.FindOrAdd(TEXT("CurveDimension"), *errorMsg);
#endif
		//PK_ASSERT_NOT_REACHED();
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerCurve::ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	// Make sure the sampler matches what the effect expects
	// Mismatchs should only happen when using an external sampler
	const PopcornFX::CResourceDescriptor_Curve	*defaultCurveSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Curve>(defaultSampler);
	if (!PK_VERIFY(defaultCurveSampler != null))
		return false;

	if (Properties.CurveDimension != static_cast<EAttributeSamplerCurveDimension::Type>(defaultCurveSampler->ValueType()))
	{
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor *UPopcornFXAttributeSamplerCurve::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	PK_TODO("Determine when this should be set to true : has the curve asset changed ?")
		const PopcornFX::CResourceDescriptor_Curve *defaultCurveSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Curve>(defaultSampler);
	const PopcornFX::CResourceDescriptor_DoubleCurve *defaultDoubleCurveSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_DoubleCurve>(defaultSampler);
	if (!PK_VERIFY(defaultCurveSampler != null || defaultDoubleCurveSampler != null))
		return null;
	PK_ASSERT(defaultCurveSampler == null || defaultDoubleCurveSampler == null);
	m_Data->m_NeedsReload = true;
	const bool	defaultIsDoubleCurve = defaultDoubleCurveSampler != null;
	if (m_Data->m_NeedsReload)
	{
		if (defaultIsDoubleCurve != Properties.bIsDoubleCurve)
			return null;
		if (m_Data->m_Curve0 == null)
			m_Data->m_Curve0 = PK_NEW(PopcornFX::CCurveDescriptor());
		if (Properties.bIsDoubleCurve)
		{
			if (m_Data->m_Curve1 == null)
				m_Data->m_Curve1 = PK_NEW(PopcornFX::CCurveDescriptor());
		}
		else if (m_Data->m_Curve1 != null)
			m_Data->m_Curve1 = null;
		if (!RebuildCurvesData())
			return null;
		m_Data->m_NeedsReload = false;
		if (m_Data->m_CurveDesc == null &&
			m_Data->m_DoubleCurveDesc == null)
		{
			if (defaultIsDoubleCurve)
			{
				m_Data->m_DoubleCurveDesc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_DoubleCurve_Default());
				if (PK_VERIFY(m_Data->m_DoubleCurveDesc != null))
				{
					m_Data->m_DoubleCurveDesc->m_Curve0 = m_Data->m_Curve0;
					m_Data->m_DoubleCurveDesc->m_Curve1 = m_Data->m_Curve1;
				}
			}
			else
			{
				m_Data->m_CurveDesc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_Curve_Default());
				if (PK_VERIFY(m_Data->m_CurveDesc != null))
					m_Data->m_CurveDesc->m_Curve0 = m_Data->m_Curve0;
			}
		}
	}
	if (defaultIsDoubleCurve)
		return m_Data->m_DoubleCurveDesc.Get();
	return m_Data->m_CurveDesc.Get();
	//return defaultIsDoubleCurve ? m_Data->m_DoubleCurveDesc.Get() : m_Data->m_CurveDesc.Get();
}

#undef LOCTEXT_NAMESPACE
