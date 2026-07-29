//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplerText.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_samplers_classes.h>
#include <pk_render_helpers/include/frame_collector/rh_frame_data.h>

#include "PopcornFXPlugin.h"
#include "PopcornFXAttributeList.h"
#include "Assets/PopcornFXEffect.h"
#include "Assets/PopcornFXEffectPriv.h"


//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplerText"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplerText, Log, All);

//----------------------------------------------------------------------------
//
// FPopcornFXAttributeSamplerPropertiesText
//
//----------------------------------------------------------------------------

bool	FPopcornFXAttributeSamplerPropertiesText::ArePropertiesSupported(UPopcornFXEmitterComponent *emitter, const FString &samplerName)
{
	return true;
}

//----------------------------------------------------------------------------

bool	FPopcornFXAttributeSamplerPropertiesText::ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const FString &samplerName, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	return true;
}

//----------------------------------------------------------------------------
//
// FPopcornFXAttributeSamplerText
//
//----------------------------------------------------------------------------

struct FAttributeSamplerTextData
{
	PopcornFX::PParticleSamplerDescriptor_Text_Default	m_Desc;

	bool	m_NeedsReload;
};

//----------------------------------------------------------------------------

void	FPopcornFXAttributeSamplerText::SetText(FString InText)
{
	m_Data->m_NeedsReload = true;
	Properties.Text = InText;
}

//----------------------------------------------------------------------------

FPopcornFXAttributeSamplerText::FPopcornFXAttributeSamplerText()
{

	Properties.Text = "";
	// FPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Text;

	m_Data = new FAttributeSamplerTextData();
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeSamplerText::BeginDestroy()
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

void	FPopcornFXAttributeSamplerText::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.Property != NULL)
	{
		if (propertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesText, Text))
			m_Data->m_NeedsReload = true;
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	FPopcornFXAttributeSamplerText::CopyPropertiesFrom(const FPopcornFXAttributeSamplerProperties *other)
{
	const FPopcornFXAttributeSamplerPropertiesText *newTextProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesText *>(other);
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

//----------------------------------------------------------------------------

void	FPopcornFXAttributeSamplerText::RefreshFromProperties(const FPopcornFXAttributeSamplerProperties *other)
{
	const FPopcornFXAttributeSamplerPropertiesText *newTextProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesText *>(other);
	if (newTextProperties == null)
	{
		return;
	}

	if (newTextProperties->Text != Properties.Text)
	{
		m_Data->m_NeedsReload = true;
	}

	Properties = *newTextProperties;
}
//----------------------------------------------------------------------------

void	FPopcornFXAttributeSamplerPropertiesText::SetupDefaults(const PopcornFX::CParticleAttributeSamplerDeclaration *const decl, bool updateUnlockedValues)
{
	Super::SetupDefaults(decl, updateUnlockedValues);

	if (decl == null)
	{
		return;
	}
	const PopcornFX::PResourceDescriptor		defaultSampler = decl->AttribSamplerDefaultValue();

	const PopcornFX::CResourceDescriptor_Text *text = PopcornFX::HBO::Cast<PopcornFX::CResourceDescriptor_Text>(defaultSampler.Get());
	if (text != null)
	{
		if (updateUnlockedValues)
		{
			Text = ToUE(text->TextData());
		}
	}
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor *FPopcornFXAttributeSamplerText::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, const FPopcornFXAttributeSamplerProperties *properties, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);

	const PopcornFX::CResourceDescriptor_Text *defaultTextSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Text>(defaultSampler);
	if (!PK_VERIFY(defaultTextSampler != null))
		return null;

	const FPopcornFXAttributeSamplerPropertiesText *propertiesText = static_cast<const FPopcornFXAttributeSamplerPropertiesText *>(properties);
	if (propertiesText == nullptr)
		return null;
	Properties = *propertiesText;

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
