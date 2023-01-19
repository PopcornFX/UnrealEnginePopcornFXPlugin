//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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

		// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
		const PopcornFX::CParticleAttributeDeclaration	*decl = static_cast<const PopcornFX::CParticleAttributeDeclaration*>(attrList->GetAttributeDeclaration(component->Effect, attri));
		if (!PK_VERIFY(decl != null))
			return false;

		PopcornFX::SAttributesContainer_SAttrib		attribValue = decl->GetDefaultValue();
		attrList->SetAttribute(attri, *reinterpret_cast<FPopcornFXAttributeValue*>(&attribValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
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

			// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
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
					attri, ANSI_TO_TCHAR(decl->ExportedName().Data()),
					ANSI_TO_TCHAR(inType), _Dim,
					ANSI_TO_TCHAR(attrType), dstAttrTraits.VectorDimension,
					*(component->GetPathName())
					);
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

		// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
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
		// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
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
					attri, ANSI_TO_TCHAR(decl->ExportedName().Data()),
					ANSI_TO_TCHAR(inType), _Dim,
					ANSI_TO_TCHAR(attrType), dstAttrTraits.VectorDimension,
					*(component->GetPathName())
					);
				return false;
			}
		}
#endif
		// Later versions will hide SAttributesContainer_SAttrib
		// Todo: template GetAttribute with target type
		PopcornFX::SAttributesContainer_SAttrib	out;
		attrList->GetAttribute(attri, *reinterpret_cast<FPopcornFXAttributeValue*>(&out)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
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

int32	UPopcornFXAttributeFunctions::FindAttributeIndex(const UPopcornFXEmitterComponent *InSelf, FName InAttributeName)
{
	if (!PK_VERIFY(InSelf != null) || !PK_VERIFY(InSelf->Effect != null))
		return -1;
	const UWorld	*world = InSelf->GetWorld();
	if (!FApp::CanEverRender() || (world != null && world->IsNetMode(NM_DedicatedServer)))
		return -1;
	UPopcornFXAttributeList		*attrList = InSelf->GetAttributeListIFP();
	if (!PK_VERIFY(attrList != null))
		return -1;
	return attrList->FindAttributeIndex(InAttributeName);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeFunctions::ResetToDefaultValue(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex)
{
	return _ResetAttribute(InSelf, InAttributeIndex);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeFunctions::SetAttributeAsFloat(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float InValue, bool InApplyGlobalScale)
{
	CFloat1		inValue(InValue);
	if (InApplyGlobalScale)
		inValue *= FPopcornFXPlugin::GlobalScaleRcp();
	return _SetAttribute(InSelf, InAttributeIndex, inValue);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsFloat(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float &OutValue, bool InApplyGlobalScale)
{
	CFloat1		outValue;
	if (_GetAttribute(InSelf, InAttributeIndex, outValue))
	{
		if (InApplyGlobalScale)
			outValue *= FPopcornFXPlugin::GlobalScale();
		OutValue = outValue.x();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsFloat2(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float InValueX, float InValueY, bool InApplyGlobalScale)
{
	CFloat2		inValue(InValueX, InValueY);

	if (InApplyGlobalScale)
		inValue *= FPopcornFXPlugin::GlobalScaleRcp();
	return _SetAttribute(InSelf, InAttributeIndex, inValue);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsVector2D(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FVector2D InValue, bool InApplyGlobalScale)
{
	return SetAttributeAsFloat2(InSelf, InAttributeIndex, InValue.X, InValue.Y, InApplyGlobalScale);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsFloat2(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float &OutValueX, float &OutValueY, bool InApplyGlobalScale)
{
	CFloat2		outValue;
	if (_GetAttribute(InSelf, InAttributeIndex, outValue))
	{
		if (InApplyGlobalScale)
			outValue *= FPopcornFXPlugin::GlobalScale();
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsVector2D(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FVector2D &OutValue, bool InApplyGlobalScale)
{
	float	outValues[2];
	if (!GetAttributeAsFloat2(InSelf, InAttributeIndex, outValues[0], outValues[1], InApplyGlobalScale))
		return false;
	OutValue.X = outValues[0];
	OutValue.Y = outValues[1];
	return true;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsFloat3(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float InValueX, float InValueY, float InValueZ, bool InApplyGlobalScale)
{
	CFloat3		inValue(InValueX, InValueY, InValueZ);

	if (InApplyGlobalScale)
		inValue *= FPopcornFXPlugin::GlobalScaleRcp();
	return _SetAttribute(InSelf, InAttributeIndex, inValue);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsVector(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FVector InValue, bool InApplyGlobalScale)
{
	return SetAttributeAsFloat3(InSelf, InAttributeIndex, InValue.X, InValue.Y, InValue.Z, InApplyGlobalScale);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsFloat3(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float &OutValueX, float &OutValueY, float &OutValueZ, bool InApplyGlobalScale)
{
	CFloat3		outValue;
	if (_GetAttribute(InSelf, InAttributeIndex, outValue))
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

bool	UPopcornFXAttributeFunctions::GetAttributeAsVector(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FVector &OutValue, bool InApplyGlobalScale)
{
	float	outValues[3];
	if (!GetAttributeAsFloat3(InSelf, InAttributeIndex, outValues[0], outValues[1], outValues[2], InApplyGlobalScale))
		return false;
	OutValue.X = outValues[0];
	OutValue.Y = outValues[1];
	OutValue.Z = outValues[2];
	return true;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsFloat4(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float InValueX, float InValueY, float InValueZ, float InValueW, bool InApplyGlobalScale)
{
	CFloat4		inValue(InValueX, InValueY, InValueZ, InValueW);

	if (InApplyGlobalScale)
		inValue *= FPopcornFXPlugin::GlobalScaleRcp();
	return _SetAttribute(InSelf, InAttributeIndex, inValue);
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsLinearColor(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FLinearColor InValue, bool InApplyGlobalScale)
{
	return SetAttributeAsFloat4(InSelf, InAttributeIndex, InValue.R, InValue.G, InValue.B, InValue.A, InApplyGlobalScale);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsFloat4(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, float &OutValueX, float &OutValueY, float &OutValueZ, float &OutValueW, bool InApplyGlobalScale)
{
	CFloat4		outValue;
	if (_GetAttribute(InSelf, InAttributeIndex, outValue))
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

bool	UPopcornFXAttributeFunctions::GetAttributeAsLinearColor(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FLinearColor &OutValue, bool InApplyGlobalScale)
{
	return GetAttributeAsFloat4(InSelf, InAttributeIndex, OutValue.R, OutValue.G, OutValue.B, OutValue.A, InApplyGlobalScale);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeFunctions::SetAttributeAsRotator(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FRotator InValue, bool InApplyGlobalScale)
{
	const FQuat		quat = InValue.Quaternion();
	const CFloat4	inValue(quat.X, quat.Y, quat.Z, quat.W); // Storage is a CQuaternion

	return _SetAttribute(InSelf, InAttributeIndex, inValue);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeFunctions::GetAttributeAsRotator(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, FRotator &OutValue, bool InApplyGlobalScale)
{
	CFloat4	outValue; // Storage is a CQuaternion

	if (GetAttributeAsFloat4(InSelf, InAttributeIndex, outValue.x(), outValue.y(), outValue.z(), outValue.w(), InApplyGlobalScale))
	{
		OutValue = FQuat(outValue.x(), outValue.y(), outValue.z(), outValue.w()).Rotator();
		return true;
	}

	return false;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeFunctions::SetAttributeAsInt(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 InValue)
{
	return _SetAttribute(InSelf, InAttributeIndex, CInt1(InValue));
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsInt(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 &OutValue)
{
	CInt1	outValue;
	if (_GetAttribute(InSelf, InAttributeIndex, outValue))
	{
		OutValue = outValue.x();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsInt2(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 InValueX, int32 InValueY)
{
	return _SetAttribute(InSelf, InAttributeIndex, CInt2(InValueX, InValueY));
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsInt2(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 &OutValueX, int32 &OutValueY)
{
	CInt2	outValue;
	if (_GetAttribute(InSelf, InAttributeIndex, outValue))
	{
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsInt3(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 InValueX, int32 InValueY, int32 InValueZ)
{
	return _SetAttribute(InSelf, InAttributeIndex, CInt3(InValueX, InValueY, InValueZ));
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsInt3(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ)
{
	CInt3	outValue;
	if (_GetAttribute(InSelf, InAttributeIndex, outValue))
	{
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		OutValueZ = outValue.z();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsInt4(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 InValueX, int32 InValueY, int32 InValueZ, int32 InValueW)
{
	return _SetAttribute(InSelf, InAttributeIndex, CInt4(InValueX, InValueY, InValueZ, InValueW));
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsInt4(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ, int32 &OutValueW)
{
	CInt4	outValue;
	if (_GetAttribute(InSelf, InAttributeIndex, outValue))
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

bool	UPopcornFXAttributeFunctions::SetAttributeAsBool(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool InValue)
{
	return _SetAttribute(InSelf, InAttributeIndex, CBool1(InValue), false);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsBool(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool &OutValue)
{
	CBool1	outValue;
	if (_GetAttribute(InSelf, InAttributeIndex, outValue))
	{
		OutValue = outValue.x();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsBool2(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool InValueX, bool InValueY)
{
	return _SetAttribute(InSelf, InAttributeIndex, CBool2(InValueX, InValueY), false);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsBool2(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool &OutValueX, bool &OutValueY)
{
	CBool2	outValue;
	if (_GetAttribute(InSelf, InAttributeIndex, outValue))
	{
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsBool3(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool InValueX, bool InValueY, bool InValueZ)
{
	return _SetAttribute(InSelf, InAttributeIndex, CBool3(InValueX, InValueY, InValueZ), false);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsBool3(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool &OutValueX, bool &OutValueY, bool &OutValueZ)
{
	CBool3	outValue;
	if (_GetAttribute(InSelf, InAttributeIndex, outValue))
	{
		OutValueX = outValue.x();
		OutValueY = outValue.y();
		OutValueZ = outValue.z();
		return true;
	}
	return false;
}

bool	UPopcornFXAttributeFunctions::SetAttributeAsBool4(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool InValueX, bool InValueY, bool InValueZ, bool InValueW)
{
	return _SetAttribute(InSelf, InAttributeIndex, CBool4(InValueX, InValueY, InValueZ, InValueW), false);
}

bool	UPopcornFXAttributeFunctions::GetAttributeAsBool4(UPopcornFXEmitterComponent *InSelf, int32 InAttributeIndex, bool &OutValueX, bool &OutValueY, bool &OutValueZ, bool &OutValueW)
{
	CBool4	outValue;
	if (_GetAttribute(InSelf, InAttributeIndex, outValue))
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
#undef LOCTEXT_NAMESPACE
