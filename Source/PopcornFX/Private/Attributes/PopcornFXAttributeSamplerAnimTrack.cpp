//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplerAnimTrack.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_samplers_classes.h>
#include <pk_render_helpers/include/frame_collector/rh_frame_data.h>

#include "PopcornFXPlugin.h"
#include "PopcornFXStats.h"
#include "PopcornFXAttributeList.h"
#include "Assets/PopcornFXEffect.h"
#include "Assets/PopcornFXEffectPriv.h"


#if (PK_GPU_D3D12 != 0)
#	include <pk_particles/include/Samplers/D3D12/image_gpu_d3d12.h>
#	include <pk_particles/include/Samplers/D3D12/rectangle_list_gpu_d3d12.h>

#	include "Windows/HideWindowsPlatformTypes.h"
#	include "D3D12RHIPrivate.h"
#	include "D3D12Util.h"
#endif // (PK_GPU_D3D12 != 0)

#include "Components/SplineComponent.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplerAnimTrack"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplerAnimTrack, Log, All);

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSamplerAnimTrack
//
//----------------------------------------------------------------------------

namespace	PopcornFXAttributeSamplers
{
	extern VectorRegister4f _TransformVector(VectorRegister4f &v, const FMatrix44f *m);
}

// UE animtrack attribute sampler: only contains one track
// Expensive sampling as each particle individually evaluates the UE curve.
class	CParticleSamplerDescriptor_AnimTrack_UE : public PopcornFX::CParticleSamplerDescriptor_AnimTrack
{
public:
	CParticleSamplerDescriptor_AnimTrack_UE(const FMatrix44f *trackTransforms)
	:	m_TransformFlags(0)
	,	m_TrackTransforms(trackTransforms)
	{
	}

	bool	Setup(const USplineComponent *splineComponent, u32 transformFlags)
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::Setup", POPCORNFX_UE_PROFILER_COLOR);

		PK_ASSERT(splineComponent != null);

		m_Positions.Reset();
		m_Rotations.Reset();
		m_Scales.Reset();
		m_ReparamTable.Reset();

		m_TransformFlags = transformFlags;
		if ((m_TransformFlags & PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Translate) != 0)
			m_Positions = splineComponent->GetSplinePointsPosition();
		if ((m_TransformFlags & PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Rotate) != 0)
			m_Rotations = splineComponent->GetSplinePointsRotation();
		if ((m_TransformFlags & PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Scale) != 0)
			m_Scales = splineComponent->GetSplinePointsScale();

		if (m_Positions.Points.Num() > 0 || m_Rotations.Points.Num() > 0 || m_Scales.Points.Num() > 0)
		{
			m_ReparamTable = splineComponent->SplineCurves.ReparamTable;
			m_SplineLength = splineComponent->GetSplineLength();
		}
		return true;
	}

	virtual void	SamplePosition(	const TStridedMemoryView<CFloat3>		&dstValues,
									const TStridedMemoryView<const float>	&times,
									const TStridedMemoryView<const s32>		&trackIds) const override
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::SamplePosition", POPCORNFX_UE_PROFILER_COLOR);

		PK_ASSERT(dstValues.Count() == times.Count());

		const float		invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
		const u32		timesCount = times.Count();

		for (u32 idxTime = 0; idxTime < timesCount; ++idxTime)
		{
			const float	currentTime = m_ReparamTable.Eval(times[idxTime] * m_SplineLength, 0.0f);

			dstValues[idxTime] = ToPk(m_Positions.Eval(currentTime)) * invGlobalScale;
		}
		_TransformValues(*m_TrackTransforms, dstValues);
	}

	virtual void	SampleOrientation(	const TStridedMemoryView<CQuaternion>	&dstValues,
										const TStridedMemoryView<const float>	&times,
										const TStridedMemoryView<const s32>		&trackIds) const override
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::SampleOrientation", POPCORNFX_UE_PROFILER_COLOR);

		PK_ASSERT(dstValues.Count() == times.Count());

		const u32	timesCount = times.Count();

		for (u32 idxTime = 0; idxTime < timesCount; ++idxTime)
		{
			const float currentTime = m_ReparamTable.Eval(times[idxTime] * m_SplineLength, 0.0f);

			dstValues[idxTime] = ToPk(m_Rotations.Eval(currentTime));
		}
		_TransformValues(*m_TrackTransforms, dstValues);
	}

	virtual void	SampleScale(	const TStridedMemoryView<CFloat3>		&dstValues,
									const TStridedMemoryView<const float>	&times,
									const TStridedMemoryView<const s32>		&trackIds) const override
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::SampleScale", POPCORNFX_UE_PROFILER_COLOR);

		PK_ASSERT(dstValues.Count() == times.Count());

		const u32	timesCount = times.Count();

		for (u32 idxTime = 0; idxTime < timesCount; ++idxTime)
		{
			const float currentTime = m_ReparamTable.Eval(times[idxTime] * m_SplineLength, 0.0f);

			dstValues[idxTime] = ToPk(m_Scales.Eval(currentTime));
		}
		_TransformValues(*m_TrackTransforms, dstValues);
	}

	virtual void	Transform(	const TStridedMemoryView<CFloat3>		&outSamples,
								const TStridedMemoryView<const CFloat3>	&locations,
								const TStridedMemoryView<const float>	&times,
								const TStridedMemoryView<const s32>		&trackIds,
								u32										transformFlags) const override
	{
		_Transform<false>(outSamples, locations, times, transformFlags);
		_TransformValues(*m_TrackTransforms, outSamples);
	}

	virtual void	GetTrackCount(const TStridedMemoryView<s32> &dstValues) const override
	{
		PK_ASSERT(!dstValues.Empty());
		const u32	trackCount = 1;
		if (!dstValues.Virtual())
		{
			PK_RELEASE_ASSERT(dstValues.Contiguous());
			PopcornFX::Mem::Fill32(dstValues.Data(), trackCount, dstValues.Count());
		}
		else
			dstValues[0] = trackCount;
	}

	virtual void	GetTrackDuration(const TStridedMemoryView<float> &dstValues, const TStridedMemoryView<const s32> &trackIds) const override
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::GetTrackDuration", POPCORNFX_UE_PROFILER_COLOR);
		(void)trackIds;
		PK_ASSERT(!dstValues.Empty());
		const float	broadcast = m_SplineLength;
		if (!dstValues.Virtual())
			PopcornFX::Mem::Fill32(dstValues.Data(), *reinterpret_cast<const u32*>(&broadcast), dstValues.Count());
		else
			dstValues[0] = broadcast;
	}

	virtual void	GetTrackLength(const TStridedMemoryView<float> &dstValues, const TStridedMemoryView<const s32> &trackIds) const override
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::GetTrackLength", POPCORNFX_UE_PROFILER_COLOR);
		(void)trackIds;
		PK_ASSERT(!dstValues.Empty());
		const float	broadcast = 1.0f;
		if (!dstValues.Virtual())
			PopcornFX::Mem::Fill32(dstValues.Data(), *reinterpret_cast<const u32*>(&broadcast), dstValues.Count());
		else
			dstValues[0] = broadcast;
	}

	virtual bool	SupportsSim(const PopcornFX::CParticleUpdater *updater) const override
	{
		(void)updater;
		PK_ASSERT(updater != null);
#if (PK_PARTICLES_UPDATER_USE_D3D12 != 0)
		if (updater->Manager()->UpdateClass() == PopcornFX::CParticleUpdateManager_D3D12::DefaultUpdateClass())
			return m_TrackResourceD3D12 != null;
#endif
		
		return true;
	}
#if (PK_PARTICLES_UPDATER_USE_D3D12 != 0)
	virtual void	BindAnimTrackResourceD3D12(const PopcornFX::SBindingContextD3D12 &context) const override
	{
		if (m_TrackResourceD3D12 != null)
		{
			context.m_Heap.BindTextureShaderResourceView(context.m_Device, m_TrackResourceD3D12, context.m_Location, m_TrackResourceD3D12->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D);
			context.m_Heap.KeepResourceReference(m_TrackResourceD3D12);
		}
	}
#endif

private:
	template<bool _InverseTransforms>
	void	_Transform(const TStridedMemoryView<CFloat3>		&outSamples,
					   const TStridedMemoryView<const CFloat3>	&locations,
					   const TStridedMemoryView<const float>	&cursors,
					   u32										transformFlags) const
	{
		PK_NAMEDSCOPEDPROFILE_C("CParticleSamplerDescriptor_AnimTrack_UE::_Transform", POPCORNFX_UE_PROFILER_COLOR);

		PK_ASSERT(outSamples.Count() == locations.Count());
		PK_ASSERT(outSamples.Count() == cursors.Count());

		bool	translate = ((m_TransformFlags & CParticleSamplerDescriptor_AnimTrack::Transform_Translate) != 0);
		bool	rotate = ((m_TransformFlags & CParticleSamplerDescriptor_AnimTrack::Transform_Rotate) != 0);
		bool	scale = ((m_TransformFlags & CParticleSamplerDescriptor_AnimTrack::Transform_Scale) != 0);

		translate = translate && ((transformFlags & CParticleSamplerDescriptor_AnimTrack::Transform_Translate) != 0);
		rotate = rotate && ((transformFlags & CParticleSamplerDescriptor_AnimTrack::Transform_Rotate) != 0);
		scale = scale && ((transformFlags & CParticleSamplerDescriptor_AnimTrack::Transform_Scale) != 0);

		if (translate || rotate || scale)
		{
			const float		invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();
			const u32		cursorCount = cursors.Count();
			for (u32 iCursor = 0; iCursor < cursorCount; ++iCursor)
			{
				const float		time = m_ReparamTable.Eval(cursors[iCursor] * m_SplineLength, 0.0f);

				outSamples[iCursor] = locations[iCursor];
				if (!_InverseTransforms)
				{
					if (scale)
						outSamples[iCursor] *= ToPk(m_Scales.Eval(time));
					if (rotate)
						outSamples[iCursor] = ToPk(m_Rotations.Eval(time).RotateVector(FVector(ToUE(outSamples[iCursor]))));
					if (translate)
						outSamples[iCursor] += ToPk(m_Positions.Eval(time)) * invGlobalScale;
				}
				else
				{
					if (translate)
						outSamples[iCursor] -= ToPk(m_Positions.Eval(time)) * invGlobalScale;
					if (rotate)
						outSamples[iCursor] = ToPk(m_Rotations.Eval(time).Inverse().RotateVector(FVector(ToUE(outSamples[iCursor]))));
					if (scale)
						outSamples[iCursor] /= ToPk(m_Scales.Eval(time));
				}
			}
		}
	}

	void	_TransformValues(const FMatrix44f &transforms, const TStridedMemoryView<CFloat3> &outSamples) const
	{
		if (transforms.Equals(FMatrix44f::Identity, 1.0e-6f))
			return;
		const u32	sampleCount = outSamples.Count();
		const u32	stride = outSamples.Stride();
		CFloat3		*positions = outSamples.Data();
		for (u32 iSample = 0; iSample < sampleCount; ++iSample)
		{
			VectorRegister4f	srcPos = VectorLoadFloat3(reinterpret_cast<FVector3f*>(positions));

			srcPos = PopcornFXAttributeSamplers::_TransformVector(srcPos, &transforms);
			VectorStoreFloat3(srcPos, reinterpret_cast<FVector3f*>(positions));
			positions = PopcornFX::Mem::AdvanceRawPointer(positions, stride);
		}
	}

	void	_TransformValues(const FMatrix44f &transforms, const TStridedMemoryView<CQuaternion> &outSamples) const
	{
		// TODO
	}

private:
	// Local copy of USplineComponent's curves
	FInterpCurveVector		m_Positions;
	FInterpCurveQuat		m_Rotations;
	FInterpCurveVector		m_Scales;
	FInterpCurveFloat		m_ReparamTable;

	u32						m_TransformFlags;
	float					m_SplineLength;
	const FMatrix44f		*m_TrackTransforms;
#if (PK_PARTICLES_UPDATER_USE_D3D12 != 0)
	ID3D12Resource			*m_TrackResourceD3D12 = null;
#endif
};
PK_DECLARE_REFPTRCLASS(ParticleSamplerDescriptor_AnimTrack_UE);

//----------------------------------------------------------------------------

struct	FAttributeSamplerAnimTrackData
{
	PParticleSamplerDescriptor_AnimTrack_UE		m_Desc = null;

	PopcornFX::PParticleSamplerDescriptor_AnimTrack_Default	m_DescFast = null;
	PopcornFX::CCurveDescriptor								*m_Positions = null;
	PopcornFX::CCurveDescriptor								*m_Scales = null;

	TWeakObjectPtr<USplineComponent>		m_CurrentSplineComponent = null;

	bool	m_NeedsReload = false;
};

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerAnimTrack::UPopcornFXAttributeSamplerAnimTrack(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	bAutoActivate = true;

	// By default, only translate
	Properties.bTranslate = true;
	Properties.bScale = false;
	Properties.bRotate = false;
	Properties.bFastSampler = true;
	Properties.bEditorRebuildEachFrame = false;

	Properties.Transforms = EPopcornFXSplineTransforms::AttrSamplerRelativeTr;

	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::AnimTrack;

	m_Data = new FAttributeSamplerAnimTrackData();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerAnimTrack::BeginDestroy()
{
	if (m_Data != null)
	{
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

USplineComponent	*UPopcornFXAttributeSamplerAnimTrack::ResolveSplineComponent(bool logErrors)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerAnimTrack::ResolveSplineComponent", POPCORNFX_UE_PROFILER_COLOR);

	AActor	*fallbackActor = GetOwner();
	if (fallbackActor == null)
	{
		FString errorMsg(TEXT("Could not find fallback actor"));
#if WITH_EDITOR
		m_UnsupportedProperties.Add(TEXT("TargetActor"), errorMsg);
#endif
		UE_LOG_UNSUPPORTED_SAMPLER(LogPopcornFXAttributeSamplerAnimTrack, Error, animtrack, this, errorMsg);
		return null;
	}
	const AActor		*parent = Properties.TargetActor == null ? fallbackActor : Properties.TargetActor;
	USplineComponent	*spline = null;
	if (Properties.SplineComponentName != NAME_None)
	{
		FObjectPropertyBase	*prop = FindFProperty<FObjectPropertyBase>(parent->GetClass(), Properties.SplineComponentName);

		if (prop != null)
			spline = Cast<USplineComponent>(prop->GetObjectPropertyValue_InContainer(parent));
	}
	else
	{
		spline = Cast<USplineComponent>(parent->GetRootComponent());
	}
	if (spline == null)
	{
		FString errorMsg(TEXT("Could not find USPlineComponent in target actor"));
#if WITH_EDITOR
		m_UnsupportedProperties.Add(TEXT("SplineComponentName"), errorMsg);
#endif
		if (logErrors)
		{
			UE_LOG_UNSUPPORTED_SAMPLER(LogPopcornFXAttributeSamplerAnimTrack, Error, animtrack, this, errorMsg);
		}
		return null;
	}
	return spline;
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
void	UPopcornFXAttributeSamplerAnimTrack::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.Property != NULL)
	{
		if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, TargetActor) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, SplineComponentName) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, bTranslate) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, bRotate) ||
			propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesAnimTrack, bScale))
		{
			m_Data->m_NeedsReload = true;
		}
	}
	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerAnimTrack::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	const FPopcornFXAttributeSamplerPropertiesAnimTrack *newAnimTrackProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesAnimTrack *>(other->GetProperties());
	if (!PK_VERIFY(newAnimTrackProperties != null))
	{
		UE_LOG(LogPopcornFXAttributeSamplerAnimTrack, Error, TEXT("New properties are null or not AnimTrack properties"));
		return;
	}

	Super::CopyPropertiesFrom(other);

	if (newAnimTrackProperties->TargetActor != Properties.TargetActor ||
		newAnimTrackProperties->SplineComponentName != Properties.SplineComponentName ||
		newAnimTrackProperties->bTranslate != Properties.bTranslate ||
		newAnimTrackProperties->bRotate != Properties.bRotate ||
		newAnimTrackProperties->bScale != Properties.bScale)
	{
		m_Data->m_NeedsReload = true;
	}

	Properties = *newAnimTrackProperties;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerAnimTrack::SetupDefaults(UPopcornFXEffect *effect, const uint32 samplerIdx, bool updateUnlockedValues)
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

	const PopcornFX::CResourceDescriptor_AnimTrack *animTrack = PopcornFX::HBO::Cast<PopcornFX::CResourceDescriptor_AnimTrack>(defaultSampler.Get());
	if (animTrack != null)
	{
		if (updateUnlockedValues)
		{
			Properties.bTranslate = animTrack->TransformTranslate();
			Properties.bRotate = animTrack->TransformRotate();
			Properties.bScale = animTrack->TransformScale();
		}
	}
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerAnimTrack::ArePropertiesSupported()
{
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerAnimTrack::ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerAnimTrack::RebuildCurvesIFN()
{
	USplineComponent	*splineComponent = ResolveSplineComponent(true);
	if (splineComponent == null)
		return false;

	m_Data->m_CurrentSplineComponent = splineComponent;

	if (Properties.bFastSampler)
	{
		PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerAnimTrack::Setup (Fast)", POPCORNFX_UE_PROFILER_COLOR);

		if (m_Data->m_DescFast == null)
			return false; // Requires effect restart
		const FInterpCurveVector	&positions = splineComponent->GetSplinePointsPosition();
		const FInterpCurveVector	&scales = splineComponent->GetSplinePointsScale();

		// Build fast sampler
		const u32					keyCount = positions.Points.Num();
		const u32					realKeyCount = PopcornFX::PKMax(keyCount, 2U);
		PopcornFX::TArray<float>	times(realKeyCount);
		float						*dstTimes = times.RawDataPointer() + 1;
		const float					splineLength = splineComponent->GetSplineLength();

		times[0] = 0.0f;
		times[realKeyCount - 1] = 1.0f;
		for (u32 iPoint = 1; iPoint < keyCount - 1; ++iPoint)
			*dstTimes++ = splineComponent->GetDistanceAlongSplineAtSplinePoint(iPoint) / splineLength;
		const float	*srcTimes = times.RawDataPointer();
		const float	invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();

		{
#		define	REBUILD_CURVE(cond, curve, count)																\
				if (cond)																						\
				{																								\
					if (curve == null)																			\
						curve = PK_NEW(PopcornFX::CCurveDescriptor());											\
					if (!PK_VERIFY(curve != null))																\
						return false;																			\
					curve->m_Order = 3;																			\
					curve->m_Interpolator = PopcornFX::CInterpolableVectorArray::Interpolator_Hermite;			\
					if (!PK_VERIFY(curve->Resize(count)))														\
						return false;																			\
					PopcornFX::Mem::Copy(curve->m_Times.RawDataPointer(), srcTimes, sizeof(float) * count);		\
				}																								\
				else if (curve != null)																			\
					PK_SAFE_DELETE(curve);

			REBUILD_CURVE(Properties.bTranslate, m_Data->m_Positions, realKeyCount);
			REBUILD_CURVE(Properties.bScale, m_Data->m_Scales, realKeyCount);
		}

		const VectorRegister	invScale = MakeVectorRegister(invGlobalScale, invGlobalScale, invGlobalScale, invGlobalScale);

		// Positions curve
		if (Properties.bTranslate)
		{
			PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerAnimTrack::Setup (Fast) - Positions", POPCORNFX_UE_PROFILER_COLOR);

			CFloat3		*dstPos = reinterpret_cast<CFloat3*>(m_Data->m_Positions->m_FloatValues.RawDataPointer());
			CFloat3		*dstTangents = reinterpret_cast<CFloat3*>(m_Data->m_Positions->m_FloatTangents.RawDataPointer());
			const FInterpCurvePoint<FVector>	*srcPoints = positions.Points.GetData();
			for (u32 iPoint = 0; iPoint < keyCount; ++iPoint)
			{
				const CFloat3	pos = ToPk(srcPoints->OutVal) * invGlobalScale;
				const CFloat3	srcArriveTan = ToPk(srcPoints->ArriveTangent) * invGlobalScale;
				const CFloat3	srcLeaveTan = ToPk(srcPoints->LeaveTangent) * invGlobalScale;

				*dstPos++ = pos;
				*dstTangents++ = srcArriveTan;
				*dstTangents++ = srcLeaveTan;
				++srcPoints;
			}
			// Duplicate first and last values
			if (keyCount == 1)
			{
				PopcornFX::Mem::Copy(dstPos, dstPos - 1, sizeof(float) * 3);
				PopcornFX::Mem::Copy(dstTangents, dstTangents - 2, sizeof(float) * 6);
			}

			m_Data->m_Positions->RecomputeParametricDomain();
			if (!PK_VERIFY(m_Data->m_Positions->IsCoherent()))
				return false;
		}

		// Scales curve
		if (Properties.bScale)
		{
			PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerAnimTrack::Setup (Fast) - Scales", POPCORNFX_UE_PROFILER_COLOR);

			CFloat3		*dstPos = reinterpret_cast<CFloat3*>(m_Data->m_Scales->m_FloatValues.RawDataPointer());
			CFloat3		*dstTangents = reinterpret_cast<CFloat3*>(m_Data->m_Scales->m_FloatTangents.RawDataPointer());
			const FInterpCurvePoint<FVector>	*srcPoints = scales.Points.GetData();
			for (u32 iPoint = 0; iPoint < keyCount; ++iPoint)
			{
				*dstPos++ = ToPk(srcPoints->OutVal);
				*dstTangents++ = ToPk(srcPoints->ArriveTangent);
				*dstTangents++ = ToPk(srcPoints++->LeaveTangent);
			}
			// Duplicate first and last values
			if (keyCount == 1)
			{
				PopcornFX::Mem::Copy(dstPos, dstPos - 1, sizeof(float) * 3);
				PopcornFX::Mem::Copy(dstTangents, dstTangents - 2, sizeof(float) * 6);
			}

			m_Data->m_Scales->RecomputeParametricDomain();
			if (!PK_VERIFY(m_Data->m_Scales->IsCoherent()))
				return false;
		}

		// Rotations curve
		if (Properties.bRotate)
		{
#if WITH_EDITOR
			if (!Properties.bEditorRebuildEachFrame)
#endif // WITH_EDITOR
				UE_LOG(LogPopcornFXAttributeSamplerAnimTrack, Warning, TEXT("Rotations disabled for the AnimTrack fast sampler. Uncheck \"Fast sampler\" to enable exact rotations"));
		}

		// (Re)setup the descriptor
		m_Data->m_DescFast->m_Tracks.Clear();
		const PopcornFX::CGuid	id = m_Data->m_DescFast->m_Tracks.PushBack(PopcornFX::CParticleSamplerDescriptor_AnimTrack_Default::SPathDefinition(m_Data->m_Positions, null, m_Data->m_Scales, splineLength));
		PK_ASSERT(id.Valid());
		return id.Valid();
	}
	if (m_Data->m_Desc == null)
		return false; // Requires effect restart

	// Exact sampler
	u32	transformFlags = 0;
	if (Properties.bTranslate)
		transformFlags |= PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Translate;
	if (Properties.bRotate)
		transformFlags |= PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Rotate;
	if (Properties.bScale)
		transformFlags |= PopcornFX::CParticleSamplerDescriptor_AnimTrack::Transform_Scale;
	return m_Data->m_Desc->Setup(splineComponent, transformFlags);
}

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerAnimTrack::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	check(m_Data != null);
	const PopcornFX::CResourceDescriptor_AnimTrack	*defaultAnimTrackSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_AnimTrack>(defaultSampler);
	if (!PK_VERIFY(defaultAnimTrackSampler != null))
		return null;
	if (Properties.bFastSampler)
	{
		// Keep those around..
		// if (m_Data->m_Desc != null)
		// 	m_Data->m_Desc = null;
		if (m_Data->m_DescFast == null)
		{
			m_Data->m_DescFast = PK_NEW(PopcornFX::CParticleSamplerDescriptor_AnimTrack_Default());
			if (!PK_VERIFY(m_Data->m_DescFast != null))
				return null;
			m_Data->m_DescFast->m_TrackTransforms = reinterpret_cast<CFloat4x4*>(&(m_TrackTransforms));
			m_Data->m_DescFast->m_TrackTransformsUnscaled = reinterpret_cast<CFloat4x4*>(&(m_TrackTransformsUnscaled));
			m_Data->m_NeedsReload = true;
		}
	}
	else
	{
		// Keep those around..
		// {
		// 	PK_SAFE_DELETE(m_Data->m_Positions);
		// 	PK_SAFE_DELETE(m_Data->m_Scales);
		// 	m_Data->m_DescFast = null;
		// }
		if (m_Data->m_Desc == null)
		{
			m_Data->m_Desc = PK_NEW(CParticleSamplerDescriptor_AnimTrack_UE(&m_TrackTransforms));
			if (!PK_VERIFY(m_Data->m_Desc != null))
				return null;
			m_Data->m_NeedsReload = true;
		}
	}
	if (m_Data->m_NeedsReload)
	{
		if (!RebuildCurvesIFN())
			return null;

		m_Data->m_NeedsReload = false;
	}
	desc.m_NeedUpdate = true;
	_AttribSampler_PreUpdate(0.f);
	if (Properties.bFastSampler)
		return m_Data->m_DescFast.Get();
	return m_Data->m_Desc.Get();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerAnimTrack::_AttribSampler_PreUpdate(float deltaTime)
{
	check(m_Data != null);

	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerAnimTrack::_AttribSampler_PreUpdate", POPCORNFX_UE_PROFILER_COLOR);

#if WITH_EDITOR
	if (m_Data->m_Desc == null && m_Data->m_DescFast == null)
		return; // We shouldn't be here. This can occur in blueprints where components get registered/unregistered multiple times
#else
	if (!PK_VERIFY(m_Data->m_Desc != null || m_Data->m_DescFast != null))
		return;
#endif

	if (!Properties.bTranslate && !Properties.bRotate && !Properties.bScale)
		return;
	if (!m_Data->m_CurrentSplineComponent.IsValid())
		m_Data->m_CurrentSplineComponent = ResolveSplineComponent(false);

#if WITH_EDITOR
	if (Properties.bEditorRebuildEachFrame)
	{
		if (!RebuildCurvesIFN())
			return;
	}
#endif // WITH_EDITOR

	if (!m_Data->m_CurrentSplineComponent.IsValid() &&
		(Properties.Transforms == EPopcornFXSplineTransforms::SplineComponentRelativeTr ||
			Properties.Transforms == EPopcornFXSplineTransforms::SplineComponentWorldTr))
	{
		// No transforms update, keep previously created transforms, if any
		return;
	}

	switch (Properties.Transforms)
	{
		case	EPopcornFXSplineTransforms::SplineComponentRelativeTr:
			if (Properties.bScale || Properties.bFastSampler)
				m_TrackTransforms = (FMatrix44f)m_Data->m_CurrentSplineComponent->GetRelativeTransform().ToMatrixWithScale();
			m_TrackTransformsUnscaled = (FMatrix44f)m_Data->m_CurrentSplineComponent->GetRelativeTransform().ToMatrixNoScale();
			break;
		case	EPopcornFXSplineTransforms::SplineComponentWorldTr:
			if (Properties.bScale || Properties.bFastSampler)
				m_TrackTransforms = (FMatrix44f)m_Data->m_CurrentSplineComponent->GetComponentTransform().ToMatrixWithScale();
			m_TrackTransformsUnscaled = (FMatrix44f)m_Data->m_CurrentSplineComponent->GetComponentTransform().ToMatrixNoScale();
			break;
		case	EPopcornFXSplineTransforms::AttrSamplerRelativeTr:
			if (Properties.bScale || Properties.bFastSampler)
				m_TrackTransforms = (FMatrix44f)GetRelativeTransform().ToMatrixWithScale();
			m_TrackTransformsUnscaled = (FMatrix44f)GetRelativeTransform().ToMatrixNoScale();
			break;
		case	EPopcornFXSplineTransforms::AttrSamplerWorldTr:
			if (Properties.bScale || Properties.bFastSampler)
				m_TrackTransforms = (FMatrix44f)GetComponentTransform().ToMatrixWithScale();
			m_TrackTransformsUnscaled = (FMatrix44f)GetComponentTransform().ToMatrixNoScale();
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			break;
	}

	const float	invGlobalScale = FPopcornFXPlugin::GlobalScaleRcp();

	if (!Properties.bScale && !Properties.bFastSampler)
		m_TrackTransforms = m_TrackTransformsUnscaled;

	m_TrackTransforms.M[3][0] *= invGlobalScale;
	m_TrackTransforms.M[3][1] *= invGlobalScale;
	m_TrackTransforms.M[3][2] *= invGlobalScale;
}

#undef LOCTEXT_NAMESPACE
