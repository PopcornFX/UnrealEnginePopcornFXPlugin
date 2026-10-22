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

class	UPopcornFXEffect;
class	IDetailGroup;

UPopcornFXEffect						*ResolveEffect(const FPopcornFXAttributeList *attrList);
FString									GenerateTypeName(PopcornFX::EBaseTypeID typeId);
const char								*ResolveAttribSamplerNodeName(const PopcornFX::CParticleAttributeSamplerDeclaration *sampler, EPopcornFXAttributeSamplerType::Type samplerType);
TSharedPtr<IPropertyHandle>				ResolveSamplerProperties(const TSharedPtr<IPropertyHandle> samplerDescPty, EPopcornFXAttributeSamplerType::Type type, const FString &samplerTypeName);

class FPopcornFXCustomizationAttributeList : public IPropertyTypeCustomization
{
public:

	// Data needed to craft a widget representing an attribute
	struct FAttributeDesc
	{
	public:
		typedef FAttributeDesc							TSelf;
		typedef FPopcornFXCustomizationAttributeList	TParent;

		FPopcornFXAttributeList							*m_AttributeList = nullptr;

		bool											m_ReadOnly = false;

		uint32											m_Index = 0;
		u32												m_VectorDimension = 0;
		const PopcornFX::CBaseTypeTraits				*m_Traits = null;

		bool											m_IsColor = false;
		bool											m_IsQuaternion = false;
		bool											m_IsOneShotTrigger = false;
		EPopcornFXAttributeDropDownMode::Type			m_DropDownMode;
		TArray<FString>									m_EnumList; // copy
		TArray<TSharedPtr<FString>>						m_SharedEnumList;
		TArray<TSharedPtr<int32>>						m_EnumListIndices;
		bool											m_HasMin;
		bool											m_HasMax;

		FSlateFontInfo									m_Font;

		PopcornFX::SAttributesContainer_SAttrib			m_Min;
		PopcornFX::SAttributesContainer_SAttrib			m_Max;
		PopcornFX::SAttributesContainer_SAttrib			m_Def;
	};

	typedef FPopcornFXCustomizationAttributeList		TSelf;

	FPopcornFXCustomizationAttributeList();
	~FPopcornFXCustomizationAttributeList();

public:

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization>	MakeInstance() { return MakeShareable(new FPopcornFXCustomizationAttributeList); }

	virtual void	CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow &HeaderRow, IPropertyTypeCustomizationUtils &CustomizationUtils) override;
	virtual void	CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder &ChildBuilder, IPropertyTypeCustomizationUtils &CustomizationUtils) override;

	FPopcornFXAttributeList			*RetrieveAttributeList() const;
	FPopcornFXAttributeList			*AttrList();
	const FPopcornFXAttributeList	*AttrList() const;

protected:

	void						RebuildAndRefresh();
	void						Rebuild();
	void						RebuildIFN();
	void						UpdateSampler(const FPopcornFXSamplerDesc *desc, FPopcornFXAttributeSampler *sampler);
	void						RebuildAttributes();
	void						BuildAttribute(const FPopcornFXAttributeDesc *desc, const FPopcornFXAttributeList *attrList, uint32 attri, uint32 iCategory);
	void						RebuildSamplers();
	virtual void				BuildSampler(const FPopcornFXSamplerDesc *desc, const TSharedPtr<IPropertyHandle> samplerDescPty, FPopcornFXAttributeList *attrList, uint32 sampleri, uint32 iCategory);
	FReply						OnResetClicked(FAttributeDesc slateDesc);
	FReply						OnDimResetClicked(uint32 dimi, FAttributeDesc slateDesc);
	EVisibility					GetResetVisibility(FAttributeDesc slateDesc) const;
	EVisibility					GetDimResetVisibility(uint32 dimi, FAttributeDesc slateDesc) const;

	TArray<UObject* >					m_BeingCustomized;
	IDetailLayoutBuilder				*m_DetailLayoutBuilder;
	TSharedPtr<IPropertyUtilities>		m_PropertyUtilities;
	IDetailChildrenBuilder				*m_ChildBuilder;

	TSharedPtr<IPropertyHandle>			m_AttributeListPty;
	TSharedPtr<IPropertyHandle>			m_AttributesRawDataPty;
	TSharedPtr<IPropertyHandle>			m_SamplersPty;
	TSharedPtr<IPropertyHandle>			m_SamplersDescPty;

	UPopcornFXEffect					*m_Effect = nullptr;
	UPopcornFXEmitterComponent			*m_Owner = nullptr;

	TArray<IDetailGroup*>				m_IGroups;
	TArray<uint32>						m_NumAttributes;
};

#endif // WITH_EDITOR
