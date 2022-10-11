//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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

#if (ENGINE_MAJOR_VERSION == 5)
#include "UObject/ObjectSaveContext.h"
#endif // (ENGINE_MAJOR_VERSION == 5)

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
template float	UPopcornFXAttributeList::GetAttributeDim<float>(uint32, uint32);
template int32	UPopcornFXAttributeList::GetAttributeDim<int32>(uint32, uint32);
template void	UPopcornFXAttributeList::SetAttributeDim<float>(uint32, uint32, float);
template void	UPopcornFXAttributeList::SetAttributeDim<int32>(uint32, uint32, int32);
#endif // WITH_EDITOR

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

enum {
	kAttributeSize = sizeof(PopcornFX::SAttributesContainer_SAttrib)
};

using PopcornFX::CShapeDescriptor;
using PopcornFX::PShapeDescriptor;
using PopcornFX::PCShapeDescriptor;

uint32		ToPkShapeType(EPopcornFXAttribSamplerShapeType::Type ueShapeType)
{
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Box			== (u32)CShapeDescriptor::ShapeBox);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Sphere		== (u32)CShapeDescriptor::ShapeSphere);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Ellipsoid	== (u32)CShapeDescriptor::ShapeEllipsoid);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Cylinder		== (u32)CShapeDescriptor::ShapeCylinder);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Capsule		== (u32)CShapeDescriptor::ShapeCapsule);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Cone			== (u32)CShapeDescriptor::ShapeCone);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Mesh			== (u32)CShapeDescriptor::ShapeMesh);
	//PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Collection	== (u32)CShapeDescriptor::ShapeCollection);
	return static_cast<CShapeDescriptor::EShapeType>(ueShapeType);
}

namespace
{
	const char		*_ResolveAttribSamplerShapeNodeName(CShapeDescriptor::EShapeType shapeType)
	{
		switch (shapeType)
		{
		case PopcornFX::CShapeDescriptor::ShapeBox:
			return "CShapeDescriptor_Box";
		case PopcornFX::CShapeDescriptor::ShapeSphere:
			return "CShapeDescriptor_Sphere";
		case PopcornFX::CShapeDescriptor::ShapeEllipsoid:
			return "CShapeDescriptor_Ellipsoid";
		case PopcornFX::CShapeDescriptor::ShapeCylinder:
			return "CShapeDescriptor_Cylinder";
		case PopcornFX::CShapeDescriptor::ShapeCapsule:
			return "CShapeDescriptor_Capsule";
		case PopcornFX::CShapeDescriptor::ShapeCone:
			return "CShapeDescriptor_Cone";
		case PopcornFX::CShapeDescriptor::ShapeMesh:
			return "CShapeDescriptor_Mesh";
		case PopcornFX::CShapeDescriptor::ShapeCollection:
			return "CShapeDescriptor_Collection";
		default:
			break;
		}
		PK_ASSERT_NOT_REACHED();
		return "CParticleSamplerShape";
	}
}

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
		case	PopcornFX::SParticleDeclaration::SSampler::Sampler_Text:
			return EPopcornFXAttributeSamplerType::Text;
		case	PopcornFX::SParticleDeclaration::SSampler::Sampler_VectorField:
			return EPopcornFXAttributeSamplerType::Turbulence;
		default:
			break;
		}
	}
	PK_ASSERT_NOT_REACHED();
	return EPopcornFXAttributeSamplerType::None;
}

//----------------------------------------------------------------------------

const char		*ResolveAttribSamplerShapeNodeName(EPopcornFXAttribSamplerShapeType::Type shapeType)
{
	return _ResolveAttribSamplerShapeNodeName((CShapeDescriptor::EShapeType)ToPkShapeType(shapeType));
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
		attrib.m_AttributeFName = *FString(ANSI_TO_TCHAR(decl->ExportedName().Data()));
		attrib.m_AttributeType = decl->ExportedType();

#if WITH_EDITOR
		PK_STATIC_ASSERT(EPopcornFXAttributeSemantic::AttributeSemantic_None			== (u32)PopcornFX::EDataSemantic::DataSemantic_None);
		PK_STATIC_ASSERT(EPopcornFXAttributeSemantic::AttributeSemantic_3DCoordinate	== (u32)PopcornFX::EDataSemantic::DataSemantic_3DCoordinate);
		PK_STATIC_ASSERT(EPopcornFXAttributeSemantic::AttributeSemantic_3DScale			== (u32)PopcornFX::EDataSemantic::DataSemantic_3DScale);
		PK_STATIC_ASSERT(EPopcornFXAttributeSemantic::AttributeSemantic_Color			== (u32)PopcornFX::EDataSemantic::DataSemantic_Color);

		attrib.m_AttributeSemantic = static_cast<EPopcornFXAttributeSemantic::Type>(decl->GetEffectiveDataSemantic());

		if ((PopcornFX::EBaseTypeID)attrib.m_AttributeType == PopcornFX::EBaseTypeID::BaseType_Quaternion)
		{
			const PopcornFX::SAttributesContainer_SAttrib		attribValue = decl->GetDefaultValue();
			const FRotator										rotator = FQuat(attribValue.m_Data32f[0], attribValue.m_Data32f[1], attribValue.m_Data32f[2], attribValue.m_Data32f[3]).Rotator();
			attrib.m_AttributeEulerAngles = FVector(rotator.Roll, rotator.Pitch, rotator.Yaw);
		}
#endif // WITH_EDITOR
		attrib.m_AttributeCategoryName = *FString((const UTF16CHAR*)decl->CategoryName().MapDefault().Data());
		if (!attrib.m_AttributeCategoryName.IsValid() || attrib.m_AttributeCategoryName.IsNone())
			attrib.m_AttributeCategoryName = "General";
	}
	else
		attrib.Reset();
}

//----------------------------------------------------------------------------

// Not a member of FPopcornFXSamplerDesc so PopcornFXAttributeList.h can be a public header
void	ResetAttributeSampler(FPopcornFXSamplerDesc &attribSampler, const PopcornFX::CParticleAttributeSamplerDeclaration *decl)
{
	if (decl != null)
	{
		attribSampler.m_SamplerFName = *FString(ANSI_TO_TCHAR(decl->ExportedName().Data()));
		attribSampler.m_SamplerType = ResolveAttribSamplerType(decl);

		attribSampler.m_AttributeCategoryName = *FString((const UTF16CHAR*)decl->CategoryName().MapDefault().Data());
		if (!attribSampler.m_AttributeCategoryName.IsValid() || attribSampler.m_AttributeCategoryName.IsNone())
			attribSampler.m_AttributeCategoryName = "General";
	}
	else
		attribSampler.Reset();
}

//----------------------------------------------------------------------------

// Not a member of UPopcornFXAttributeList so PopcornFXAttributeList.h can be a public header
inline PopcornFX::TMemoryView<PopcornFX::SAttributesContainer_SAttrib>	AttributeRawDataAttributes(UPopcornFXAttributeList *attrList)
{
	PK_ASSERT(attrList != null);
	PK_ASSERT(attrList->CheckDataIntegrity());
	return PopcornFX::TMemoryView<PopcornFX::SAttributesContainer_SAttrib>(reinterpret_cast<PopcornFX::SAttributesContainer_SAttrib*>(attrList->m_AttributesRawData.GetData()), attrList->AttributeCount());
}

//----------------------------------------------------------------------------

// Not a member of UPopcornFXAttributeList so PopcornFXAttributeList.h can be a public header
inline const PopcornFX::TMemoryView<const PopcornFX::SAttributesContainer_SAttrib>	AttributeRawDataAttributesConst(const UPopcornFXAttributeList *attrList)
{
	PK_ASSERT(attrList != null);
	PK_ASSERT(attrList->CheckDataIntegrity());
	return PopcornFX::TMemoryView<const PopcornFX::SAttributesContainer_SAttrib>(reinterpret_cast<const PopcornFX::SAttributesContainer_SAttrib*>(attrList->m_AttributesRawData.GetData()), attrList->AttributeCount());
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSampler		*FPopcornFXSamplerDesc::ResolveAttributeSampler(UPopcornFXEmitterComponent *emitter, UObject *enableLogForOwner) const
{
	PK_ASSERT(emitter != null);

	AActor	*fallbackActor = emitter->GetOwner();
	if (fallbackActor == null)
	{
		// No owner ? Anim notify ? Try finding an attach parent
		USceneComponent	*attachParent = emitter->GetAttachParent();
		if (attachParent != null)
			fallbackActor = attachParent->GetOwner();
	}

	const bool		validCompProperty = m_AttributeSamplerComponentProperty.IsValid() && !m_AttributeSamplerComponentProperty.IsNone();
	AActor			*parentActor = m_AttributeSamplerActor == null ? fallbackActor : m_AttributeSamplerActor;
	if (parentActor == null && !validCompProperty)
	{
		// nothing to do here
		return null;
	}
	if (!PK_VERIFY_MESSAGE(parentActor != null, "AttributeSampler is set but no parent Actor: should not happen"))
		return null;

	UPopcornFXAttributeSampler		*attrib = null;
	if (validCompProperty)
	{
		FObjectPropertyBase		*prop = FindFProperty<FObjectPropertyBase>(parentActor->GetClass(), m_AttributeSamplerComponentProperty);
		if (prop != null)
			attrib = Cast<UPopcornFXAttributeSampler>(prop->GetObjectPropertyValue_InContainer(parentActor));
	}
	else
	{
		attrib = Cast<UPopcornFXAttributeSampler>(parentActor->GetRootComponent());
	}
	if (attrib == null)
	{
		const bool		userSpecifedSomething = (m_AttributeSamplerActor != null || validCompProperty);
		if (enableLogForOwner != null && userSpecifedSomething) // do not log for default attrib sampler
		{
			UE_LOG(LogPopcornFXAttributeList, Warning,
				TEXT("Could not find component 'UPopcornFXAttributeSampler %s.%s' for attribute sampler '%s' in '%s'"),
				*parentActor->GetName(), (m_AttributeSamplerComponentProperty.IsValid() ? *m_AttributeSamplerComponentProperty.ToString() : TEXT("RootComponent")),
				*SamplerName(), *enableLogForOwner->GetFullName());
		}
		return null;
	}
	return attrib;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

UPopcornFXAttributeList::UPopcornFXAttributeList(const FObjectInitializer& PCIP)
:	Super(PCIP)
,	m_FileVersionId(0)
#if WITH_EDITORONLY_DATA
,	m_ColumnWidth(0.65f)
#endif // WITH_EDITORONLY_DATA
{
	SetFlags(RF_Transactional);
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeList::CheckDataIntegrity() const
{
	bool	ok = true;

	ok &= PK_VERIFY(m_AttributesRawData.Num() == m_Attributes.Num() * kAttributeSize);
	if (m_Owner != null && m_Owner->IsEmitterStarted())
	{
		PopcornFX::CParticleEffectInstance	*effectInstance = m_Owner->_GetEffectInstance();

		if (effectInstance->GetAllAttributes() == null)
			effectInstance->ResetAllAttributes();
		if (PK_VERIFY(effectInstance != null) && effectInstance->GetAllAttributes() != null)
		{
			const PopcornFX::SAttributesContainer	*instanceContainer = effectInstance->GetAllAttributes();

			ok &= PK_VERIFY(instanceContainer->AttributeCount() == m_Attributes.Num());
			ok &= PK_VERIFY(instanceContainer->SamplerCount() == m_Samplers.Num());
			ok &= PK_VERIFY(instanceContainer->AttributeCount() * kAttributeSize == m_AttributesRawData.Num());
		}
	}
	return ok;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeList::Valid() const
{
	return m_Effect != null;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeList::IsUpToDate(UPopcornFXEffect *effect) const
{
	if (effect == null)
	{
		PK_ASSERT(AttributeCount() == 0);
		PK_ASSERT(SamplerCount() == 0);
		PK_ASSERT(m_AttributesRawData.Num() == 0);
		PK_ASSERT(CheckDataIntegrity());
		return m_Effect == null && m_FileVersionId == 0;
	}
	if (effect != m_Effect)
		return false;
	if (effect->FileVersionId() != m_FileVersionId)
		return false;
	if (effect->IsTheDefaultAttributeList(this))
	{
		PK_ASSERT(CheckDataIntegrity());
		return true;
	}
	PK_ONLY_IF_ASSERTS(const UPopcornFXAttributeList		*defAttribs = effect->GetDefaultAttributeList());
	PK_ASSERT(this != defAttribs); // checked with IsTheDefaultAttributeList
	PK_ASSERT(defAttribs->IsUpToDate(effect));
	PK_ASSERT(defAttribs->AttributeCount() == AttributeCount());
	PK_ASSERT(defAttribs->SamplerCount() == SamplerCount());
	PK_ASSERT(CheckDataIntegrity());
	// @TOD may other cases
	return true;
}

//----------------------------------------------------------------------------

UPopcornFXAttributeList::~UPopcornFXAttributeList()
{
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeList::IsEmpty() const
{
	return m_Attributes.Num() == 0 && m_Samplers.Num() == 0;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::Clean()
{
	DBG_HERE();
#if WITH_EDITOR
	Modify();
#endif

	m_Effect = null;
	m_FileVersionId = 0;
	m_Attributes.Empty(m_Attributes.Num());
	m_Samplers.Empty(m_Samplers.Num());
	m_AttributesRawData.Empty(m_AttributesRawData.Num());
	PK_ASSERT(CheckDataIntegrity());

#if WITH_EDITOR
	m_Categories.Empty();
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------------

int32	UPopcornFXAttributeList::FindAttributeIndex(FName fname) const
{
	PK_ASSERT(CheckDataIntegrity());
	for (int32 attri = 0; attri < m_Attributes.Num(); ++attri)
	{
		if (m_Attributes[attri].m_AttributeFName == fname)
			return attri;
	}
	return -1;
}

//----------------------------------------------------------------------------

int32	UPopcornFXAttributeList::FindSamplerIndex(FName fname) const
{
	PK_ASSERT(CheckDataIntegrity());
	for (int32 sampleri = 0; sampleri < m_Samplers.Num(); ++sampleri)
	{
		if (m_Samplers[sampleri].m_SamplerFName == fname)
			return sampleri;
	}
	return -1;
}

//----------------------------------------------------------------------------

const UPopcornFXAttributeList		*UPopcornFXAttributeList::GetDefaultAttributeList(UPopcornFXEffect *effect) const
{
	if (!PK_VERIFY(IsUpToDate(effect)))
		return null;
	if (effect == null)
		return null;
	PK_ASSERT(CheckDataIntegrity());
	const UPopcornFXAttributeList	*defAttribs = effect->GetDefaultAttributeList();
	if (!PK_VERIFY(defAttribs != null))
		return null;
	return defAttribs;
}

//----------------------------------------------------------------------------
#if WITH_EDITOR
bool	UPopcornFXAttributeList::IsCategoryExpanded(uint32 categoryId) const
{
	if (!PK_VERIFY((int32)categoryId < m_CategoriesExpanded.Num()))
		return false;
	return m_CategoriesExpanded[categoryId] != 0 ? true : false;
}

void	UPopcornFXAttributeList::ToggleExpandedAttributeDetails(uint32 attributeId)
{
	PK_ASSERT(CheckDataIntegrity());
	if (!PK_VERIFY(int32(attributeId) < m_Attributes.Num()))
		return;
	m_Attributes[attributeId].m_IsExpanded = !m_Attributes[attributeId].m_IsExpanded;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::ToggleExpandedSamplerDetails(uint32 samplerId)
{
	PK_ASSERT(CheckDataIntegrity());
	if (!PK_VERIFY(int32(samplerId) < m_Samplers.Num()))
		return;
	m_Samplers[samplerId].m_IsExpanded = !m_Samplers[samplerId].m_IsExpanded;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::ToggleExpandedCategoryDetails(uint32 categoryId)
{
	PK_ASSERT(m_Effect != null);
	PK_ASSERT(CheckDataIntegrity());
	if (!PK_VERIFY(int32(categoryId) < m_CategoriesExpanded.Num()))
		return;
	PK_ASSERT(m_Categories.Num() == m_CategoriesExpanded.Num() || // Effect
			  m_Categories.Num() == 0);							  // Emitter
	m_CategoriesExpanded[categoryId] = m_CategoriesExpanded[categoryId] != 0 ? 0 : 1;
}
#endif // WITH_EDITOR
//----------------------------------------------------------------------------

const FPopcornFXAttributeDesc	*UPopcornFXAttributeList::GetAttributeDesc(uint32 attributeId) const
{
	PK_ASSERT(CheckDataIntegrity());
	if (!PK_VERIFY(int32(attributeId) < m_Attributes.Num()))
		return null;
	return &(m_Attributes[attributeId]);
}

//----------------------------------------------------------------------------

const void	*UPopcornFXAttributeList::GetAttributeDeclaration(UPopcornFXEffect *effect, uint32 attributeId) const
{
	if (!PK_VERIFY(int32(attributeId) < m_Attributes.Num()))
		return null;
	const UPopcornFXAttributeList				*defAttribs = GetDefaultAttributeList(effect);
	if (defAttribs == null)
		return null;
	const PopcornFX::CParticleAttributeList		*attrList = effect->Effect()->AttributeList().Get();
	if (!PK_VERIFY(attrList != null))
		return null;
	PK_ASSERT(attrList->UniqueAttributeList().Count() == m_Attributes.Num());
	return attrList->UniqueAttributeList()[attributeId];
}

//----------------------------------------------------------------------------

const FPopcornFXSamplerDesc	*UPopcornFXAttributeList::GetSamplerDesc(uint32 samplerId) const
{
	PK_ASSERT(CheckDataIntegrity());
	if (!PK_VERIFY(int32(samplerId) < m_Samplers.Num()))
		return null;
	return &(m_Samplers[samplerId]);
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
const void	*UPopcornFXAttributeList::GetParticleSampler(UPopcornFXEffect *effect, uint32 samplerId) const
{
	PK_ASSERT(CheckDataIntegrity());
	if (!PK_VERIFY(int32(samplerId) < m_Samplers.Num()))
		return null;
	const UPopcornFXAttributeList			*defAttribs = GetDefaultAttributeList(effect);
	if (defAttribs == null)
		return null;
	const PopcornFX::CParticleAttributeList		*attrList = effect->Effect()->AttributeList().Get();
	if (!PK_VERIFY(attrList != null))
		return null;
	PK_ASSERT(attrList->UniqueSamplerList().Count() == m_Samplers.Num());
	const PopcornFX::CParticleAttributeSamplerDeclaration	*decl = attrList->UniqueSamplerList()[samplerId];
	return decl;
}
#endif // WITH_EDITOR

//----------------------------------------------------------------------------

typedef PopcornFX::TMemoryView<PopcornFX::CParticleAttributeDeclaration const * const> CMVAttributes;
typedef PopcornFX::TMemoryView<PopcornFX::CParticleAttributeSamplerDeclaration const * const> CMVSamplers;

void	UPopcornFXAttributeList::SetupDefault(UPopcornFXEffect *effect, bool force)
{
	DBG_HERE();

	if (!force && IsUpToDate(effect))
	{
#if WITH_EDITOR
		//if (m_Categories.Num() == 0)
		//	m_Categories.Add("General"); // Effects not reimported
		m_CategoriesExpanded.SetNum(m_Categories.Num());
		const u32	categoryCount = m_CategoriesExpanded.Num();
		for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
			m_CategoriesExpanded[iCategory] = 1;
#endif // WITH_EDITOR
		return;
	}

#if WITH_EDITOR
	Modify();
#endif

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

#if WITH_EDITOR
	m_Attributes.SetNum(attrCount);
	for (int32 attri = 0; attri < attrCount; ++attri)
	{
		ResetAttribute(m_Attributes[attri], attrs[attri]);
		if (m_Attributes[attri].m_AttributeCategoryName.IsValid() && !m_Attributes[attri].m_AttributeCategoryName.IsNone())
			m_Categories.AddUnique(m_Attributes[attri].m_AttributeCategoryName);
	}

	m_Samplers.SetNum(samplerCount);
	for (int32 sampleri = 0; sampleri < samplerCount; ++sampleri)
	{
		ResetAttributeSampler(m_Samplers[sampleri], samplers[sampleri]);
		if (m_Samplers[sampleri].m_AttributeCategoryName.IsValid() && !m_Samplers[sampleri].m_AttributeCategoryName.IsNone())
			m_Categories.AddUnique(m_Samplers[sampleri].m_AttributeCategoryName);
	}

	if (attrCount > 0 || samplerCount > 0)
	{
		// Make "General" the first category if not already
		const int32	generalCategoryIndex = m_Categories.Find("General");
		if (generalCategoryIndex > 0)
			m_Categories.Swap(generalCategoryIndex, 0);
	}

	m_CategoriesExpanded.SetNum(m_Categories.Num());
	const u32	categoryCount = m_CategoriesExpanded.Num();
	for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
		m_CategoriesExpanded[iCategory] = 1;
#endif // WITH_EDITOR

	const PopcornFX::SAttributesContainer		*defContainer = *(attrList.DefaultAttributes());
	PK_ASSERT(defContainer != null);

	const uint32	attribBytes = defContainer->Attributes().CoveredBytes();
	m_AttributesRawData.SetNumUninitialized(attribBytes);
	if (attribBytes > 0)
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

bool	UPopcornFXAttributeList::Prepare(UPopcornFXEffect *effect, bool force)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeList::Prepare", POPCORNFX_UE_PROFILER_COLOR);

	DBG_HERE();

	PK_ASSERT(CheckDataIntegrity());

	if (!force && IsUpToDate(effect))
	{
#if WITH_EDITOR
		if (effect != null)
		{
			const UPopcornFXAttributeList	*refAttrList = effect->GetDefaultAttributeList();
			if (refAttrList != null &&
				m_CategoriesExpanded.Num() != refAttrList->m_CategoriesExpanded.Num())
			{
				m_CategoriesExpanded.SetNum(refAttrList->m_Categories.Num());
				const u32	categoryCount = m_CategoriesExpanded.Num();
				for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
					m_CategoriesExpanded[iCategory] = 1;
			}
		}
#endif // WITH_EDITOR
		return true;
	}

#if WITH_EDITOR
	Modify();
#endif

	if (effect == null)
	{
		Clean();
		return false;
	}

	const UPopcornFXAttributeList		*refAttrList = effect->GetDefaultAttributeList();
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

	TArray<FPopcornFXAttributeDesc>			*attrs = &m_Attributes;
	TArray<FPopcornFXSamplerDesc>			*samplers = &m_Samplers;
	TArray<uint8>							*rawData = &m_AttributesRawData;

	const TArray<FPopcornFXAttributeDesc>	*refAttrs = &refAttrList->m_Attributes;
	const TArray<FPopcornFXSamplerDesc>		*refSamplers = &refAttrList->m_Samplers;
	const TArray<uint8>						*refRawData = &refAttrList->m_AttributesRawData;

	bool		attrsChanged = false;

	// Re-match Attributes
	if (refRawData->Num() == 0)
	{
		attrsChanged |= !(attrs->Num() == 0);
		attrs->Empty();
		rawData->Empty();
	}
	else if (attrs->Num() == 0)
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
			if (attr->AttributeFName() != refAttr.AttributeFName() ||
				attr->m_AttributeCategoryName != refAttr.m_AttributeCategoryName)

			{
				attrsChanged = true;
				PopcornFX::CGuid		found;
				for (int32 findAttri = attri + 1; findAttri < attrs->Num(); ++findAttri)
				{
					FPopcornFXAttributeDesc		&findAttr = (*attrs)[findAttri];
					if (findAttr.AttributeFName() == refAttr.AttributeFName() &&
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
				PK_ASSERT(attr->AttributeFName() == refAttr.AttributeFName());
				PK_ASSERT(attr->m_AttributeCategoryName == refAttr.m_AttributeCategoryName);
			}

			if (attr->AttributeBaseTypeID() != refAttr.AttributeBaseTypeID())
			{
				attrsChanged = true;
				const PopcornFX::CBaseTypeTraits		&refTraits = PopcornFX::CBaseTypeTraits::Traits((PopcornFX::EBaseTypeID)refAttr.AttributeBaseTypeID());
				const PopcornFX::CBaseTypeTraits		&traits = PopcornFX::CBaseTypeTraits::Traits((PopcornFX::EBaseTypeID)attr->AttributeBaseTypeID());
				// If attribute changed float <-> int, reset value to default
				if (refTraits.ScalarType != traits.ScalarType)
				{
					CopyAttributeRawData(*rawData, attri, *refRawData, attri);
				}
				attr->m_AttributeType = refAttr.m_AttributeType;

#if	WITH_EDITOR
				if ((PopcornFX::EBaseTypeID)attr->m_AttributeType == PopcornFX::EBaseTypeID::BaseType_Quaternion)
				{
					const PopcornFX::SAttributesContainer_SAttrib		attrib = AttributeRawDataAttributes(this)[attri];
					const FRotator										rotator = FQuat(attrib.m_Data32f[0], attrib.m_Data32f[1], attrib.m_Data32f[2], attrib.m_Data32f[3]).Rotator();
					attr->m_AttributeEulerAngles = FVector(rotator.Roll, rotator.Pitch, rotator.Yaw);
				}
#endif
			}

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

	bool	samplersChanged = false;

	// Re-match Samplers
	if (refSamplers->Num() == 0)
	{
		samplersChanged |= !(samplers->Num() == 0);
		samplers->Empty();
	}
	else if (samplers->Num() == 0)
	{
		samplersChanged = true;
		*samplers = *refSamplers;
	}
	else
	{
		samplers->Reserve(refSamplers->Num());

		int32	sampleri = 0;
		for (; sampleri < refSamplers->Num() && sampleri < samplers->Num(); ++sampleri)
		{
			const FPopcornFXSamplerDesc		&refSampler = (*refSamplers)[sampleri];
			FPopcornFXSamplerDesc			*sampler = &((*samplers)[sampleri]);

			PK_ASSERT(refSampler.ValueIsEmpty());

			if (sampler->SamplerFName() != refSampler.SamplerFName() ||
				sampler->m_AttributeCategoryName != refSampler.m_AttributeCategoryName)
			{
				samplersChanged = true;
				PopcornFX::CGuid		found;
				for (int32 foundSampleri = sampleri + 1; foundSampleri < samplers->Num(); ++foundSampleri)
				{
					FPopcornFXSamplerDesc	&foundSampler = (*samplers)[foundSampleri];
					if (foundSampler.SamplerFName() == refSampler.SamplerFName() &&
						foundSampler.m_AttributeCategoryName == refSampler.m_AttributeCategoryName)
					{
						found = foundSampleri;
						break;
					}
				}
				if (!found.Valid())
				{
					// (Can result to superfluous copies, but, here, dont care, and prefer simpler code)
					found = samplers->Add(refSampler);
					sampler = &((*samplers)[sampleri]); // minds Add() realloc
				}
				samplers->SwapMemory(sampleri, found);
				PK_ASSERT(sampler->Valid());
				PK_ASSERT(sampler->SamplerFName() == refSampler.SamplerFName());
				PK_ASSERT(sampler->m_AttributeCategoryName == refSampler.m_AttributeCategoryName);
			}

			// If type missmatch, just set to default (ResetValue())
			if (sampler->SamplerType() != refSampler.SamplerType())
			{
				samplersChanged = true;
				*sampler = refSampler;
			}

			PK_ASSERT(sampler->ExactMatch(refSampler)); // Name, Category and Type must match now
		}

		samplers->SetNum(refSamplers->Num());
		for (; sampleri < refSamplers->Num(); ++sampleri)
		{
			const FPopcornFXSamplerDesc	&refSampler = (*refSamplers)[sampleri];
			FPopcornFXSamplerDesc		&sampler = (*samplers)[sampleri];
			PK_ASSERT(refSampler.ValueIsEmpty());
			sampler = refSampler;
		}
	}

	PK_ASSERT(CheckDataIntegrity());

#if WITH_EDITOR
	if (attrsChanged || samplersChanged)
	{
		m_CategoriesExpanded.SetNum(refAttrList->m_Categories.Num());
		const u32	categoryCount = m_CategoriesExpanded.Num();
		for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
			m_CategoriesExpanded[iCategory] = 1;

		ForceSetPackageDirty(this);
	}
#endif

	return true;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::CopyFrom(const UPopcornFXAttributeList *other, AActor *patchParentActor)
{
	DBG_HERE();

#if WITH_EDITOR
	Modify();
#endif

	if (!PK_VERIFY(other != null))
	{
		Clean();
		return;
	}

	PK_ASSERT(other->CheckDataIntegrity());

	m_Effect = other->m_Effect;
	m_FileVersionId = other->m_FileVersionId;
	m_Attributes = other->m_Attributes;
	m_Samplers = other->m_Samplers;
	m_AttributesRawData = other->m_AttributesRawData;
#if WITH_EDITOR
	if (other != null)
	{
		m_CategoriesExpanded.SetNum(other->m_Categories.Num());
		const u32	categoryCount = m_CategoriesExpanded.Num();
		for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
			m_CategoriesExpanded[iCategory] = 1;
	}
#endif // WITH_EDITOR

	if (patchParentActor != null)
	{
		for (int32 sampleri = 0; sampleri < m_Samplers.Num(); ++sampleri)
		{
			FPopcornFXSamplerDesc		&desc = m_Samplers[sampleri];
			if (desc.m_AttributeSamplerActor == null && desc.m_AttributeSamplerComponentProperty.IsValid() && !desc.m_AttributeSamplerComponentProperty.IsNone())
			{
				desc.m_AttributeSamplerActor = patchParentActor;
			}
		}
	}
	PK_ASSERT(CheckDataIntegrity());
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::RestoreAttributesFromCachedRawData(const TArray<uint8> &rawData)
{
	const u32	coveredBytes = rawData.Num();
	if (!PK_VERIFY(CheckDataIntegrity()) &&
		!PK_VERIFY(m_AttributesRawData.Num() == coveredBytes))
		return;
	PopcornFX::Mem::Copy(m_AttributesRawData.GetData(), rawData.GetData(), coveredBytes);

	if (m_Owner != null)
		_RefreshAttributes(m_Owner);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::ResetToDefaultValues(UPopcornFXEffect *effect)
{
	DBG_HERE();

	if (!PK_VERIFY(Valid()))
		return;

#if WITH_EDITOR
	Modify();
#endif

	PK_ASSERT(CheckDataIntegrity());

	const UPopcornFXAttributeList		*defAttribs = GetDefaultAttributeList(effect);
	if (defAttribs == null)
		return;

	const TArray<uint8>			&defRawData = defAttribs->m_AttributesRawData;
	PK_ASSERT(m_AttributesRawData.Num() == defRawData.Num());
	m_AttributesRawData = defRawData;

	if (m_Owner != null)
		_RefreshAttributes(m_Owner);

	for (int32 i = 0; i < m_Samplers.Num(); ++i)
		m_Samplers[i].ResetValue();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::PostLoad()
{
	DBG_HERE();
	m_Owner = null;
	Super::PostLoad();
	PK_ASSERT(CheckDataIntegrity());
}

void	UPopcornFXAttributeList::PostInitProperties()
{
	DBG_HERE();
	m_Owner = null;
	Super::PostInitProperties();
}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5)
void	UPopcornFXAttributeList::PreSave(FObjectPreSaveContext SaveContext)
#else
void	UPopcornFXAttributeList::PreSave(const class ITargetPlatform* TargetPlatform)
#endif // (ENGINE_MAJOR_VERSION == 5)
{
	DBG_HERE();
	PK_ASSERT(CheckDataIntegrity());
	// make sure m_AttributesContainer up to date
	PK_ASSERT(CheckDataIntegrity());

#if (ENGINE_MAJOR_VERSION == 5)
	Super::PreSave(SaveContext);
#else
	Super::PreSave(TargetPlatform);
#endif // (ENGINE_MAJOR_VERSION == 5)
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::Serialize(FArchive& Ar)
{
	DBG_HERE();

	// Attributes loading/saving: we don't want delta on properties like m_Effect, m_Attributes, ..
	// They don't differ between this and what seem to be the container (parent blueprint)
	// At load time, this leads to corrupt attributes
	const bool	arNoDeltaPrev = Ar.ArNoDelta;
	Ar.ArNoDelta = true;

	Super::Serialize(Ar);

	// Restore old state
	Ar.ArNoDelta = arNoDeltaPrev;

	Ar.UsingCustomVersion(FPopcornFXCustomVersion::GUID);

	//UE_LOG(LogPopcornFXAttributeList, Log, TEXT("--- ATTRLIST Serizalie save %p %s --- %s"), this, (Ar.IsSaving() ? "saving" : "restoring"), *GetFullName());

	const int32 version = Ar.CustomVer(FPopcornFXCustomVersion::GUID);
	//UE_LOG(LogPopcornFXAttributeList, Log, TEXT("--- Serialize load:%d %d --- %3d %3d %3d --- %s"), Ar.IsLoading() == 1, version, m_AttributesRawData.Num(), m_Attributes.Num(), m_Samplers.Num(), *GetFullName());

	if (version < FPopcornFXCustomVersion::BeforeCustomVersionWasAdded)
	{
		TArray<uint8>	attribs;
		int32			attribCount = 0;
		int32			samplerCount = 0;

		Ar << attribs;
		Ar << samplerCount;
		Ar << attribCount;

		//UE_LOG(LogPopcornFXAttributeList, Log, TEXT("--- Serialize %d data     --- %3d %3d %3d --- %s"), Ar.IsLoading() == 1, attribs.Num(), attribCount, samplerCount, *GetFullName());
		//UE_LOG(LogPopcornFXAttributeList, Log, TEXT("--- Serialize %d OLD     --- %3d %3d %3d --- %s"), Ar.IsLoading() == 1, attribs.Num(), attribCount, samplerCount, *GetFullName());
		if (Ar.IsLoading())
		{
			if (attribs.Num() > 0)
				m_AttributesRawData = attribs;
			if (!PK_VERIFY(m_AttributesRawData.Num() == m_Attributes.Num() * kAttributeSize))
			{
				//UE_LOG(LogPopcornFXAttributeList, Warning, TEXT("Attribute values seems corrupted, attribute will be reset for '%s'"), *GetFullName());
				m_Effect = null;
				m_FileVersionId = 0;
				m_Attributes.Empty(m_Attributes.Num());
				m_Samplers.Empty(m_Samplers.Num());
				m_AttributesRawData.Empty(m_AttributesRawData.Num());
			}
		}
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::BeginDestroy()
{
	DBG_HERE();
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------
#if WITH_EDITOR
//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::PostEditUndo()
{
	if (m_Owner != null)
		_RefreshAttributes(m_Owner);
	Super::PostEditUndo();
}

//----------------------------------------------------------------------------
#endif // WITH_EDITOR
//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::Scene_PreUpdate(UPopcornFXEmitterComponent *emitter, float deltaTime)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeList::Scene_PreUpdate", POPCORNFX_UE_PROFILER_COLOR);

	PK_ASSERT(emitter != null);
	PK_ASSERT(m_Owner == null || emitter == m_Owner);
	m_Owner = emitter;

	for (int32 sampleri = 0; sampleri < m_Samplers.Num(); ++sampleri)
	{
		FPopcornFXSamplerDesc			&desc = m_Samplers[sampleri];
		if (desc.m_NeedUpdate)
		{
			UPopcornFXAttributeSampler		*attribSampler = desc.ResolveAttributeSampler(emitter, null/*dont log each frame*/);
			if (attribSampler != null)
			{
				attribSampler->_AttribSampler_PreUpdate(deltaTime);
			}
		}
	}
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
void	UPopcornFXAttributeList::AttributeSamplers_IndirectSelectedThisTick(UPopcornFXEmitterComponent *emitter) const
{
	for (int32 sampleri = 0; sampleri < m_Samplers.Num(); ++sampleri)
	{
		const FPopcornFXSamplerDesc		&desc = m_Samplers[sampleri];
		UPopcornFXAttributeSampler		*attribSampler = desc.ResolveAttributeSampler(emitter, null/*dont log at each frame*/);
		if (attribSampler != null)
			attribSampler->_AttribSampler_IndirectSelectedThisTick();
	}
}
#endif // WITH_EDITOR

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeList::SetAttributeSampler(FName samplerName, AActor *actor, FName propertyName)
{
	// The actual UPopcornFXAttributeSampler will be resolved later
	if (m_Owner != null && m_Owner->IsEmitterStarted())
	{
		UE_LOG(LogPopcornFXAttributeList, Warning, TEXT("SetAttributeSampler cannot be called on started emitters."));
		return false;
	}
	for (int32 iSampler = 0; iSampler < m_Samplers.Num(); ++iSampler)
	{
		if (m_Samplers[iSampler].SamplerFName() == samplerName)
		{
			m_Samplers[iSampler].m_AttributeSamplerActor = actor;
			m_Samplers[iSampler].m_AttributeSamplerComponentProperty = propertyName;
			return true;
		}
	}
	UE_LOG(LogPopcornFXAttributeList, Warning, TEXT("SetAttributeSampler couldn't find sampler name %s"), *samplerName.ToString());
	return false;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::GetAttribute(uint32 attributeId, FPopcornFXAttributeValue &outAttribute) const
{
#if WITH_EDITOR
	if (!PK_VERIFY(attributeId < (u32)m_Attributes.Num()))
		return;
#else
	check(attributeId < (u32)m_Attributes.Num());
#endif
	PopcornFX::SAttributesContainer_SAttrib	&_outAttribute = *reinterpret_cast<PopcornFX::SAttributesContainer_SAttrib*>(&outAttribute);

	const PopcornFX::EBaseTypeID	typeID = (PopcornFX::EBaseTypeID)m_Attributes[attributeId].AttributeBaseTypeID();
	if (m_Owner != null && m_Owner->IsEmitterStarted())
	{
		PopcornFX::CParticleEffectInstance	*effectInstance = m_Owner->_GetEffectInstance();

		if (PK_VERIFY(effectInstance != null))
		{
			if (PK_VERIFY(effectInstance->GetRawAttribute(attributeId, typeID, &_outAttribute, true)))
				return;
			UE_LOG(LogPopcornFXAttributeList, Warning, TEXT("Couldn't get attribute %s on effect instance %p"), *m_Attributes[attributeId].AttributeName(), effectInstance);
		}
	}
	_outAttribute = AttributeRawDataAttributesConst(this)[attributeId];
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::SetAttribute(uint32 attributeId, const FPopcornFXAttributeValue &value)
{
#if WITH_EDITOR
	if (!PK_VERIFY(attributeId < (u32)m_Attributes.Num()))
		return;
#else
	check(attributeId < (u32)m_Attributes.Num());
#endif
	const PopcornFX::SAttributesContainer_SAttrib	&_value = *reinterpret_cast<const PopcornFX::SAttributesContainer_SAttrib*>(&value);

	const PopcornFX::EBaseTypeID	typeID = (PopcornFX::EBaseTypeID)m_Attributes[attributeId].AttributeBaseTypeID();
	if (m_Owner != null && m_Owner->IsEmitterStarted())
	{
		PopcornFX::CParticleEffectInstance	*effectInstance = m_Owner->_GetEffectInstance();

		if (PK_VERIFY(effectInstance != null))
		{
			if (!PK_VERIFY(effectInstance->SetRawAttribute(attributeId, typeID, &_value, true)))
			{
				UE_LOG(LogPopcornFXAttributeList, Warning, TEXT("Couldn't set attribute %s on effect instance %p"), *m_Attributes[attributeId].AttributeName(), effectInstance);
			}
		}
	}
	AttributeRawDataAttributes(this)[attributeId] = _value;
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

template<typename _Scalar>
_Scalar	UPopcornFXAttributeList::GetAttributeDim(uint32 attributeId, uint32 dim)
{
	PopcornFX::SAttributesContainer_SAttrib	value;
	GetAttribute(attributeId, *reinterpret_cast<FPopcornFXAttributeValue*>(&value)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day

	return value.Get<_Scalar>()[dim];
}

//----------------------------------------------------------------------------

template<>
bool	UPopcornFXAttributeList::GetAttributeDim<bool>(uint32 attributeId, uint32 dim)
{
	PopcornFX::SAttributesContainer_SAttrib	value;
	GetAttribute(attributeId, *reinterpret_cast<FPopcornFXAttributeValue*>(&value)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day

	return reinterpret_cast<bool*>(value.Get<uint32>())[dim];
}

//----------------------------------------------------------------------------

float UPopcornFXAttributeList::GetAttributeQuaternionDim(uint32 attributeId, uint32 dim)
{
#if WITH_EDITOR
	if (!PK_VERIFY(attributeId < (u32)m_Attributes.Num()))
		return 0.0f;
#else
	check(attributeId < (u32)m_Attributes.Num());
#endif

	PopcornFX::SAttributesContainer_SAttrib	value;
	GetAttribute(attributeId, *reinterpret_cast<FPopcornFXAttributeValue*>(&value)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day

	const float	*scalarValue = value.Get<float>();
	const FQuat	quat = FQuat(scalarValue[0], scalarValue[1], scalarValue[2], scalarValue[3]);

	const FQuat storedQuat = FQuat(FRotator(m_Attributes[attributeId].m_AttributeEulerAngles[1], m_Attributes[attributeId].m_AttributeEulerAngles[2], m_Attributes[attributeId].m_AttributeEulerAngles[0]));

	if (storedQuat != quat)
	{
		const FRotator	rotator = quat.Rotator();
		m_Attributes[attributeId].m_AttributeEulerAngles = FVector(rotator.Roll, rotator.Pitch, rotator.Yaw);
	}

	return m_Attributes[attributeId].m_AttributeEulerAngles[dim];
}

//----------------------------------------------------------------------------

void UPopcornFXAttributeList::SetAttributeQuaternionDim(uint32 attributeId, uint32 dim, float value)
{
#if WITH_EDITOR
	if (!PK_VERIFY(attributeId < (u32)m_Attributes.Num()))
		return;
#else
	check(attributeId < (u32)m_Attributes.Num());
#endif

	FPopcornFXAttributeDesc* attributeDesc = &m_Attributes[attributeId];

	attributeDesc->m_AttributeEulerAngles[dim] = value;

	const FVector			&eulerAngles = attributeDesc->m_AttributeEulerAngles;

	const FQuat				quaternion = FRotator(eulerAngles[1], eulerAngles[2], eulerAngles[0]).Quaternion();

	PopcornFX::SAttributesContainer_SAttrib	newValue = AttributeRawDataAttributes(this)[attributeId];
	newValue.m_Data32f[0] = quaternion.X;
	newValue.m_Data32f[1] = quaternion.Y;
	newValue.m_Data32f[2] = quaternion.Z;
	newValue.m_Data32f[3] = quaternion.W;

	SetAttribute(attributeId, *reinterpret_cast<FPopcornFXAttributeValue*>(&newValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
}

//----------------------------------------------------------------------------

template<typename _Scalar>
void	UPopcornFXAttributeList::SetAttributeDim(uint32 attributeId, uint32 dim, _Scalar value)
{
#if WITH_EDITOR
	if (!PK_VERIFY(attributeId < (u32)m_Attributes.Num()))
		return;
#else
	check(attributeId < (u32)m_Attributes.Num());
#endif

	PopcornFX::SAttributesContainer_SAttrib	newValue = AttributeRawDataAttributes(this)[attributeId];
	newValue.Get<_Scalar>()[dim] = value;

	SetAttribute(attributeId, *reinterpret_cast<FPopcornFXAttributeValue*>(&newValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
}

//----------------------------------------------------------------------------

template <>
void	UPopcornFXAttributeList::SetAttributeDim<bool>(uint32 attributeId, uint32 dim, bool value)
{
#if WITH_EDITOR
	if (!PK_VERIFY(attributeId < (u32)m_Attributes.Num()))
		return;
#else
	check(attributeId < (u32)m_Attributes.Num());
#endif

	PopcornFX::SAttributesContainer_SAttrib	newValue = AttributeRawDataAttributes(this)[attributeId];
	reinterpret_cast<bool*>(newValue.Get<uint32>())[dim] = value;

	SetAttribute(attributeId, *reinterpret_cast<FPopcornFXAttributeValue*>(&newValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::_RefreshAttributes(const UPopcornFXEmitterComponent *emitter)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeList::_RefreshAttributes", POPCORNFX_UE_PROFILER_COLOR);

	DBG_HERE();

	PK_ASSERT(emitter != null);
	PK_ASSERT(m_Owner == null || emitter == m_Owner);
	m_Owner = emitter;

	// If the emitter is running, set its attributes from serialized attributes
	if (!PK_VERIFY(m_Owner != null))
		return;
	PK_ASSERT(CheckDataIntegrity());

	if (!m_Owner->IsEmitterStarted())
		return;
	PopcornFX::CParticleEffectInstance	*effectInstance = m_Owner->_GetEffectInstance();
	if (!PK_VERIFY(effectInstance != null))
		return;

	const u32																attrCount = m_Attributes.Num();
	const PopcornFX::TMemoryView<PopcornFX::SAttributesContainer_SAttrib>	attrValues = AttributeRawDataAttributes(this);

	PK_ASSERT(attrCount == attrValues.Count());
	for (u32 iAttr = 0; iAttr < attrCount; ++iAttr)
	{
		const FPopcornFXAttributeDesc					&attrDesc = m_Attributes[iAttr];
		const PopcornFX::SAttributesContainer_SAttrib	&value = attrValues[iAttr];

		// First call to effectInstance->SetAttribute will lazy create the SAttributesContainer
		if (!PK_VERIFY(effectInstance->SetRawAttribute(iAttr, (PopcornFX::EBaseTypeID)attrDesc.AttributeBaseTypeID(), &value, true)))
		{
			UE_LOG(LogPopcornFXAttributeList, Warning, TEXT("Couldn't set attribute %s on effect instance %p"), *attrDesc.AttributeName(), effectInstance);
		}
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::_RefreshAttributeSamplers(UPopcornFXEmitterComponent *emitter, bool reload)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeList::_RefreshAttributeSamplers", POPCORNFX_UE_PROFILER_COLOR);

	PK_ASSERT(emitter != null);
	PK_ASSERT(m_Owner == null || emitter == m_Owner);
	m_Owner = emitter;

	if (!PK_VERIFY(emitter != null))
		return;
	UPopcornFXEffect	*effect = emitter->Effect;
	Prepare(effect);

	PK_ASSERT(IsUpToDate(effect));
	if (!PK_VERIFY(effect != null))
		return;

	if (m_Samplers.Num() == 0)
		return;

	if (m_Owner == null || !m_Owner->IsEmitterStarted())
		return;
	PopcornFX::CParticleEffectInstance	*effectInstance = m_Owner->_GetEffectInstance();
	if (!PK_VERIFY(effectInstance != null))
		return;

	PK_ASSERT(CheckDataIntegrity());

	if (effectInstance->GetAllAttributes() != null &&
		!PK_VERIFY(m_Samplers.Num() == effectInstance->GetAllAttributes()->Samplers().Count()))
		return;
	const PopcornFX::PCParticleAttributeList	&attrListPtr = effect->Effect()->AttributeList();
	if (attrListPtr == null || *(attrListPtr->DefaultAttributes()) == null)
		return;
	const PopcornFX::SAttributesContainer	*defCont = *(attrListPtr->DefaultAttributes());
	if (!PK_VERIFY(defCont != null && defCont->Samplers().Count() == m_Samplers.Num()))
		return;

	for (int32 sampleri = 0; sampleri < m_Samplers.Num(); ++sampleri)
	{
		PK_ASSERT(attrListPtr->SamplerList()[sampleri] != null);
		const PopcornFX::PResourceDescriptor	defaultSampler = attrListPtr->SamplerList()[sampleri]->AttribSamplerDefaultValue();
		if (!PK_VERIFY(defaultSampler != null))
			continue;

		FPopcornFXSamplerDesc	&desc = m_Samplers[sampleri];
		desc.m_NeedUpdate = false;

		if (desc.SamplerType() == EPopcornFXAttributeSamplerType::None)
		{
			effectInstance->SetAttributeSampler(TCHAR_TO_ANSI(*desc.SamplerName()), null);
			continue;
		}

		PK_ASSERT(desc.SamplerType() == ResolveAttribSamplerType(attrListPtr->SamplerList()[sampleri].Get()));
		UPopcornFXAttributeSampler	*attribSampler = desc.ResolveAttributeSampler(emitter, emitter);
		if (attribSampler == null ||
			!PK_VERIFY(desc.SamplerType() == attribSampler->SamplerType()))
		{
			effectInstance->SetAttributeSampler(TCHAR_TO_ANSI(*desc.SamplerName()), null);
			continue;
		}

		PopcornFX::CParticleSamplerDescriptor	*samplerDescriptor = null;

		// Dont setup samplers when cooking
		if (!IsRunningCommandlet() && attribSampler != null)
			samplerDescriptor = attribSampler->_AttribSampler_SetupSamplerDescriptor(desc, defaultSampler.Get());

		effectInstance->SetAttributeSampler(TCHAR_TO_ANSI(*desc.SamplerName()), samplerDescriptor != null ? samplerDescriptor : null);
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeList::CheckEmitter(const class UPopcornFXEmitterComponent *emitter)
{
	PK_ASSERT(emitter != null);
#if 0 // We can't trust this when UE is doing its garbage collect
	PK_ASSERT(m_Owner == null || m_Owner == emitter);
#endif
	m_Owner = emitter;
}

//----------------------------------------------------------------------------
