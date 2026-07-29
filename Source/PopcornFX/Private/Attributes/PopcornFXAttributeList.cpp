//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeList.h"

#include "PopcornFXPlugin.h"
#include "Internal/ParticleScene.h"
#include "Assets/PopcornFXEffect.h"
#include "Editor/EditorHelpers.h"
#include "PopcornFXEmitterComponent.h"
#include "PopcornFXAttributeSamplerShape.h"
#include "PopcornFXCustomVersion.h"
#include "Assets/PopcornFXEffectPriv.h"

#include "UObject/ObjectSaveContext.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_descriptor.h>
#include <pk_particles/include/ps_samplers_classes.h>
#include <pk_particles/include/ps_attributes.h>

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeList, Log, All);

//#define DBG_HERE()	UE_LOG(LogPopcornFXAttributeList, Log, TEXT("--- %20s --- %p --- %s"), ANSI_TO_TCHAR(__FUNCTION__), this, *GetFullName());
#define DBG_HERE()

//----------------------------------------------------------------------------
#if WITH_EDITOR
template float	FPopcornFXAttributeList::GetAttributeDim<float>(uint32, uint32);
template int32	FPopcornFXAttributeList::GetAttributeDim<int32>(uint32, uint32);
template void	FPopcornFXAttributeList::SetAttributeDim<float>(uint32, uint32, float, bool);
template void	FPopcornFXAttributeList::SetAttributeDim<int32>(uint32, uint32, int32, bool);
#endif // WITH_EDITOR

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

enum {
	kAttributeSize = sizeof(PopcornFX::SAttributesContainer_SAttrib)
};

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

// static
EPopcornFXAttributeSamplerType::Type	ResolveAttribSamplerType(const PopcornFX::CParticleAttributeSamplerDeclaration *attrSampler)
{
	if (PK_VERIFY(attrSampler != null))
	{
		switch (attrSampler->ExportedType())
		{
		case	PopcornFX::SParticleDeclaration::SSampler::Sampler_Animtrack:
			return EPopcornFXAttributeSamplerType::AnimTrack;
		case	PopcornFX::SParticleDeclaration::SSampler::Sampler_Curve:
			return EPopcornFXAttributeSamplerType::Curve;
		case	PopcornFX::SParticleDeclaration::SSampler::Sampler_Geometry:
			return EPopcornFXAttributeSamplerType::Shape;
		case	PopcornFX::SParticleDeclaration::SSampler::Sampler_Image:
			return EPopcornFXAttributeSamplerType::Image;
		case	PopcornFX::SParticleDeclaration::SSampler::Sampler_Grid:
			return EPopcornFXAttributeSamplerType::Grid;
		case	PopcornFX::SParticleDeclaration::SSampler::Sampler_Text:
			return EPopcornFXAttributeSamplerType::Text;
		case	PopcornFX::SParticleDeclaration::SSampler::Sampler_VectorField:
			return EPopcornFXAttributeSamplerType::VectorField;
		default:
			break;
		}
	}
	PK_ASSERT_NOT_REACHED();
	return EPopcornFXAttributeSamplerType::None;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

// Not a member of FPopcornFXAttributeDesc so PopcornFXAttributeList.h can be a public header
void	ResetAttribute(FPopcornFXAttributeDesc &attrib, const PopcornFX::CParticleAttributeDeclaration *decl)
{
	if (decl != null)
	{
		attrib.m_AttributeName = *ToUE(decl->ExportedName());
		attrib.m_AttributeType = decl->ExportedType();
		attrib.m_IsPrivate = decl->IsPrivate();

#if WITH_EDITOR
		PK_STATIC_ASSERT(EPopcornFXAttributeSemantic::AttributeSemantic_None			== (u32)PopcornFX::DataSemantic_None);
		PK_STATIC_ASSERT(EPopcornFXAttributeSemantic::AttributeSemantic_3DCoordinate	== (u32)PopcornFX::DataSemantic_3DCoordinate);
		PK_STATIC_ASSERT(EPopcornFXAttributeSemantic::AttributeSemantic_3DScale			== (u32)PopcornFX::DataSemantic_3DScale);
		PK_STATIC_ASSERT(EPopcornFXAttributeSemantic::AttributeSemantic_Color			== (u32)PopcornFX::DataSemantic_Color);
		attrib.m_AttributeSemantic = static_cast<EPopcornFXAttributeSemantic::Type>(decl->GetEffectiveDataSemantic());

		PK_STATIC_ASSERT(EPopcornFXAttributeDropDownMode::AttributeDropDownMode_None			== (u32)PopcornFX::EnumDropDown_None);
		PK_STATIC_ASSERT(EPopcornFXAttributeDropDownMode::AttributeDropDownMode_SingleSelect	== (u32)PopcornFX::EnumDropDown_SingleSelect);
		PK_STATIC_ASSERT(EPopcornFXAttributeDropDownMode::AttributeDropDownMode_MultiSelect		== (u32)PopcornFX::EnumDropDown_MultiSelect);
		attrib.m_DropDownMode = static_cast<EPopcornFXAttributeDropDownMode::Type>(decl->DropDownMode());

		attrib.m_EnumList.SetNum(decl->EnumList().Count());
		for (u32 i = 0; i < decl->EnumList().Count(); ++i)
			attrib.m_EnumList[i] = ToUE(decl->EnumList()[i]);

		if ((PopcornFX::EBaseTypeID)attrib.m_AttributeType == PopcornFX::BaseType_Quaternion)
		{
			const PopcornFX::SAttributesContainer_SAttrib		attribValue = decl->GetDefaultValue();
			const FRotator										rotator = FQuat(attribValue.m_Data32f[0], attribValue.m_Data32f[1], attribValue.m_Data32f[2], attribValue.m_Data32f[3]).Rotator();
			attrib.m_AttributeEulerAngles = FVector(rotator.Roll, rotator.Pitch, rotator.Yaw);
		}
#endif // WITH_EDITOR
		attrib.m_AttributeCategoryName = *ToUE(decl->CategoryName().MapDefault());
		if (attrib.m_AttributeCategoryName.IsEmpty())
			attrib.m_AttributeCategoryName = "General";
	}
	else
		attrib.Reset();
}

//----------------------------------------------------------------------------

// Not a member of FPopcornFXSamplerDesc so PopcornFXAttributeList.h can be a public header
void	ResetAttributeSampler(FPopcornFXSamplerDesc &desc, FPopcornFXSampler &attribSampler, const PopcornFX::CParticleAttributeSamplerDeclaration *decl)
{
	if (decl != null)
	{
		desc.m_SamplerName = *ToUE(decl->ExportedName());
		desc.m_SamplerType = ResolveAttribSamplerType(decl);
		switch (desc.m_SamplerType)
		{
		case EPopcornFXAttributeSamplerType::Type::Shape:
		{
			FPopcornFXAttributeSamplerShape newAttributeSamplerShape;
			newAttributeSamplerShape.SetName(desc.m_SamplerName);
			attribSampler.m_SamplerShape = newAttributeSamplerShape;

			FPopcornFXAttributeSamplerPropertiesShape	newSamplerShapeProperties;
#if WITH_EDITOR
			newSamplerShapeProperties.SetupDefaults(decl, true);
#endif
			desc.m_ShapeProperties = newSamplerShapeProperties;
			break;
		}
		case EPopcornFXAttributeSamplerType::Type::Image:
		{
			FPopcornFXAttributeSamplerImage newAttributeSamplerImage;
			newAttributeSamplerImage.SetName(desc.m_SamplerName);
			attribSampler.m_SamplerImage = newAttributeSamplerImage;

			FPopcornFXAttributeSamplerPropertiesImage newSamplerPropertiesImage;
#if WITH_EDITOR
			newSamplerPropertiesImage.SetupDefaults(decl, true);
#endif
			desc.m_ImageProperties = newSamplerPropertiesImage;
			break;
		}
		case EPopcornFXAttributeSamplerType::Type::Grid:
		{
			FPopcornFXAttributeSamplerGrid newAttributeSamplerGrid;
			newAttributeSamplerGrid.SetName(desc.m_SamplerName);
			attribSampler.m_SamplerGrid = newAttributeSamplerGrid;

			FPopcornFXAttributeSamplerPropertiesGrid newSamplerPropertiesGrid;
#if WITH_EDITOR
			newSamplerPropertiesGrid.SetupDefaults(decl, true);
#endif
			desc.m_GridProperties = newSamplerPropertiesGrid;
			break;
		}
		case EPopcornFXAttributeSamplerType::Type::Curve:
		{
			FPopcornFXAttributeSamplerCurve newAttributeSamplerCurve;
			newAttributeSamplerCurve.SetName(desc.m_SamplerName);
			attribSampler.m_SamplerCurve = newAttributeSamplerCurve;

			FPopcornFXAttributeSamplerPropertiesCurve newSamplerPropertiesCurve;
#if WITH_EDITOR
			newSamplerPropertiesCurve.SetupDefaults(decl, true);
#endif
			desc.m_CurveProperties = newSamplerPropertiesCurve;
			break;
		}
		case EPopcornFXAttributeSamplerType::Type::AnimTrack:
		{
			FPopcornFXAttributeSamplerAnimTrack newAttributeSamplerAnimTrack;
			newAttributeSamplerAnimTrack.SetName(desc.m_SamplerName);
			attribSampler.m_SamplerAnimTrack = newAttributeSamplerAnimTrack;

			FPopcornFXAttributeSamplerPropertiesAnimTrack newSamplerPropertiesAnimTrack;
#if WITH_EDITOR
			newSamplerPropertiesAnimTrack.SetupDefaults(decl, true);
#endif
			desc.m_AnimTrackProperties = newSamplerPropertiesAnimTrack;
			break;
		}
		case EPopcornFXAttributeSamplerType::Type::VectorField:
		{
			FPopcornFXAttributeSamplerVectorField newAttributeSamplerVectorField;
			newAttributeSamplerVectorField.SetName(desc.m_SamplerName);
			attribSampler.m_SamplerVectorField = newAttributeSamplerVectorField;

			FPopcornFXAttributeSamplerPropertiesVectorField newSamplerPropertiesVectorField;
#if WITH_EDITOR
			newSamplerPropertiesVectorField.SetupDefaults(decl, true);
#endif
			desc.m_VectorFieldProperties = newSamplerPropertiesVectorField;
			break;
		}
		case EPopcornFXAttributeSamplerType::Type::Text:
		{
			FPopcornFXAttributeSamplerText newAttributeSamplerText;
			newAttributeSamplerText.SetName(desc.m_SamplerName);
			attribSampler.m_SamplerText = newAttributeSamplerText;

			FPopcornFXAttributeSamplerPropertiesText newSamplerPropertiesText;
#if WITH_EDITOR
			newSamplerPropertiesText.SetupDefaults(decl, true);
#endif
			desc.m_TextProperties = newSamplerPropertiesText;
			break;
		}
		default:
			break;
		}
		desc.m_IsPrivate = decl->IsPrivate();

		desc.m_AttributeCategoryName = *ToUE(decl->CategoryName().MapDefault());
		if (desc.m_AttributeCategoryName.IsEmpty())
			desc.m_AttributeCategoryName = "General";
	}
	else
		desc.Reset();
}

//----------------------------------------------------------------------------

// Not a member of FPopcornFXAttributeList so PopcornFXAttributeList.h can be a public header
inline PopcornFX::TMemoryView<PopcornFX::SAttributesContainer_SAttrib>	AttributeRawDataAttributes(FPopcornFXAttributeList *attrList)
{
	PK_ASSERT(attrList != null);
	PK_ASSERT(attrList->CheckDataIntegrity());
	return PopcornFX::TMemoryView<PopcornFX::SAttributesContainer_SAttrib>(reinterpret_cast<PopcornFX::SAttributesContainer_SAttrib*>(attrList->m_AttributesRawData.GetData()), attrList->AttributeDescCount());
}

//----------------------------------------------------------------------------

// Not a member of FPopcornFXAttributeList so PopcornFXAttributeList.h can be a public header
inline const PopcornFX::TMemoryView<const PopcornFX::SAttributesContainer_SAttrib>	AttributeRawDataAttributesConst(const FPopcornFXAttributeList *attrList)
{
	PK_ASSERT(attrList != null);
	PK_ASSERT(attrList->CheckDataIntegrity());
	return PopcornFX::TMemoryView<const PopcornFX::SAttributesContainer_SAttrib>(reinterpret_cast<const PopcornFX::SAttributesContainer_SAttrib*>(attrList->m_AttributesRawData.GetData()), attrList->AttributeDescCount());
}

//----------------------------------------------------------------------------

uint32	FPopcornFXAttributeDesc::AttributeBaseTypeID() const
{
	PK_ASSERT(ValidAttributeType());
	return m_AttributeType;
}

//----------------------------------------------------------------------------

FPopcornFXAttributeSampler		*FPopcornFXAttributeList::ResolveAttributeSampler(int32 samplerId)
{
	if (!PK_VERIFY(samplerId < m_SamplerDescs.Num() && samplerId < m_Samplers.Num()))
		return nullptr;

	FPopcornFXAttributeSampler				*attributeSampler = nullptr;
	switch (m_SamplerDescs[samplerId].m_SamplerType)
	{
	case EPopcornFXAttributeSamplerType::Type::Shape:
	{
		attributeSampler = m_Samplers[samplerId].m_SamplerShape.GetPtrOrNull();
		break;
	}
	case EPopcornFXAttributeSamplerType::Type::Image:
	{
		attributeSampler = m_Samplers[samplerId].m_SamplerImage.GetPtrOrNull();
		break;
	}
	case EPopcornFXAttributeSamplerType::Type::Grid:
	{
		attributeSampler = m_Samplers[samplerId].m_SamplerGrid.GetPtrOrNull();
		break;
	}
	case EPopcornFXAttributeSamplerType::Type::Curve:
	{
		attributeSampler = m_Samplers[samplerId].m_SamplerCurve.GetPtrOrNull();
		break;
	}
	case EPopcornFXAttributeSamplerType::Type::AnimTrack:
	{
		attributeSampler = m_Samplers[samplerId].m_SamplerAnimTrack.GetPtrOrNull();
		break;
	}
	case EPopcornFXAttributeSamplerType::Type::VectorField:
	{
		attributeSampler = m_Samplers[samplerId].m_SamplerVectorField.GetPtrOrNull();
		break;
	}
	case EPopcornFXAttributeSamplerType::Type::Text:
	{
		attributeSampler = m_Samplers[samplerId].m_SamplerText.GetPtrOrNull();
		break;
	}
	default:
		PK_ASSERT_NOT_REACHED();
		break;
	}
	return attributeSampler;
}

//----------------------------------------------------------------------------

FPopcornFXAttributeSamplerProperties *FPopcornFXSamplerDesc::ResolveAttributeProperties()
{
	if (m_UseSamplerAsset)
	{
		if (m_SamplerAsset)
		{
			FPopcornFXAttributeSamplerProperties *properties = m_SamplerAsset->GetProperties();
			if (properties && properties->m_SamplerType == m_SamplerType)
				return properties;
			return nullptr;
		}
		return nullptr;
	}
	switch (m_SamplerType)
	{
	case EPopcornFXAttributeSamplerType::Type::Shape:
		return m_ShapeProperties.GetPtrOrNull();
	case EPopcornFXAttributeSamplerType::Type::Image:
		return m_ImageProperties.GetPtrOrNull();
	case EPopcornFXAttributeSamplerType::Type::Grid:
		return m_GridProperties.GetPtrOrNull();
	case EPopcornFXAttributeSamplerType::Type::Curve:
		return m_CurveProperties.GetPtrOrNull();
	case EPopcornFXAttributeSamplerType::Type::AnimTrack:
		return m_AnimTrackProperties.GetPtrOrNull();
	case EPopcornFXAttributeSamplerType::Type::VectorField:
		return m_VectorFieldProperties.GetPtrOrNull();
	case EPopcornFXAttributeSamplerType::Type::Text:
		return m_TextProperties.GetPtrOrNull();
	default:
		break;
	}
	return nullptr;
}

//----------------------------------------------------------------------------

const FPopcornFXAttributeSamplerProperties *FPopcornFXSamplerDesc::ResolveAttributeProperties() const
{
	if (m_UseSamplerAsset)
	{
		if (m_SamplerAsset)
			return m_SamplerAsset->GetProperties();
		return nullptr;
	}
	switch (m_SamplerType)
	{
	case EPopcornFXAttributeSamplerType::Type::Shape:
		return m_ShapeProperties.GetPtrOrNull();
	case EPopcornFXAttributeSamplerType::Type::Image:
		return m_ImageProperties.GetPtrOrNull();
	case EPopcornFXAttributeSamplerType::Type::Grid:
		return m_GridProperties.GetPtrOrNull();
	case EPopcornFXAttributeSamplerType::Type::Curve:
		return m_CurveProperties.GetPtrOrNull();
	case EPopcornFXAttributeSamplerType::Type::AnimTrack:
		return m_AnimTrackProperties.GetPtrOrNull();
	case EPopcornFXAttributeSamplerType::Type::VectorField:
		return m_VectorFieldProperties.GetPtrOrNull();
	case EPopcornFXAttributeSamplerType::Type::Text:
		return m_TextProperties.GetPtrOrNull();
	default:
		break;
	}
	return nullptr;
}


//----------------------------------------------------------------------------

void	FPopcornFXSamplerDesc::SetProperties(const FPopcornFXAttributeSamplerProperties *newProperties)
{
	switch (m_SamplerType)
	{
	case EPopcornFXAttributeSamplerType::Type::Shape:
	{
		const FPopcornFXAttributeSamplerPropertiesShape *shapeProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesShape *>(newProperties);
		m_ShapeProperties = *shapeProperties;
		break;
	}
	case EPopcornFXAttributeSamplerType::Type::Image:
	{
		const FPopcornFXAttributeSamplerPropertiesImage *imageProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesImage *>(newProperties);
		m_ImageProperties = *imageProperties;
		break;
	}
	case EPopcornFXAttributeSamplerType::Type::Grid:
	{
		const FPopcornFXAttributeSamplerPropertiesGrid *gridProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesGrid *>(newProperties);
		m_GridProperties = *gridProperties;
		break;
	}
	case EPopcornFXAttributeSamplerType::Type::Curve:
	{
		const FPopcornFXAttributeSamplerPropertiesCurve *curveProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesCurve *>(newProperties);
		m_CurveProperties = *curveProperties;
		break;
	}
	case EPopcornFXAttributeSamplerType::Type::AnimTrack:
	{
		const FPopcornFXAttributeSamplerPropertiesAnimTrack *animTrackProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesAnimTrack *>(newProperties);
		m_AnimTrackProperties = *animTrackProperties;
		break;
	}
	case EPopcornFXAttributeSamplerType::Type::VectorField:
	{
		const FPopcornFXAttributeSamplerPropertiesVectorField *vectorFieldProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesVectorField *>(newProperties);
		m_VectorFieldProperties = *vectorFieldProperties;
		break;
	}
	case EPopcornFXAttributeSamplerType::Type::Text:
	{
		const FPopcornFXAttributeSamplerPropertiesText *textProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesText *>(newProperties);
		m_TextProperties = *textProperties;
		break;
	}
	default:
		PK_ASSERT_NOT_REACHED();
		break;
	}
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

FPopcornFXAttributeList::FPopcornFXAttributeList()
:	m_FileVersionId(0)
,	m_Owner(null)
{
}

//----------------------------------------------------------------------------

bool	FPopcornFXAttributeList::CheckDataIntegrity() const
{
	bool	ok = true;

	ok &= PK_VERIFY(m_AttributesRawData.Num() == m_AttributeDescs.Num() * kAttributeSize);
	if (m_Owner != null && m_Owner->IsEmitterStarted())
	{
		PopcornFX::CParticleEffectInstance	*effectInstance = m_Owner->_GetEffectInstance();

		if (effectInstance->GetAllAttributes() == null)
			effectInstance->ResetAllAttributes();
		if (PK_VERIFY(effectInstance != null) && effectInstance->GetAllAttributes() != null)
		{
			const PopcornFX::SAttributesContainer	*instanceContainer = effectInstance->GetAllAttributes();

			ok &= PK_VERIFY(instanceContainer->AttributeCount() == m_AttributeDescs.Num());
			ok &= PK_VERIFY(instanceContainer->SamplerCount() == m_SamplerDescs.Num());
			ok &= PK_VERIFY(instanceContainer->SamplerCount() == m_Samplers.Num());
			ok &= PK_VERIFY((instanceContainer->AttributeCount()) * kAttributeSize == m_AttributesRawData.Num());
		}
	}
	return ok;
}

//----------------------------------------------------------------------------

bool	FPopcornFXAttributeList::Valid() const
{
	return m_Effect != null;
}

//----------------------------------------------------------------------------
typedef PopcornFX::TMemoryView<PopcornFX::CParticleAttributeDeclaration const *const> CMVAttributes;
typedef PopcornFX::TMemoryView<PopcornFX::CParticleAttributeSamplerDeclaration const *const> CMVSamplers;

bool	FPopcornFXAttributeList::IsUpToDate(UPopcornFXEffect *effect) const
{
	if (effect == null)
	{
		PK_ASSERT(AttributeDescCount() == 0);
		PK_ASSERT(SamplerCount() == 0);
		PK_ASSERT(SamplerDescCount() == 0);
		PK_ASSERT(m_AttributesRawData.Num() == 0);
		PK_ASSERT(CheckDataIntegrity());
		return m_Effect == null && m_FileVersionId == 0;
	}
	if (effect != m_Effect)
		return false;
	if (effect->FileVersionId() != m_FileVersionId)
		return false;
	const PopcornFX::PCParticleAttributeList &attrListPtr = effect->Effect()->AttributeList();
	if (attrListPtr == null || *(attrListPtr->DefaultAttributes()) == null)
	{
		return false;
	}
	const PopcornFX::CParticleAttributeList &attrList = *(attrListPtr.Get());

	const CMVAttributes		attrs = attrList.UniqueAttributeList();
	const CMVSamplers		samplers = attrList.UniqueSamplerList();
	const int32				attrCount = attrs.Count();
	const int32				samplerCount = samplers.Count();
	int32					privateAttrCount = 0;
	int32					localPrivateAttrCount = 0;
	int32					privateSamplerCount = 0;
	int32					localPrivateSamplerCount = 0;

	if (m_AttributeDescs.Num() != attrCount || m_SamplerDescs.Num() != samplerCount || m_Samplers.Num() != samplerCount)
		return false;
	for (int32 attri = 0; attri < attrCount; ++attri)
	{
		if (attrs[attri]->IsPrivate())
			privateAttrCount++;
	}
	for (int32 attri = 0; attri < m_AttributeDescs.Num(); ++attri)
	{
		if (m_AttributeDescs[attri].m_IsPrivate)
			localPrivateAttrCount++;
		if (!m_AttributeDescs[attri].ExactMatch(effect->DefaultAttributeList.m_AttributeDescs[attri]))
			return false;
	}
	if (privateAttrCount != localPrivateAttrCount)
		return false;

	for (int32 sampleri = 0; sampleri < samplerCount; ++sampleri)
	{
		if (samplers[sampleri]->IsPrivate())
			privateSamplerCount++;
	}
	for (int32 sampleri = 0; sampleri < m_SamplerDescs.Num(); ++sampleri)
	{
		if (m_SamplerDescs[sampleri].m_IsPrivate)
			localPrivateSamplerCount++;
		if (!m_SamplerDescs[sampleri].ExactMatch(effect->DefaultAttributeList.m_SamplerDescs[sampleri]))
			return false;
	}
	if (privateSamplerCount != localPrivateSamplerCount)
		return false;

	if (effect->IsTheDefaultAttributeList(this))
	{
		PK_ASSERT(CheckDataIntegrity());
		return true;
	}
	PK_ONLY_IF_ASSERTS(const FPopcornFXAttributeList		*defAttribs = &effect->DefaultAttributeList);
	PK_ASSERT(this != defAttribs); // checked with IsTheDefaultAttributeList
	PK_ASSERT(defAttribs->IsUpToDate(effect));
	PK_ASSERT(defAttribs->AttributeDescCount() == AttributeDescCount());
	PK_ASSERT(defAttribs->SamplerDescCount() == SamplerDescCount());
	PK_ASSERT(defAttribs->SamplerCount() == SamplerCount());
	PK_ASSERT(CheckDataIntegrity());
	// @TODO many other cases
	return true;
}

//----------------------------------------------------------------------------

FPopcornFXAttributeList::~FPopcornFXAttributeList()
{
}

//----------------------------------------------------------------------------

bool	FPopcornFXAttributeList::IsEmpty() const
{
	return m_AttributeDescs.Num() == 0 && m_SamplerDescs.Num() == 0 && m_Samplers.Num() == 0;
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::Clean()
{
	DBG_HERE();

	m_Effect = null;
	m_FileVersionId = 0;
	m_AttributeDescs.Empty(m_AttributeDescs.Num());
	m_SamplerDescs.Empty(m_SamplerDescs.Num());
	m_Samplers.Empty(m_Samplers.Num());
	m_AttributesRawData.Empty(m_AttributesRawData.Num());
	PK_ASSERT(CheckDataIntegrity());

#if WITH_EDITOR
	m_Categories.Empty();
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------------

int32	FPopcornFXAttributeList::FindAttributeIndex(const FString &name) const
{
	PK_ASSERT(CheckDataIntegrity());
	for (int32 attri = 0; attri < m_AttributeDescs.Num(); ++attri)
	{
		if (m_AttributeDescs[attri].m_AttributeName == name)
			return attri;
	}
	return -1;
}

//----------------------------------------------------------------------------

int32	FPopcornFXAttributeList::FindSamplerIndex(const FString &name) const
{
	PK_ASSERT(CheckDataIntegrity());
	for (int32 sampleri = 0; sampleri < m_SamplerDescs.Num(); ++sampleri)
	{
		if (m_SamplerDescs[sampleri].m_SamplerName == name)
			return sampleri;
	}
	return -1;
}

//----------------------------------------------------------------------------

FPopcornFXAttributeList		*FPopcornFXAttributeList::GetDefaultAttributeList(UPopcornFXEffect *effect) const
{
	if (!PK_VERIFY(IsUpToDate(effect)))
		return null;
	if (effect == null)
		return null;
	PK_ASSERT(CheckDataIntegrity());
	FPopcornFXAttributeList	*defAttribs = &effect->DefaultAttributeList;
	if (!PK_VERIFY(defAttribs != null))
		return null;
	return defAttribs;
}

//----------------------------------------------------------------------------

const FPopcornFXAttributeDesc	*FPopcornFXAttributeList::GetAttributeDesc(uint32 attributeId) const
{
	PK_ASSERT(CheckDataIntegrity());
	if (!PK_VERIFY(int32(attributeId) < m_AttributeDescs.Num()))
		return null;
	return &(m_AttributeDescs[attributeId]);
}

//----------------------------------------------------------------------------

const void	*FPopcornFXAttributeList::GetAttributeDeclaration(UPopcornFXEffect *effect, uint32 attributeId) const
{
	if (!PK_VERIFY(int32(attributeId) < m_AttributeDescs.Num()))
		return null;
	const FPopcornFXAttributeList				*defAttribs = GetDefaultAttributeList(effect);
	if (defAttribs == null)
		return null;
	const PopcornFX::CParticleAttributeList		*attrList = effect->Effect()->AttributeList().Get();
	if (!PK_VERIFY(attrList != null))
		return null;
	PK_ASSERT(attrList->UniqueAttributeList().Count() == m_AttributeDescs.Num());
	return attrList->UniqueAttributeList()[attributeId];
}

//----------------------------------------------------------------------------

FPopcornFXSamplerDesc	*FPopcornFXAttributeList::GetSamplerDesc(uint32 samplerId)
{
	PK_ASSERT(CheckDataIntegrity());
	if (!PK_VERIFY(int32(samplerId) < m_SamplerDescs.Num()))
		return null;
	return &(m_SamplerDescs[samplerId]);
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
const void	*FPopcornFXAttributeList::GetParticleSampler(UPopcornFXEffect *effect, uint32 samplerId) const
{
	PK_ASSERT(CheckDataIntegrity());
	if (!PK_VERIFY(int32(samplerId) < m_SamplerDescs.Num() && int32(samplerId) < m_Samplers.Num()))
		return null;
	const FPopcornFXAttributeList			*defAttribs = GetDefaultAttributeList(effect);
	if (defAttribs == null)
		return null;
	const PopcornFX::CParticleAttributeList		*attrList = effect->Effect()->AttributeList().Get();
	if (!PK_VERIFY(attrList != null))
		return null;
	PK_ASSERT(attrList->UniqueSamplerList().Count() == m_SamplerDescs.Num());
	PK_ASSERT(attrList->UniqueSamplerList().Count() == m_Samplers.Num());
	const PopcornFX::CParticleAttributeSamplerDeclaration	*decl = attrList->UniqueSamplerList()[samplerId];
	return decl;
}
#endif // WITH_EDITOR

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::SetupDefault(UPopcornFXEffect *effect, bool force)
{
	DBG_HERE();

	if (!force && IsUpToDate(effect))
	{
		return;
	}

	const uint32 oldAttributesCount = m_AttributeDescs.Num();

	if (effect == null)
	{
		Clean();
		return;
	}

	m_Effect = effect;
	m_FileVersionId = effect->FileVersionId();

#if WITH_EDITOR
	m_Categories.Empty();
#endif // WITH_EDITOR

	const PopcornFX::PCParticleAttributeList	&attrListPtr = effect->Effect()->AttributeList();
	if (attrListPtr == null || *(attrListPtr->DefaultAttributes()) == null)
	{
		Clean();
		m_Effect = effect;
		m_FileVersionId = effect->FileVersionId();
		return;
	}

	const PopcornFX::CParticleAttributeList		&attrList = *(attrListPtr.Get());

	const CMVAttributes		attrs = attrList.UniqueAttributeList();
	const CMVSamplers		samplers = attrList.UniqueSamplerList();
	const int32				attrCount = attrs.Count();
	const int32				samplerCount = samplers.Count();

	m_AttributeDescs.SetNum(attrCount);
	for (int32 attri = 0; attri < attrCount; ++attri)
	{
		ResetAttribute(m_AttributeDescs[attri], attrs[attri]);
#if WITH_EDITOR
		if (!m_AttributeDescs[attri].m_AttributeCategoryName.IsEmpty() && !m_AttributeDescs[attri].m_IsPrivate)
		{
			m_Categories.AddUnique(m_AttributeDescs[attri].m_AttributeCategoryName);
		}
#endif // WITH_EDITOR
	}

	m_SamplerDescs.SetNum(samplerCount);
	m_Samplers.SetNum(samplerCount);
	for (int32 sampleri = 0; sampleri < samplerCount; ++sampleri)
	{
		ResetAttributeSampler(m_SamplerDescs[sampleri], m_Samplers[sampleri], samplers[sampleri]);

#if WITH_EDITOR
		if (!m_SamplerDescs[sampleri].m_AttributeCategoryName.IsEmpty() && !m_SamplerDescs[sampleri].m_IsPrivate)
		{
			m_Categories.AddUnique(m_SamplerDescs[sampleri].m_AttributeCategoryName);
		}
#endif // WITH_EDITOR
	}

#if WITH_EDITOR
	if (attrCount > 0 || samplerCount > 0)
	{
		// Make "General" the first category if not already
		const int32	generalCategoryIndex = m_Categories.Find("General");
		if (generalCategoryIndex > 0)
			m_Categories.Swap(generalCategoryIndex, 0);
	}

#endif // WITH_EDITOR

	const PopcornFX::SAttributesContainer		*defContainer = *(attrList.DefaultAttributes());
	PK_ASSERT(defContainer != null);

	const uint32	attribBytes = defContainer->Attributes().CoveredBytes();
	m_AttributesRawData.SetNumUninitialized(attribBytes);

	if (attribBytes > 0 && (oldAttributesCount != attrCount))
		PopcornFX::Mem::Copy(m_AttributesRawData.GetData(), defContainer->Attributes().Data(), attribBytes);

	PK_ASSERT(CheckDataIntegrity());
}

//----------------------------------------------------------------------------

namespace
{
	void	CopyAttributeRawData(TArray<uint8> &dst, uint32 dstAttribIndex, const TArray<uint8> &src, uint32 srcAttribIndex)
	{
		reinterpret_cast<PopcornFX::SAttributesContainer_SAttrib&>(dst[dstAttribIndex * kAttributeSize]) = reinterpret_cast<const PopcornFX::SAttributesContainer_SAttrib&>(src[srcAttribIndex * kAttributeSize]);
	}

	void	SwapAttributeRawData(TArray<uint8> &dst, uint32 dstAttribIndex, TArray<uint8> &src, uint32 srcAttribIndex)
	{
		FMemory::Memswap(&dst[dstAttribIndex * kAttributeSize], &src[srcAttribIndex * kAttributeSize], kAttributeSize);
	}
} // namespace

//----------------------------------------------------------------------------

bool	FPopcornFXAttributeList::PrepareAttributes(TArray<FPopcornFXAttributeDesc> *attrs, const TArray<FPopcornFXAttributeDesc> *refAttrs, TArray<uint8> *rawData, const TArray<uint8> *refRawData)
{
	bool attrsChanged = false;
	if (attrs->Num() == 0)
	{
		attrsChanged = true;
		*attrs = *refAttrs;
		*rawData = *refRawData;
	}
	else
	{
		attrs->Reserve(refAttrs->Num());
		rawData->Reserve(refRawData->Num());

		int32		attri = 0;
		for (; attri < refAttrs->Num() && attri < attrs->Num(); ++attri)
		{
			const FPopcornFXAttributeDesc	&refAttr = (*refAttrs)[attri];
			FPopcornFXAttributeDesc			*attr = &((*attrs)[attri]);

			PK_ASSERT(refAttr.Valid());
			if (attr->m_AttributeName != refAttr.m_AttributeName ||
				attr->m_AttributeCategoryName != refAttr.m_AttributeCategoryName)
			{
				attrsChanged = true;
				PopcornFX::CGuid		found;
				for (int32 findAttri = attri + 1; findAttri < attrs->Num(); ++findAttri)
				{
					FPopcornFXAttributeDesc		&findAttr = (*attrs)[findAttri];
					if (findAttr.m_AttributeName == refAttr.m_AttributeName &&
						findAttr.m_AttributeCategoryName == refAttr.m_AttributeCategoryName)
					{
						found = findAttri;
						break;
					}
				}
				if (!found.Valid())
				{
					// (Can result to superfluous copies, but, here, dont care, and prefer simpler code)

					// Copy the refAttr/default to the back
					// and make it the found one...
					found = attrs->Add(refAttr);
					attr = &((*attrs)[attri]); // minds Add() realloc
					PK_ASSERT(rawData->Num() == found * kAttributeSize); // CheckDataIntegrity() should have asserted ? no ?
					rawData->SetNumUninitialized(rawData->Num() + 1 * kAttributeSize); // Push
					CopyAttributeRawData(*rawData, found, *refRawData, attri);
				}
				attrs->SwapMemory(attri, found);
				SwapAttributeRawData(*rawData, attri, *rawData, found);
				PK_ASSERT(attr->Valid());
				PK_ASSERT(attr->m_AttributeName == refAttr.m_AttributeName);
				PK_ASSERT(attr->m_AttributeCategoryName == refAttr.m_AttributeCategoryName);
			}

			if (attr->AttributeBaseTypeID() != refAttr.AttributeBaseTypeID())
			{
				attrsChanged = true;
				const PopcornFX::CBaseTypeTraits	&refTraits = PopcornFX::CBaseTypeTraits::Traits((PopcornFX::EBaseTypeID)refAttr.AttributeBaseTypeID());
				const PopcornFX::CBaseTypeTraits	&traits = PopcornFX::CBaseTypeTraits::Traits((PopcornFX::EBaseTypeID)attr->AttributeBaseTypeID());
				// If attribute changed float <-> int, reset value to default
				if (refTraits.ScalarType != traits.ScalarType)
				{
					CopyAttributeRawData(*rawData, attri, *refRawData, attri);
				}
				attr->m_AttributeType = refAttr.m_AttributeType;

#if	WITH_EDITOR
				if ((PopcornFX::EBaseTypeID)attr->m_AttributeType == PopcornFX::BaseType_Quaternion)
				{
					const PopcornFX::SAttributesContainer_SAttrib		attrib = AttributeRawDataAttributes(this)[attri];
					const FRotator										rotator = FQuat(attrib.m_Data32f[0], attrib.m_Data32f[1], attrib.m_Data32f[2], attrib.m_Data32f[3]).Rotator();
					attr->m_AttributeEulerAngles = FVector(rotator.Roll, rotator.Pitch, rotator.Yaw);
				}
#endif
			}
			if (attr->m_IsPrivate != refAttr.m_IsPrivate)
			{
				attr->m_IsPrivate = refAttr.m_IsPrivate;
			}
#if WITH_EDITOR
			if (attr->m_EnumList != refAttr.m_EnumList)
			{
				attr->m_EnumList = refAttr.m_EnumList;
			}
#endif

			PK_ASSERT(attr->ExactMatch(refAttr)); // Name, Category and Type must match now
		}

		// Removes removed attributes OR Sets new one to default
		attrs->SetNum(refAttrs->Num());
		rawData->SetNumUninitialized(refRawData->Num());
		for (; attri < refAttrs->Num(); ++attri)
		{
			const FPopcornFXAttributeDesc	&refAttr = (*refAttrs)[attri];
			FPopcornFXAttributeDesc			&attr = (*attrs)[attri];
			attr = refAttr;
			CopyAttributeRawData(*rawData, attri, *refRawData, attri);
		}
	}
	return attrsChanged;
}

//----------------------------------------------------------------------------

bool	FPopcornFXAttributeList::PrepareSamplers(TArray<FPopcornFXSamplerDesc> *samplerDescs, const TArray<FPopcornFXSamplerDesc> *refSamplerDescs, TArray<FPopcornFXSampler> *samplers, const TArray<FPopcornFXSampler> *refSamplers)
{
	bool	samplersChanged = false;

	// Re-match Samplers
	if (refSamplerDescs->Num() == 0 && refSamplers->Num() == 0)
	{
		samplersChanged |= !(samplerDescs->Num() == 0);
		samplersChanged |= !(samplers->Num() == 0);
		samplerDescs->Empty();
		samplers->Empty();
	}
	else if (samplerDescs->Num() == 0 && samplers->Num() == 0)
	{
		samplersChanged = true;
		*samplerDescs = *refSamplerDescs;
		*samplers = *refSamplers;
	}
	else
	{
		samplerDescs->Reserve(refSamplerDescs->Num());
		samplers->Reserve(refSamplers->Num());

		int32	sampleri = 0;
		for (; sampleri < refSamplerDescs->Num() && sampleri < samplerDescs->Num(); ++sampleri)
		{
			const FPopcornFXSamplerDesc	&refSamplerDesc = (*refSamplerDescs)[sampleri];
			FPopcornFXSamplerDesc		*samplerDesc = &((*samplerDescs)[sampleri]);

			const FPopcornFXSampler &refSampler = (*refSamplers)[sampleri];
			FPopcornFXSampler *sampler = &((*samplers)[sampleri]);

			if (samplerDesc->m_SamplerName != refSamplerDesc.m_SamplerName ||
				samplerDesc->m_AttributeCategoryName != refSamplerDesc.m_AttributeCategoryName)
			{
				samplersChanged = true;
				PopcornFX::CGuid		foundDesc;
				PopcornFX::CGuid		found;
				for (int32 foundSampleri = sampleri + 1; foundSampleri < samplerDescs->Num(); ++foundSampleri)
				{
					FPopcornFXSamplerDesc	&foundSampler = (*samplerDescs)[foundSampleri];
					if (foundSampler.m_SamplerName == refSamplerDesc.m_SamplerName &&
						foundSampler.m_AttributeCategoryName == refSamplerDesc.m_AttributeCategoryName)
					{
						foundDesc = foundSampleri;
						found = foundSampleri;
						break;
					}
				}
				if (!foundDesc.Valid())
				{
					// (Can result to superfluous copies, but, here, dont care, and prefer simpler code)
					foundDesc = samplerDescs->Add(refSamplerDesc);
					found = samplers->Add(refSampler);
					samplerDesc = &((*samplerDescs)[sampleri]); // minds Add() realloc
					sampler = &((*samplers)[sampleri]); // minds Add() realloc
				}
				samplerDescs->SwapMemory(sampleri, foundDesc);
				samplers->SwapMemory(sampleri, found);
				PK_ASSERT(samplerDesc->m_SamplerName == refSamplerDesc.m_SamplerName);
				PK_ASSERT(samplerDesc->m_AttributeCategoryName == refSamplerDesc.m_AttributeCategoryName);
			}

			// If type missmatch, just set to default (ResetValue())
			if (samplerDesc->SamplerType() != refSamplerDesc.SamplerType())
			{
				samplersChanged = true;
				*samplerDesc = refSamplerDesc;
				*sampler = refSampler;
			}

			if (samplerDesc->m_IsPrivate != refSamplerDesc.m_IsPrivate)
			{
				samplerDesc->m_IsPrivate = refSamplerDesc.m_IsPrivate;
			}

			PK_ASSERT(samplerDesc->ExactMatch(refSamplerDesc)); // Name, Category and Type must match now
		}

		samplerDescs->SetNum(refSamplerDescs->Num());
		samplers->SetNum(refSamplerDescs->Num());
		for (; sampleri < refSamplerDescs->Num(); ++sampleri)
		{
			const FPopcornFXSamplerDesc	&refSamplerDesc = (*refSamplerDescs)[sampleri];
			FPopcornFXSamplerDesc		&samplerDesc = (*samplerDescs)[sampleri];
			const FPopcornFXSampler		&refSampler = (*refSamplers)[sampleri];
			FPopcornFXSampler			&sampler = (*samplers)[sampleri];

			samplerDesc = refSamplerDesc;
			sampler = refSampler;
		}
	}
	return samplersChanged;
}

//----------------------------------------------------------------------------

bool	FPopcornFXAttributeList::Prepare(UPopcornFXEffect *effect, bool force)
{
	PK_NAMEDSCOPEDPROFILE_C("FPopcornFXAttributeList::Prepare", POPCORNFX_UE_PROFILER_COLOR);

	DBG_HERE();

	PK_ASSERT(CheckDataIntegrity());

	if (!force && IsUpToDate(effect))
	{
		return true;
	}

	if (effect == null)
	{
		Clean();
		return false;
	}

	const FPopcornFXAttributeList		*refAttrList = &effect->DefaultAttributeList;
	if (!PK_VERIFY(refAttrList != null)) // should not happen ?
	{
		Clean();
		m_Effect = effect;
		m_FileVersionId = 0; // try next time to reload
		return false;
	}

	if (m_Effect != effect && IsEmpty())
	{
		CopyFrom(refAttrList);
		return true;
	}

	PK_ASSERT(FPopcornFXPlugin::IsMainThread());

	m_Effect = effect;
	m_FileVersionId = effect->FileVersionId();

	TArray<FPopcornFXAttributeDesc>			*attrs = &m_AttributeDescs;
	TArray<uint8>							*rawData = &m_AttributesRawData;
	const TArray<uint8>						*refRawData = &refAttrList->m_AttributesRawData;

	bool	attrsChanged = false;

	// Re-match Attributes
	if (refRawData->Num() == 0)
	{
		attrsChanged |= !(attrs->Num() == 0);
		attrs->Empty();
		rawData->Empty();
	}
	else
	{
		attrsChanged |= PrepareAttributes(&m_AttributeDescs, &refAttrList->m_AttributeDescs, &m_AttributesRawData, &refAttrList->m_AttributesRawData);
	}

	bool	samplersChanged = false;
	samplersChanged |= PrepareSamplers(&m_SamplerDescs, &refAttrList->m_SamplerDescs, &m_Samplers, &refAttrList->m_Samplers);
	for (int32 sampleri = 0; sampleri < m_SamplerDescs.Num(); sampleri++)
	{
		ResolveAttributeSampler(sampleri)->RefreshFromProperties(m_SamplerDescs[sampleri].ResolveAttributeProperties());
	}

	PK_ASSERT(CheckDataIntegrity());

#if WITH_EDITOR
	if (attrsChanged || samplersChanged)
	{
		//ForceSetPackageDirty(m_Owner->GetOutermost());
	}
#endif

	return true;
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::CopyFrom(const FPopcornFXAttributeList *other)
{
	DBG_HERE();

	if (!PK_VERIFY(other != null))
	{
		Clean();
		return;
	}

	PK_ASSERT(other->CheckDataIntegrity());

	m_Effect = other->m_Effect;
	m_FileVersionId = other->m_FileVersionId;
	m_AttributeDescs = other->m_AttributeDescs;
	m_SamplerDescs = other->m_SamplerDescs;
	m_Samplers = other->m_Samplers;
	m_AttributesRawData = other->m_AttributesRawData;

	PK_ASSERT(CheckDataIntegrity());
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::RestoreAttributesFromCachedRawData(const TArray<uint8> &rawData)
{
	const u32	coveredBytes = rawData.Num();
	if (!PK_VERIFY(CheckDataIntegrity()) &&
		!PK_VERIFY(m_AttributesRawData.Num() == coveredBytes))
		return;
	PopcornFX::Mem::Copy(m_AttributesRawData.GetData(), rawData.GetData(), coveredBytes);

	if (m_Owner != null)
		_RefreshAttributes(m_Owner.Get());
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::ResetAllToDefaultValues(UPopcornFXEmitterComponent *emitter, UPopcornFXEffect *effect)
{
	DBG_HERE();

	if (!PK_VERIFY(Valid()))
		return;

	PK_ASSERT(CheckDataIntegrity());

	FPopcornFXAttributeList *defAttribs = GetDefaultAttributeList(effect);
	if (defAttribs == null)
		return;

	const TArray<uint8>			&defRawData = defAttribs->m_AttributesRawData;
	PK_ASSERT(m_AttributesRawData.Num() == defRawData.Num());
	m_AttributesRawData = defRawData;

	for (int32 i = 0; i < m_SamplerDescs.Num(); ++i)
	{
		m_SamplerDescs[i].CopyValuesFrom(*defAttribs->GetSamplerDesc(i));
		m_Samplers[i] = defAttribs->m_Samplers[i];
	}

	if (emitter)
	{
		_RefreshAttributes(emitter);
		_RefreshAttributeSamplers(emitter, false);
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::ResetAttributesToDefaultValues(UPopcornFXEmitterComponent *emitter, UPopcornFXEffect *effect)
{
	DBG_HERE();

	if (!PK_VERIFY(Valid()))
		return;

	PK_ASSERT(CheckDataIntegrity());

	const FPopcornFXAttributeList *defAttribs = GetDefaultAttributeList(effect);
	if (defAttribs == null)
		return;

	const TArray<uint8> &defRawData = defAttribs->m_AttributesRawData;
	PK_ASSERT(m_AttributesRawData.Num() == defRawData.Num());
	m_AttributesRawData = defRawData;

	if (emitter)
	{
		_RefreshAttributes(emitter);
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::Scene_PreUpdate(UPopcornFXEmitterComponent *emitter, float deltaTime)
{
	PK_NAMEDSCOPEDPROFILE_C("FPopcornFXAttributeList::Scene_PreUpdate", POPCORNFX_UE_PROFILER_COLOR);

	PK_ASSERT(emitter != null);
	PK_ASSERT(!m_Owner.IsValid() || emitter == m_Owner);
	m_Owner = emitter;

	for (int32 sampleri = 0; sampleri < m_SamplerDescs.Num(); ++sampleri)
	{
		FPopcornFXAttributeSampler		*attribSampler = ResolveAttributeSampler(sampleri);
		if (attribSampler != null && attribSampler->m_NeedUpdate)
		{
			attribSampler->_AttribSampler_PreUpdate(emitter, deltaTime);
		}
	}
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
void	FPopcornFXAttributeList::AttributeSamplers_IndirectSelectedThisTick(UPopcornFXEmitterComponent *emitter)
{
	for (int32 sampleri = 0; sampleri < m_SamplerDescs.Num(); ++sampleri)
	{
		FPopcornFXAttributeSampler		*attribSampler = ResolveAttributeSampler(sampleri);
		if (attribSampler != null)
			attribSampler->_AttribSampler_IndirectSelectedThisTick();
	}
}
#endif // WITH_EDITOR

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::GetAttribute(uint32 attributeId, FPopcornFXAttributeValue &outAttribute) const
{
#if WITH_EDITOR
	if (!PK_VERIFY(attributeId < (u32)m_AttributeDescs.Num()))
		return;
#else
	check(attributeId < (u32)m_AttributeDescs.Num());
#endif
	PopcornFX::SAttributesContainer_SAttrib	&_outAttribute = *reinterpret_cast<PopcornFX::SAttributesContainer_SAttrib*>(&outAttribute);

	const PopcornFX::EBaseTypeID	typeID = (PopcornFX::EBaseTypeID)m_AttributeDescs[attributeId].AttributeBaseTypeID();
	if (m_Owner.IsValid() && m_Owner->IsEmitterStarted())
	{
		PopcornFX::CParticleEffectInstance	*effectInstance = m_Owner->_GetEffectInstance();

		if (PK_VERIFY(effectInstance != null))
		{
			if (PK_VERIFY(effectInstance->GetRawAttribute(attributeId, typeID, &_outAttribute, true)))
				return;
			UE_LOG(LogPopcornFXAttributeList, Warning, TEXT("Couldn't get attribute %s on effect instance %p"), *m_AttributeDescs[attributeId].m_AttributeName, effectInstance);
		}
	}
	_outAttribute = AttributeRawDataAttributesConst(this)[attributeId];
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::SetAttribute(uint32 attributeId, const FPopcornFXAttributeValue &value, bool fromUI/* = false*/)
{
#if WITH_EDITOR
	if (!PK_VERIFY(attributeId < (u32)m_AttributeDescs.Num()))
		return;
	if (m_AttributeDescs[attributeId].m_IsPrivate)
		return;
	m_RestartEmitter |= fromUI && FPopcornFXPlugin::Get().SettingsEditor()->bRestartEmitterWhenAttributesChanged;
#else
	check(attributeId < (u32)m_AttributeDescs.Num());
#endif
	const PopcornFX::SAttributesContainer_SAttrib	&_value = *reinterpret_cast<const PopcornFX::SAttributesContainer_SAttrib*>(&value);

	const PopcornFX::EBaseTypeID	typeID = (PopcornFX::EBaseTypeID)m_AttributeDescs[attributeId].AttributeBaseTypeID();
	if (m_Owner.IsValid() && m_Owner->IsEmitterStarted())
	{
		PopcornFX::CParticleEffectInstance	*effectInstance = m_Owner->_GetEffectInstance();

		if (PK_VERIFY(effectInstance != null))
		{
			if (!PK_VERIFY(effectInstance->SetRawAttribute(attributeId, typeID, &_value, true)))
			{
				UE_LOG(LogPopcornFXAttributeList, Warning, TEXT("Couldn't set attribute %s on effect instance %p"), *m_AttributeDescs[attributeId].m_AttributeName, effectInstance);
			}
		}
	}
	AttributeRawDataAttributes(this)[attributeId] = _value;
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

template<typename _Scalar>
_Scalar	FPopcornFXAttributeList::GetAttributeDim(uint32 attributeId, uint32 dim)
{
	PopcornFX::SAttributesContainer_SAttrib	value;
	GetAttribute(attributeId, *reinterpret_cast<FPopcornFXAttributeValue*>(&value)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day

	return value.Get<_Scalar>()[dim];
}

//----------------------------------------------------------------------------

template<>
bool	FPopcornFXAttributeList::GetAttributeDim<bool>(uint32 attributeId, uint32 dim)
{
	PopcornFX::SAttributesContainer_SAttrib	value;
	GetAttribute(attributeId, *reinterpret_cast<FPopcornFXAttributeValue*>(&value)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day

	return reinterpret_cast<bool*>(value.Get<uint32>())[dim];
}

//----------------------------------------------------------------------------

float FPopcornFXAttributeList::GetAttributeQuaternionDim(uint32 attributeId, uint32 dim)
{
#if WITH_EDITOR
	if (!PK_VERIFY(attributeId < (u32)m_AttributeDescs.Num()))
		return 0.0f;
#else
	check(attributeId < (u32)m_AttributeDescs.Num());
#endif

	PopcornFX::SAttributesContainer_SAttrib	value;
	GetAttribute(attributeId, *reinterpret_cast<FPopcornFXAttributeValue*>(&value)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day

	const float	*scalarValue = value.Get<float>();
	const FQuat	quat = FQuat(scalarValue[0], scalarValue[1], scalarValue[2], scalarValue[3]);

	const FQuat storedQuat = FQuat(FRotator(m_AttributeDescs[attributeId].m_AttributeEulerAngles[1], m_AttributeDescs[attributeId].m_AttributeEulerAngles[2], m_AttributeDescs[attributeId].m_AttributeEulerAngles[0]));

	if (storedQuat != quat)
	{
		const FRotator	rotator = quat.Rotator();
		m_AttributeDescs[attributeId].m_AttributeEulerAngles = FVector(rotator.Roll, rotator.Pitch, rotator.Yaw);
	}

	return m_AttributeDescs[attributeId].m_AttributeEulerAngles[dim];
}

//----------------------------------------------------------------------------

void FPopcornFXAttributeList::SetAttributeQuaternionDim(uint32 attributeId, uint32 dim, float value, bool fromUI/* = false*/)
{
#if WITH_EDITOR
	if (!PK_VERIFY(attributeId < (u32)m_AttributeDescs.Num()))
		return;
#else
	check(attributeId < (u32)m_AttributeDescs.Num());
#endif

	FPopcornFXAttributeDesc *attributeDesc = &m_AttributeDescs[attributeId];
	attributeDesc->m_AttributeEulerAngles[dim] = value;

	const FVector			&eulerAngles = attributeDesc->m_AttributeEulerAngles;

	const FQuat				quaternion = FRotator(eulerAngles[1], eulerAngles[2], eulerAngles[0]).Quaternion();

	PopcornFX::SAttributesContainer_SAttrib	newValue = AttributeRawDataAttributes(this)[attributeId];
	newValue.m_Data32f[0] = quaternion.X;
	newValue.m_Data32f[1] = quaternion.Y;
	newValue.m_Data32f[2] = quaternion.Z;
	newValue.m_Data32f[3] = quaternion.W;

	SetAttribute(attributeId, *reinterpret_cast<FPopcornFXAttributeValue*>(&newValue), fromUI); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
}

//----------------------------------------------------------------------------

template<typename _Scalar>
void	FPopcornFXAttributeList::SetAttributeDim(uint32 attributeId, uint32 dim, _Scalar value, bool fromUI/* = false*/)
{
#if WITH_EDITOR
	if (!PK_VERIFY(attributeId < (u32)m_AttributeDescs.Num()))
		return;
#else
	check(attributeId < (u32)m_AttributeDescs.Num());
#endif

	PopcornFX::SAttributesContainer_SAttrib	newValue = AttributeRawDataAttributes(this)[attributeId];
	newValue.Get<_Scalar>()[dim] = value;

	SetAttribute(attributeId, *reinterpret_cast<FPopcornFXAttributeValue*>(&newValue), fromUI); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
}

//----------------------------------------------------------------------------

template <>
void	FPopcornFXAttributeList::SetAttributeDim<bool>(uint32 attributeId, uint32 dim, bool value, bool fromUI/* = false*/)
{
#if WITH_EDITOR
	if (!PK_VERIFY(attributeId < (u32)m_AttributeDescs.Num()))
		return;
#else
	check(attributeId < (u32)m_AttributeDescs.Num());
#endif

	PopcornFX::SAttributesContainer_SAttrib	newValue = AttributeRawDataAttributes(this)[attributeId];
	reinterpret_cast<bool*>(newValue.Get<uint32>())[dim] = value;

	SetAttribute(attributeId, *reinterpret_cast<FPopcornFXAttributeValue*>(&newValue), fromUI); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::PulseBoolAttributeDim(uint32 attributeId, uint32 dim, bool fromUI/* = false*/)
{
	SetAttributeDim<bool>(attributeId, dim, true, fromUI);
	m_HasPendingOneShotReset = true;
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::ResetPulsedBoolAttributesIFN()
{
	if (!m_HasPendingOneShotReset)	// Early-out most of the time
		return;
	m_HasPendingOneShotReset = false;

	if (!m_Owner.IsValid() /*&& m_Owner->IsEmitterStarted()*/)	// don't check if the emitter is started, even if stopped as long as particles are still here we want this logic to run
		return;
	PopcornFX::CParticleEffectInstance	*effectInstance = m_Owner->_GetEffectInstance();
	if (!PK_VERIFY(effectInstance != null))
		return;

	for (int32 i = 0; i < m_AttributeDescs.Num(); i++)
	{
		const PopcornFX::CParticleAttributeDeclaration	*decl = effectInstance->GetAttributeDecl(i);
		if (decl != null && decl->OneShotTrigger())
		{
			const PopcornFX::EBaseTypeID	typeID = PopcornFX::EBaseTypeID(decl->ExportedType());
			if (PK_VERIFY(PopcornFX::CBaseTypeTraits::Traits(typeID).ScalarType == PopcornFX::BaseType_Bool))	// OneShotTrigger but not a 'bool' type ?
			{
				// Do not call 'FPopcornFXAttributeList::SetAttribute()', we don't want the 'RestartEmitter' logic at all here !
				FPopcornFXAttributeValue						newValue = { 0, 0, 0, 0 };
				const PopcornFX::SAttributesContainer_SAttrib	&_value = *reinterpret_cast<const PopcornFX::SAttributesContainer_SAttrib*>(&newValue);	// FIXME: We don't actually care about FPopcornFXAttributeValue, add a way to ctor a new SAttrib in one line
				PK_VERIFY(effectInstance->SetRawAttribute(i, typeID, &_value, true));
				AttributeRawDataAttributes(this)[i] = _value;
			}
		}
	}
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::_RefreshAttributes(const UPopcornFXEmitterComponent *emitter)
{
	PK_NAMEDSCOPEDPROFILE_C("FPopcornFXAttributeList::_RefreshAttributes", POPCORNFX_UE_PROFILER_COLOR);

	DBG_HERE();

	PK_ASSERT(emitter != null);
	PK_ASSERT(!m_Owner.IsValid() || emitter == m_Owner);
	m_Owner = emitter;

	// If the emitter is running, set its attributes from serialized attributes
	if (!PK_VERIFY(m_Owner.IsValid()))
		return;
	PK_ASSERT(CheckDataIntegrity());

	if (!m_Owner->IsEmitterStarted())
		return;
	PopcornFX::CParticleEffectInstance	*effectInstance = m_Owner->_GetEffectInstance();
	if (!PK_VERIFY(effectInstance != null))
		return;

	const u32																attrCount = m_AttributeDescs.Num();
	const PopcornFX::TMemoryView<PopcornFX::SAttributesContainer_SAttrib>	attrValues = AttributeRawDataAttributes(this);

	PK_ASSERT(attrCount == attrValues.Count());
	for (u32 iAttr = 0; iAttr < attrCount; ++iAttr)
	{
		const FPopcornFXAttributeDesc					&attrDesc = m_AttributeDescs[iAttr];
		const PopcornFX::SAttributesContainer_SAttrib	&value = attrValues[iAttr];

		// First call to effectInstance->SetAttribute will lazy create the SAttributesContainer
		if (!PK_VERIFY(effectInstance->SetRawAttribute(iAttr, (PopcornFX::EBaseTypeID)attrDesc.AttributeBaseTypeID(), &value, true)))
		{
			UE_LOG(LogPopcornFXAttributeList, Warning, TEXT("Couldn't set attribute %s on effect instance %p"), *attrDesc.m_AttributeName, effectInstance);
		}
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::_RefreshAttributeSamplers(UPopcornFXEmitterComponent *emitter, bool reload)
{
	PK_NAMEDSCOPEDPROFILE_C("FPopcornFXAttributeList::_RefreshAttributeSamplers", POPCORNFX_UE_PROFILER_COLOR);

	PK_ASSERT(emitter != null);
	PK_ASSERT(!m_Owner.IsValid() || emitter == m_Owner);
	m_Owner = emitter;

	if (!PK_VERIFY(emitter != null))
		return;
	UPopcornFXEffect	*effect = emitter->Effect;
	Prepare(effect); // why needed?

	PK_ASSERT(IsUpToDate(effect));
	if (!PK_VERIFY(effect != null))
		return;

	if (m_SamplerDescs.Num() == 0 || m_Samplers.Num() == 0)
		return;
	if (!m_Owner.IsValid() || !m_Owner->IsEmitterStarted())
		return;
	PopcornFX::CParticleEffectInstance	*effectInstance = m_Owner->_GetEffectInstance();
	if (!PK_VERIFY(effectInstance != null))
		return;

	PK_ASSERT(CheckDataIntegrity());

	if (effectInstance->GetAllAttributes() != null &&
		!PK_VERIFY(m_SamplerDescs.Num() == effectInstance->GetAllAttributes()->Samplers().Count()
			&& m_Samplers.Num() == effectInstance->GetAllAttributes()->Samplers().Count()))
		return;
	const PopcornFX::PCParticleAttributeList	&attrListPtr = effect->Effect()->AttributeList();
	if (attrListPtr == null || *(attrListPtr->DefaultAttributes()) == null)
		return;
	const PopcornFX::SAttributesContainer *defCont = *(attrListPtr->DefaultAttributes());
	if (!PK_VERIFY(defCont != null && defCont->Samplers().Count() == m_SamplerDescs.Num() && defCont->Samplers().Count() == m_Samplers.Num()))
		return;

	bool atLeastOneInvalidSampler = false;
	for (int32 sampleri = 0; sampleri < m_SamplerDescs.Num(); ++sampleri)
	{
		PK_ASSERT(attrListPtr->UniqueSamplerList()[sampleri] != null);
		const PopcornFX::PResourceDescriptor	defaultSampler = attrListPtr->UniqueSamplerList()[sampleri]->AttribSamplerDefaultValue();
		if (!PK_VERIFY(defaultSampler != null))
			continue;

		FPopcornFXSamplerDesc &desc = m_SamplerDescs[sampleri];

		if (desc.SamplerType() == EPopcornFXAttributeSamplerType::None)
		{
			effectInstance->SetAttributeSampler(TCHAR_TO_UTF8(*desc.m_SamplerName), null);
			continue;
		}

		PK_ASSERT(desc.SamplerType() == ResolveAttribSamplerType(attrListPtr->UniqueSamplerList()[sampleri]));
		FPopcornFXAttributeSampler	*attribSampler = ResolveAttributeSampler(sampleri);
		if (attribSampler == null ||
			!PK_VERIFY(desc.SamplerType() == attribSampler->SamplerType()))
		{
			effectInstance->SetAttributeSampler(TCHAR_TO_UTF8(*desc.m_SamplerName), null);
			continue;
		}
		
		FPopcornFXAttributeSamplerProperties *properties = desc.ResolveAttributeProperties();
#if WITH_EDITOR
		if (properties != nullptr)
		{
			if (desc.m_UseSamplerAsset)
			{
				properties->m_EmitterSamplersUsingThis.FindOrAdd(emitter).m_SamplerNames.Add(desc.m_SamplerName);
			}
			else
			{
				properties->m_EmitterSamplersUsingThis.FindOrAdd(emitter).m_SamplerNames.Remove(desc.m_SamplerName);
				if (properties->m_EmitterSamplersUsingThis[emitter].m_SamplerNames.IsEmpty())
				{
					properties->m_EmitterSamplersUsingThis.Remove(emitter);
				}
			}
		}
#endif

		PopcornFX::CParticleSamplerDescriptor	*samplerDescriptor = null;

		attribSampler->m_NeedUpdate = false;
		// Dont setup samplers when cooking
		if (!IsRunningCommandlet() && attribSampler != null && properties != null)
			samplerDescriptor = attribSampler->_AttribSampler_SetupSampler(emitter, desc.m_SamplerName, properties, defaultSampler.Get());

		if (samplerDescriptor == null)
		{
			atLeastOneInvalidSampler = true;
		}
		effectInstance->SetAttributeSampler(TCHAR_TO_UTF8(*desc.m_SamplerName), samplerDescriptor != null ? samplerDescriptor : null);
	}

#if WITH_EDITOR
	if (atLeastOneInvalidSampler)
	{
		emitter->SetWarningSprite();
	}
	else
	{
		emitter->SetNormalSprite();
	}
	OnSamplersRefreshed.Broadcast();
#endif
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeList::CheckEmitter(const UPopcornFXEmitterComponent *emitter)
{
	check(emitter != null);
	m_Owner = emitter;
}

//----------------------------------------------------------------------------
