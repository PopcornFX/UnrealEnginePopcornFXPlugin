//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "Runtime/Launch/Resources/Version.h"
#include "PropertyEditorModule.h"
#include "PopcornFXAttributeList.h"

#include "IDetailCustomization.h"

class	UPopcornFXAttributeList;
class	UPopcornFXEffect;
class	IDetailGroup;
namespace { class	SPopcornFXAttributeCategory; }

class FPopcornFXDetailsAttributeList : public IDetailCustomization
{
public:
	typedef FPopcornFXDetailsAttributeList		TSelf;

	FPopcornFXDetailsAttributeList();
	~FPopcornFXDetailsAttributeList();

	static TSharedRef<IDetailCustomization>	MakeInstance();
	virtual void		CustomizeDetails(IDetailLayoutBuilder &detailBuilder);
	virtual void		CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder> &detailBuilder);
	void				RebuildDetails();
	void				RebuildAndRefresh();

	float				OnGetLeftColumnWidth() const { return 1.0f - m_ColumnWidth; }
	float				OnGetRightColumnWidth() const { return m_ColumnWidth; }
	void				OnSetColumnWidth(float width) { m_ColumnWidth = width; }

	UPopcornFXAttributeList			*AttrList();

public:
	TSharedPtr<IPropertyHandle>		m_AttributesPty;
	TSharedPtr<IPropertyHandle>		m_SamplersPty;

	TAttribute<float>				LeftColumnWidth;
	TAttribute<float>				RightColumnWidth;

private:
	EVisibility					AttribVisibilityAndRefresh();

	UPopcornFXAttributeList		*UnsafeAttrList();

	void						Rebuild();
	void						RebuildIFN();
	void						RebuildAttributes();
	void						BuildAttribute(const FPopcornFXAttributeDesc *desc, const UPopcornFXAttributeList *attrList, uint32 attri, uint32 iCategory);
	void						RebuildSamplers();
	void						BuildSampler(const FPopcornFXSamplerDesc *desc, const UPopcornFXAttributeList *attrList, uint32 sampleri, uint32 iCategory);

	bool						m_RefreshQueued;

	float						m_ColumnWidth;

	TArray<TWeakObjectPtr<UObject> >	m_BeingCustomized;
	TWeakPtr<IDetailLayoutBuilder>		m_DetailLayoutBuilder;
	TSharedPtr<IPropertyUtilities>		m_PropertyUtilities;

	TSharedPtr<IPropertyHandle>			m_FileVersionIdPty;
	uint32								m_FileVersionId;
	const UPopcornFXEffect				*m_Effect;

	TArray<IDetailGroup*>				m_IGroups;
	TArray<uint32>						m_NumAttributes;
};

#endif // WITH_EDITOR
