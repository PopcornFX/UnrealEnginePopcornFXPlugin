//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeFunctions.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXEmitterComponent.h"
#include "PopcornFXAttributeList.h"
#include "Engine/World.h"

#include "PopcornFXSDK.h"
#include <pk_maths/include/pk_maths.h>
#include <pk_kernel/include/kr_base_types.h>
#include <pk_particles/include/ps_attributes.h>
#include <pk_particles/include/ps_descriptor.h>

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeFunctions"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeFunctions, Log, All);

//----------------------------------------------------------------------------

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#	define	PKUE_ATTRIB_ENABLE_CHECKS		1
#else
#	define	PKUE_ATTRIB_ENABLE_CHECKS		0
#endif

//----------------------------------------------------------------------------

namespace
{
	template <typename _Scalar>
	struct IsFp
	{
		enum { Value = false };
	};

	template <>
	struct IsFp<float>
	{
		enum { Value = true };
	};

	bool			_ResetAttribute(UPopcornFXEmitterComponent *component, int32 _attri)
	{
		if (!PK_VERIFY(component != null))
			return false;
		const UWorld	*world = component->GetWorld();
		if (!FApp::CanEverRender() || (world != null && world->IsNetMode(NM_DedicatedServer)))
				return true;
		UPopcornFXAttributeList		*attrList = component->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return false;

		const u32	attri(_attri);
		if (attri >= attrList->AttributeCount())
		{
			UE_LOG(LogPopcornFXAttributeFunctions, Warning, TEXT("ResetToDefaultValue: invalid InAttributeIndex %d"), _attri);
			return false;
		}

		// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
		const PopcornFX::CParticleAttributeDeclaration	*decl = static_cast<const PopcornFX::CParticleAttributeDeclaration*>(attrList->GetAttributeDeclaration(component->Effect, attri));
		if (!PK_VERIFY(decl != null))
			return false;

		PopcornFX::SAttributesContainer_SAttrib		attribValue = decl->GetDefaultValue();
		attrList->SetAttribute(attri, *reinterpret_cast<FPopcornFXAttributeValue*>(&attribValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
		return true;
	}

	template <typename _Scalar, u32 _Dim>
	bool			_SetAttribute(UPopcornFXEmitterComponent *component, int32 _attri, const PopcornFX::TVector<_Scalar, _Dim> &value, bool clamp = true)
	{
		typedef PopcornFX::TVector<_Scalar, _Dim>		Vector;

		if (!PK_VERIFY(component != null))
			return false;
		const UWorld	*world = component->GetWorld();
		if (!FApp::CanEverRender() || (world != null && world->IsNetMode(NM_DedicatedServer)))
			return true;
		UPopcornFXAttributeList		*attrList = component->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return false;

		const u32	attri(_attri);
		if (attri >= attrList->AttributeCount())
		{
			UE_LOG(LogPopcornFXAttributeFunctions, Warning, TEXT("SetAttributeAs: invalid InAttributeIndex %d (%s)"), _attri, *(component->GetPathName()));
			return false;
		}

			// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
		const PopcornFX::CParticleAttributeDeclaration	*decl = static_cast<const PopcornFX::CParticleAttributeDeclaration*>(attrList->GetAttributeDeclaration(component->Effect, attri));
		if (!PK_VERIFY(decl != null))
			return false;

#if PKUE_ATTRIB_ENABLE_CHECKS
		const PopcornFX::CBaseTypeTraits	&dstAttrTraits = PopcornFX::CBaseTypeTraits::Traits((PopcornFX::EBaseTypeID)decl->ExportedType());
		const PopcornFX::CBaseTypeTraits	&srcTraits = PopcornFX::CBaseTypeTraits::Traits<_Scalar>();
		if (_Dim > dstAttrTraits.VectorDimension ||
			srcTraits.ScalarType != dstAttrTraits.ScalarType)
		{
			if (srcTraits.ScalarType != PopcornFX::BaseType_U32 ||
				dstAttrTraits.ScalarType != PopcornFX::BaseType_Bool)
			{
				const char			*inType = (srcTraits.ScalarType == PopcornFX::BaseType_U32 ? "Bool" : (srcTraits.IsFp ? "Float" : "Int"));
				const char			*attrType = (dstAttrTraits.ScalarType == PopcornFX::BaseType_Bool ? "Bool" : (dstAttrTraits.IsFp ? "Float" : "Int"));
				UE_LOG(LogPopcornFXAttributeFunctions, Warning,
					TEXT("SetAttributeAs: the Attribute [%d] \"%s\" cannot be set as %s %d: the attribute is %s %d (%s)"),
					attri, *ToUE(decl->ExportedName()),
					UTF8_TO_TCHAR(inType), _Dim,
					UTF8_TO_TCHAR(attrType), dstAttrTraits.VectorDimension,
					*(component->GetPathName()));
				return false;
			}
		}
#endif

		PopcornFX::SAttributesContainer_SAttrib	attribValue;
		attribValue.m_Data32u[0] = 0;
		attribValue.m_Data32u[1] = 0;
		attribValue.m_Data32u[2] = 0;
		attribValue.m_Data32u[3] = 0;
		PK_STATIC_ASSERT(sizeof(attribValue) >= sizeof(value));
		reinterpret_cast<Vector&>(attribValue) = value;

		if (clamp)
			decl->ClampToRangeIFN(attribValue);

		// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
		attrList->SetAttribute(attri, *reinterpret_cast<FPopcornFXAttributeValue*>(&attribValue));
		return true;
	}

	template <typename _Scalar, u32 _Dim>
	bool	_GetAttribute(UPopcornFXEmitterComponent *component, int32 _attri, PopcornFX::TVector<_Scalar, _Dim> &outValue)
	{
		typedef PopcornFX::TVector<_Scalar, _Dim>		Vector;

		if (!PK_VERIFY(component != null))
			return false;
		const UWorld	*world = component->GetWorld();
		if (!FApp::CanEverRender() || (world != null && world->IsNetMode(NM_DedicatedServer)))
			return true;
		UPopcornFXAttributeList		*attrList = component->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return false;

		const uint32	attri(_attri);
		if (attri >= attrList->AttributeCount())
		{
			UE_LOG(LogPopcornFXAttributeFunctions, Warning, TEXT("GetAttributeAs: invalid InAttributeIndex %d (%s)"), _attri, *(component->GetPathName()));
			return false;
		}

#if PKUE_ATTRIB_ENABLE_CHECKS
		// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
		const PopcornFX::CParticleAttributeDeclaration	*decl = static_cast<const PopcornFX::CParticleAttributeDeclaration*>(attrList->GetAttributeDeclaration(component->Effect, attri));
		if (!PK_VERIFY(decl != null))
			return false;
		const PopcornFX::CBaseTypeTraits	&dstAttrTraits = PopcornFX::CBaseTypeTraits::Traits((PopcornFX::EBaseTypeID)decl->ExportedType());
		const PopcornFX::CBaseTypeTraits	&srcTraits = PopcornFX::CBaseTypeTraits::Traits<_Scalar>();
		if (_Dim > dstAttrTraits.VectorDimension ||
			srcTraits.ScalarType != dstAttrTraits.ScalarType)
		{
			if (srcTraits.ScalarType != PopcornFX::BaseType_U32 ||
				dstAttrTraits.ScalarType != PopcornFX::BaseType_Bool)
			{
				const char	*inType = (srcTraits.ScalarType == PopcornFX::BaseType_U32 ? "Bool" : (srcTraits.IsFp ? "Float" : "Int"));
				const char	*attrType = (dstAttrTraits.ScalarType == PopcornFX::BaseType_Bool ? "Bool" : (dstAttrTraits.IsFp ? "Float" : "Int"));
				UE_LOG(LogPopcornFXAttributeFunctions, Warning,
					TEXT("GetAttributeAs: the Attribute [%d] \"%s\" cannot be get as %s %d: the attribute is %s %d (%s)"),
					attri, *ToUE(decl->ExportedName()),
					UTF8_TO_TCHAR(inType), _Dim,
					UTF8_TO_TCHAR(attrType), dstAttrTraits.VectorDimension,
					*(component->GetPathName()));
				return false;
			}
		}
#endif
		// Later versions will hide SAttributesContainer_SAttrib
		// Todo: template GetAttribute with target type
		PopcornFX::SAttributesContainer_SAttrib	out;
		attrList->GetAttribute(attri, *reinterpret_cast<FPopcornFXAttributeValue*>(&out)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
		outValue = reinterpret_cast<Vector&>(out);
		return true;
	}

} // namespace

//----------------------------------------------------------------------------

UPopcornFXAttributeFunctions::UPopcornFXAttributeFunctions(class FObjectInitializer const &pcip)
	: Super(pcip)
{
}

//----------------------------------------------------------------------------

int32	UPopcornFXAttributeFunctions::FindAttributeIndex(const UPopcornFXEmitterComponent *Emitter, FString InAttributeName)
{
	if (!PK_VERIFY(Emitter != null) || !PK_VERIFY(Emitter->Effect != null))
		return -1;
	const UWorld	*world = Emitter->GetWorld();
	if (!FApp::CanEverRender() || (world != null && world->IsNetMode(NM_DedicatedServer)))
		return -1;
	UPopcornFXAttributeList		*attrList = Emitter->GetAttributeListIFP();
	if (!PK_VERIFY(attrList != null))
		return -1;
	return attrList->FindAttributeIndex(InAttributeName);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeFunctions::ResetToDefaultValue(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex)
{
	return _ResetAttribute(Emitter, InAttributeIndex);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeFunctions::SetAttributeAsFloat(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float InValue, bool InApplyGlobalScale)
{
	CFloat1		inValue(InValue);
	if (InApplyGlobalScale)
		inValue *= FPopcornFXPlugin::GlobalScaleRcp();
	return _SetAttribute(Emitter, InAttributeIndex, inValue);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsFloat(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float &OutValue, bool InApplyGlobalScale)
{
	CFloat1		outValue;
	if (_GetAttribute(Emitter, InAttributeIndex, outValue))
	{
		if (InApplyGlobalScale)
			outValue *= FPopcornFXPlugin::GlobalScale();
		OutValue = outValue.x();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsFloat2(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float InValueX, float InValueY, bool InApplyGlobalScale)
{
	CFloat2		inValue(InValueX, InValueY);

	if (InApplyGlobalScale)
		inValue *= FPopcornFXPlugin::GlobalScaleRcp();
	return _SetAttribute(Emitter, InAttributeIndex, inValue);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsVector2D(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FVector2D InValue, bool InApplyGlobalScale)
{
	return SetAttributeAsFloat2(Emitter, InAttributeIndex, InValue.X, InValue.Y, InApplyGlobalScale);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsFloat2(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float &OutValueX, float &OutValueY, bool InApplyGlobalScale)
{
	CFloat2		outValue;
	if (_GetAttribute(Emitter, InAttributeIndex, outValue))
	{
		if (InApplyGlobalScale)
			outValue *= FPopcornFXPlugin::GlobalScale();
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsVector2D(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FVector2D &OutValue, bool InApplyGlobalScale)
{
	float	outValues[2];
	if (!GetAttributeAsFloat2(Emitter, InAttributeIndex, outValues[0], outValues[1], InApplyGlobalScale))
		return false;
	OutValue.X = outValues[0];
	OutValue.Y = outValues[1];
	return true;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsFloat3(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float InValueX, float InValueY, float InValueZ, bool InApplyGlobalScale)
{
	CFloat3		inValue(InValueX, InValueY, InValueZ);

	if (InApplyGlobalScale)
		inValue *= FPopcornFXPlugin::GlobalScaleRcp();
	return _SetAttribute(Emitter, InAttributeIndex, inValue);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsVector(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FVector InValue, bool InApplyGlobalScale)
{
	return SetAttributeAsFloat3(Emitter, InAttributeIndex, InValue.X, InValue.Y, InValue.Z, InApplyGlobalScale);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsFloat3(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float &OutValueX, float &OutValueY, float &OutValueZ, bool InApplyGlobalScale)
{
	CFloat3		outValue;
	if (_GetAttribute(Emitter, InAttributeIndex, outValue))
	{
		if (InApplyGlobalScale)
			outValue *= FPopcornFXPlugin::GlobalScale();
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		OutValueZ = outValue.z();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsVector(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FVector &OutValue, bool InApplyGlobalScale)
{
	float	outValues[3];
	if (!GetAttributeAsFloat3(Emitter, InAttributeIndex, outValues[0], outValues[1], outValues[2], InApplyGlobalScale))
		return false;
	OutValue.X = outValues[0];
	OutValue.Y = outValues[1];
	OutValue.Z = outValues[2];
	return true;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsFloat4(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float InValueX, float InValueY, float InValueZ, float InValueW, bool InApplyGlobalScale)
{
	CFloat4		inValue(InValueX, InValueY, InValueZ, InValueW);

	if (InApplyGlobalScale)
		inValue *= FPopcornFXPlugin::GlobalScaleRcp();
	return _SetAttribute(Emitter, InAttributeIndex, inValue);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsLinearColor(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FLinearColor InValue, bool InApplyGlobalScale)
{
	return SetAttributeAsFloat4(Emitter, InAttributeIndex, InValue.R, InValue.G, InValue.B, InValue.A, InApplyGlobalScale);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsFloat4(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, float &OutValueX, float &OutValueY, float &OutValueZ, float &OutValueW, bool InApplyGlobalScale)
{
	CFloat4		outValue;
	if (_GetAttribute(Emitter, InAttributeIndex, outValue))
	{
		if (InApplyGlobalScale)
			outValue *= FPopcornFXPlugin::GlobalScale();
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		OutValueZ = outValue.z();
		OutValueW = outValue.w();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsLinearColor(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FLinearColor &OutValue, bool InApplyGlobalScale)
{
	return GetAttributeAsFloat4(Emitter, InAttributeIndex, OutValue.R, OutValue.G, OutValue.B, OutValue.A, InApplyGlobalScale);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeFunctions::SetAttributeAsRotator(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FRotator InValue, bool InApplyGlobalScale)
{
	const FQuat		quat = InValue.Quaternion();
	const CFloat4	inValue(quat.X, quat.Y, quat.Z, quat.W); // Storage is a CQuaternion

	return _SetAttribute(Emitter, InAttributeIndex, inValue);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeFunctions::GetAttributeAsRotator(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, FRotator &OutValue, bool InApplyGlobalScale)
{
	CFloat4	outValue; // Storage is a CQuaternion

	if (GetAttributeAsFloat4(Emitter, InAttributeIndex, outValue.x(), outValue.y(), outValue.z(), outValue.w(), InApplyGlobalScale))
	{
		OutValue = FQuat(outValue.x(), outValue.y(), outValue.z(), outValue.w()).Rotator();
		return true;
	}

	return false;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeFunctions::SetAttributeAsInt(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 InValue)
{
	return _SetAttribute(Emitter, InAttributeIndex, CInt1(InValue));
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsInt(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 &OutValue)
{
	CInt1	outValue;
	if (_GetAttribute(Emitter, InAttributeIndex, outValue))
	{
		OutValue = outValue.x();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsInt2(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 InValueX, int32 InValueY)
{
	return _SetAttribute(Emitter, InAttributeIndex, CInt2(InValueX, InValueY));
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsInt2(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 &OutValueX, int32 &OutValueY)
{
	CInt2	outValue;
	if (_GetAttribute(Emitter, InAttributeIndex, outValue))
	{
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsInt3(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 InValueX, int32 InValueY, int32 InValueZ)
{
	return _SetAttribute(Emitter, InAttributeIndex, CInt3(InValueX, InValueY, InValueZ));
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsInt3(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ)
{
	CInt3	outValue;
	if (_GetAttribute(Emitter, InAttributeIndex, outValue))
	{
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		OutValueZ = outValue.z();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsInt4(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 InValueX, int32 InValueY, int32 InValueZ, int32 InValueW)
{
	return _SetAttribute(Emitter, InAttributeIndex, CInt4(InValueX, InValueY, InValueZ, InValueW));
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsInt4(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ, int32 &OutValueW)
{
	CInt4	outValue;
	if (_GetAttribute(Emitter, InAttributeIndex, outValue))
	{
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		OutValueZ = outValue.z();
		OutValueW = outValue.w();
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeFunctions::SetAttributeAsBool(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool InValue)
{
	return _SetAttribute(Emitter, InAttributeIndex, CBool1(InValue), false);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsBool(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool &OutValue)
{
	CBool1	outValue;
	if (_GetAttribute(Emitter, InAttributeIndex, outValue))
	{
		OutValue = outValue.x();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsBool2(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool InValueX, bool InValueY)
{
	return _SetAttribute(Emitter, InAttributeIndex, CBool2(InValueX, InValueY), false);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsBool2(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool &OutValueX, bool &OutValueY)
{
	CBool2	outValue;
	if (_GetAttribute(Emitter, InAttributeIndex, outValue))
	{
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsBool3(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool InValueX, bool InValueY, bool InValueZ)
{
	return _SetAttribute(Emitter, InAttributeIndex, CBool3(InValueX, InValueY, InValueZ), false);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsBool3(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool &OutValueX, bool &OutValueY, bool &OutValueZ)
{
	CBool3	outValue;
	if (_GetAttribute(Emitter, InAttributeIndex, outValue))
	{
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		OutValueZ = outValue.z();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsBool4(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool InValueX, bool InValueY, bool InValueZ, bool InValueW)
{
	return _SetAttribute(Emitter, InAttributeIndex, CBool4(InValueX, InValueY, InValueZ, InValueW), false);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsBool4(UPopcornFXEmitterComponent *Emitter, int32 InAttributeIndex, bool &OutValueX, bool &OutValueY, bool &OutValueZ, bool &OutValueW)
{
	CBool4	outValue;
	if (_GetAttribute(Emitter, InAttributeIndex, outValue))
	{
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		OutValueZ = outValue.z();
		OutValueW = outValue.w();
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------
// Same functions but finding attribute by name instead of by index

bool	UPopcornFXAttributeFunctions::SetAttributeAsFloatByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float InValue, bool InApplyGlobalScale)
{
	return SetAttributeAsFloat(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValue, InApplyGlobalScale);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsFloatByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float &OutValue, bool InApplyGlobalScale)
{
	return GetAttributeAsFloat(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValue, InApplyGlobalScale);
}
bool	UPopcornFXAttributeFunctions::SetAttributeAsFloat2ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float InValueX, float InValueY, bool InApplyGlobalScale)
{
	return SetAttributeAsFloat2(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValueX, InValueY, InApplyGlobalScale);
}
bool	UPopcornFXAttributeFunctions::SetAttributeAsVector2DByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FVector2D InValue, bool InApplyGlobalScale)
{
	return SetAttributeAsVector2D(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValue, InApplyGlobalScale);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsFloat2ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float &OutValueX, float &OutValueY, bool InApplyGlobalScale)
{
	return GetAttributeAsFloat2(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValueX, OutValueY, InApplyGlobalScale);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsVector2DByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FVector2D &OutValue, bool InApplyGlobalScale)
{
	return GetAttributeAsVector2D(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValue, InApplyGlobalScale);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsFloat3ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float InValueX, float InValueY, float InValueZ, bool InApplyGlobalScale)
{
	return SetAttributeAsFloat3(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValueX, InValueY, InValueZ, InApplyGlobalScale);
}
bool	UPopcornFXAttributeFunctions::SetAttributeAsVectorByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FVector InValue, bool InApplyGlobalScale)
{
	return SetAttributeAsVector(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValue, InApplyGlobalScale);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsFloat3ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float &OutValueX, float &OutValueY, float &OutValueZ, bool InApplyGlobalScale)
{
	return GetAttributeAsFloat3(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValueX, OutValueY, OutValueZ, InApplyGlobalScale);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsVectorByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FVector &OutValue, bool InApplyGlobalScale)
{
	return GetAttributeAsVector(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValue, InApplyGlobalScale);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsFloat4ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float InValueX, float InValueY, float InValueZ, float InValueW, bool InApplyGlobalScale)
{
	return SetAttributeAsFloat4(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValueX, InValueY, InValueZ, InValueW, InApplyGlobalScale);
}
bool	UPopcornFXAttributeFunctions::SetAttributeAsLinearColorByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FLinearColor InValue, bool InApplyGlobalScale)
{
	return SetAttributeAsLinearColor(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValue, InApplyGlobalScale);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsFloat4ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, float &OutValueX, float &OutValueY, float &OutValueZ, float &OutValueW, bool InApplyGlobalScale)
{
	return GetAttributeAsFloat4(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValueX, OutValueY, OutValueZ, OutValueW, InApplyGlobalScale);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsLinearColorByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FLinearColor &OutValue, bool InApplyGlobalScale)
{
	return GetAttributeAsLinearColor(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValue, InApplyGlobalScale);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsRotatorByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FRotator InValue, bool InApplyGlobalScale)
{
	return SetAttributeAsRotator(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValue, InApplyGlobalScale);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsRotatorByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, FRotator &OutValue, bool InApplyGlobalScale)
{
	return GetAttributeAsRotator(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValue, InApplyGlobalScale);
}


bool	UPopcornFXAttributeFunctions::SetAttributeAsIntByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 InValue)
{
	return SetAttributeAsInt(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValue);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsIntByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 &OutValue)
{
	return GetAttributeAsInt(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValue);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsInt2ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 InValueX, int32 InValueY)
{
	return SetAttributeAsInt2(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValueX, InValueY);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsInt2ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 &OutValueX, int32 &OutValueY)
{
	return GetAttributeAsInt2(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValueX, OutValueY);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsInt3ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 InValueX, int32 InValueY, int32 InValueZ)
{
	return SetAttributeAsInt3(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValueX, InValueY, InValueZ);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsInt3ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ)
{
	return GetAttributeAsInt3(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValueX, OutValueY, OutValueZ);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsInt4ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 InValueX, int32 InValueY, int32 InValueZ, int32 InValueW)
{
	return SetAttributeAsInt4(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValueX, InValueY, InValueZ, InValueW);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsInt4ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ, int32 &OutValueW)
{
	return GetAttributeAsInt4(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValueX, OutValueY, OutValueZ, OutValueW);
}


bool	UPopcornFXAttributeFunctions::SetAttributeAsBoolByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool InValue)
{
	return SetAttributeAsBool(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValue);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsBoolByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool &OutValue)
{
	return GetAttributeAsBool(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValue);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsBool2ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool InValueX, bool InValueY)
{
	return SetAttributeAsBool2(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValueX, InValueY);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsBool2ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool &OutValueX, bool &OutValueY)
{
	return GetAttributeAsBool2(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValueX, OutValueY);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsBool3ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool InValueX, bool InValueY, bool InValueZ)
{
	return SetAttributeAsBool3(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValueX, InValueY, InValueZ);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsBool3ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool &OutValueX, bool &OutValueY, bool &OutValueZ)
{
	return GetAttributeAsBool3(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValueX, OutValueY, OutValueZ);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsBool4ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool InValueX, bool InValueY, bool InValueZ, bool InValueW)
{
	return SetAttributeAsBool4(Emitter, FindAttributeIndex(Emitter, InAttributeName), InValueX, InValueY, InValueZ, InValueW);
}
bool	UPopcornFXAttributeFunctions::GetAttributeAsBool4ByName(UPopcornFXEmitterComponent *Emitter, FString InAttributeName, bool &OutValueX, bool &OutValueY, bool &OutValueZ, bool &OutValueW)
{
	return GetAttributeAsBool4(Emitter, FindAttributeIndex(Emitter, InAttributeName), OutValueX, OutValueY, OutValueZ, OutValueW);
}

//----------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE