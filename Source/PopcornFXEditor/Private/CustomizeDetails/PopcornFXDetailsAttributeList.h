//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "Runtime/Launch/Resources/Version.h"
#include "PopcornFXAttributeSampler.h"
#include "PropertyEditorModule.h"
#include "PopcornFXAttributeList.h"
#include "DetailLayoutBuilder.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_samplers.h>
#include <pk_particles/include/ps_samplers_classes.h>
#include <pk_particles/include/ps_attributes.h>

#include "IDetailCustomization.h"

class	UPopcornFXAttributeList;
class	UPopcornFXEffect;
class	IDetailGroup;

UPopcornFXEffect						*ResolveEffect(const UPopcornFXAttributeList *attrList);
FString									GenerateTypeName(PopcornFX::EBaseTypeID typeId);
const char								*ResolveAttribSamplerNodeName(const PopcornFX::CParticleAttributeSamplerDeclaration *sampler, EPopcornFXAttributeSamplerType::Type samplerType);
TSharedPtr<IPropertyHandle>				ResolveSamplerProperties(const TSharedPtr<IPropertyHandle> samplerPty, EPopcornFXAttributeSamplerType::Type type, const FString &samplerTypeName);

class FPopcornFXDetailsAttributeList : public IDetailCustomization
{
public:

	// Data needed to craft a widget representing an attribute
	struct FAttributeDesc
	{
	public:
		typedef FAttributeDesc					TSelf;
		typedef FPopcornFXDetailsAttributeList	TParent;

		UPopcornFXAttributeList					*m_AttributeList = nullptr;

		bool									m_ReadOnly = false;

		uint32									m_Index = 0;
		u32										m_VectorDimension = 0;
		const PopcornFX::CBaseTypeTraits		*m_Traits = null;

		bool									m_IsColor = false;
		bool									m_IsQuaternion = false;
		bool									m_IsOneShotTrigger = false;
		EPopcornFXAttributeDropDownMode::Type	m_DropDownMode;
		TArray<FString>							m_EnumList; // copy
		TArray<TSharedPtr<FString>>				m_SharedEnumList;
		TArray<TSharedPtr<int32>>				m_EnumListIndices;
		bool									m_HasMin;
		bool									m_HasMax;

		FSlateFontInfo							m_Font;

		PopcornFX::SAttributesContainer_SAttrib	m_Min;
		PopcornFX::SAttributesContainer_SAttrib	m_Max;
		PopcornFX::SAttributesContainer_SAttrib	m_Def;
	};

	typedef FPopcornFXDetailsAttributeList		TSelf;

	FPopcornFXDetailsAttributeList();
	~FPopcornFXDetailsAttributeList();

public:

	UPopcornFXAttributeList			*UnsafeAttrList();
	const UPopcornFXAttributeList	*UnsafeAttrList() const;
	UPopcornFXAttributeList			*AttrList();

protected:

	void						RebuildAndRefresh();
	void						Rebuild();
	void						RebuildIFN();
	void						RebuildAttributes();
	void						BuildAttribute(const FPopcornFXAttributeDesc *desc, const UPopcornFXAttributeList *attrList, uint32 attri, uint32 iCategory);
	void						RebuildSamplers();
	virtual void				BuildSampler(const FPopcornFXSamplerDesc *desc, const TSharedPtr<IPropertyHandle> samplerPty, const TSharedPtr<IPropertyHandle> samplerDescPty, const UPopcornFXAttributeList *attrList, uint32 sampleri, uint32 iCategory) {}
	FReply						OnResetClicked(FAttributeDesc slateDesc);
	FReply						OnDimResetClicked(uint32 dimi, FAttributeDesc slateDesc);
	EVisibility					GetResetVisibility(FAttributeDesc slateDesc) const;
	EVisibility					GetDimResetVisibility(uint32 dimi, FAttributeDesc slateDesc) const;

	TArray<TWeakObjectPtr<UObject> >	m_BeingCustomized;
	IDetailLayoutBuilder				*m_DetailLayoutBuilder;
	TSharedPtr<IPropertyUtilities>		m_PropertyUtilities;
	IDetailCategoryBuilder				*m_AttributeListCategory;

	TSharedPtr<IPropertyHandle>			m_AttributeListPty;
	TSharedPtr<IPropertyHandle>			m_SamplersPty;
	TSharedPtr<IPropertyHandle>			m_SamplersDescPty;

	UPopcornFXEffect					*m_Effect;

	TArray<IDetailGroup*>				m_IGroups;
	TArray<uint32>						m_NumAttributes;
};

/** Customization used just to hide the default AttributeList appearance */
class FPopcornFXDetailsAttributeListHidden : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization>	MakeInstance() { return MakeShareable(new FPopcornFXDetailsAttributeListHidden);  }
	
	void CustomizeDetails(IDetailLayoutBuilder &LayoutBuilder)
	{
		LayoutBuilder.HideProperty("m_FileVersionId");
		LayoutBuilder.HideProperty("m_Attributes");
		LayoutBuilder.HideProperty("m_Samplers");
		LayoutBuilder.HideProperty("m_AttributesRawData");
		return;
	}
};

#endif // WITH_EDITOR
