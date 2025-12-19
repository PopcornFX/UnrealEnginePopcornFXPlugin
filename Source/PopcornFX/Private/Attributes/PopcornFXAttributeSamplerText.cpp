//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplerText.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXAttributeList.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_samplers_classes.h>

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplerText"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplerText, Log, All);

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSamplerText
//
//----------------------------------------------------------------------------

struct FAttributeSamplerTextData
{
	PopcornFX::PParticleSamplerDescriptor_Text_Default	m_Desc;

	bool	m_NeedsReload;
};

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerText::SetText(FString InText)
{
	m_Data->m_NeedsReload = true;
	Properties.Text = InText;
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerText::UPopcornFXAttributeSamplerText(const FObjectInitializer &PCIP)
	: Super(PCIP)
{
	bAutoActivate = true;

	Properties.Text = "";
	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Text;

	m_Data = new FAttributeSamplerTextData();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerText::BeginDestroy()
{
	if (m_Data != null)
	{
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

void	UPopcornFXAttributeSamplerText::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.Property != NULL)
	{
		if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesText, Text))
			m_Data->m_NeedsReload = true;
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerText::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	const FPopcornFXAttributeSamplerPropertiesText *newTextProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesText *>(other->GetProperties());
	if (!PK_VERIFY(newTextProperties != null))
	{
		UE_LOG(LogPopcornFXAttributeSamplerText, Error, TEXT("New properties are null or not text properties"));
		return;
	}

	if (newTextProperties->Text != Properties.Text)
	{
		m_Data->m_NeedsReload = true;
	}

	Super::CopyPropertiesFrom(other);

	Properties = *newTextProperties;
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor *UPopcornFXAttributeSamplerText::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	const PopcornFX::CResourceDescriptor_Text *defaultTextSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Text>(defaultSampler);
	if (!PK_VERIFY(defaultTextSampler != null))
		return null;
	if (m_Data->m_Desc == null)
	{
		m_Data->m_Desc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_Text_Default());
		if (!PK_VERIFY(m_Data->m_Desc != null))
			return null;
		m_Data->m_NeedsReload = true;
	}
	if (m_Data->m_NeedsReload)
	{
		// @TODO kerning
		PopcornFX::CFontMetrics *fontMetrics = null;
		bool						useKerning = false;
		if (!PK_VERIFY(m_Data->m_Desc->_Setup(ToPk(Properties.Text), fontMetrics, useKerning)))
			return null;
		m_Data->m_NeedsReload = false;
	}
	return m_Data->m_Desc.Get();
}

#undef LOCTEXT_NAMESPACE
