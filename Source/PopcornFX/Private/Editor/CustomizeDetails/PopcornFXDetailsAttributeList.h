//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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
namespace { class	SPopcornFXAttributeCategory; }

UPopcornFXEffect						*ResolveEffect(const UPopcornFXAttributeList *attrList);
FString									GenerateTypeName(PopcornFX::EBaseTypeID typeId);
const char								*ResolveAttribSamplerNodeName(const PopcornFX::CParticleAttributeSamplerDeclaration *sampler, EPopcornFXAttributeSamplerType::Type samplerType);
TSharedPtr<IPropertyHandle>				ResolveSamplerProperties(const TSharedPtr<IPropertyHandle> samplerPty, EPopcornFXAttributeSamplerType::Type type, const FString &samplerTypeName);

class FPopcornFXDetailsAttributeList : public IDetailCustomization
{
public:
	typedef FPopcornFXDetailsAttributeList		TSelf;

	FPopcornFXDetailsAttributeList();
	~FPopcornFXDetailsAttributeList();

public:

	UPopcornFXAttributeList		*UnsafeAttrList();
	UPopcornFXAttributeList		*AttrList();

protected:

	void						RebuildAndRefresh();
	void						Rebuild();
	void						RebuildIFN();
	void						RebuildAttributes();
	void						BuildAttribute(const FPopcornFXAttributeDesc *desc, const UPopcornFXAttributeList *attrList, uint32 attri, uint32 iCategory);
	void						RebuildSamplers();
	virtual void				BuildSampler(const FPopcornFXSamplerDesc *desc, const TSharedPtr<IPropertyHandle> samplerPty, const TSharedPtr<IPropertyHandle> samplerDescPty, const UPopcornFXAttributeList *attrList, uint32 sampleri, uint32 iCategory) {}

	bool								m_RefreshQueued;

	float								m_ColumnWidth;

	TArray<TWeakObjectPtr<UObject> >	m_BeingCustomized;
	TWeakPtr<IDetailLayoutBuilder>		m_DetailLayoutBuilder;
	TSharedPtr<IPropertyUtilities>		m_PropertyUtilities;
	IDetailCategoryBuilder				*m_AttributeListCategory;

	TSharedPtr<IPropertyHandle>			m_AttributeListPty;
	TSharedPtr<IPropertyHandle>			m_SamplersPty;
	TSharedPtr<IPropertyHandle>			m_SamplersDescPty;

	TSharedPtr<IPropertyHandle>			m_FileVersionIdPty;
	uint32								m_FileVersionId;
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
