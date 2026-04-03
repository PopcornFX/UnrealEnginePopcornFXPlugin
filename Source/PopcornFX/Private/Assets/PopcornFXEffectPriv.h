//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

// Include "Internal/ParticleScene.h" before this header

class	CPopcornFXEffect
{
public:
	CPopcornFXEffect(UPopcornFXEffect *effect)
	:	m_Owner(effect)
	{
	}

	bool									RegisterEventCallback(const PopcornFX::FastDelegate<PopcornFX::CParticleEffect::EventCallback> &callback, const PopcornFX::CStringId &eventNameID);
	void									UnregisterEventCallback(const PopcornFX::FastDelegate<PopcornFX::CParticleEffect::EventCallback> &callback, const PopcornFX::CStringId &eventNameID);

	PopcornFX::PCParticleEffect				ParticleEffect();
	PopcornFX::PCParticleAttributeList		AttributeList(); // LoadEffectIFN
	PopcornFX::PCParticleAttributeList		AttributeListIFP() const;
	PopcornFX::PCParticleEffect				ParticleEffectIFP() const;
	PopcornFX::PParticleEffect				Unsafe_ParticleEffect() const;

private:
	PopcornFX::PCParticleEffect				m_ParticleEffect;

	friend class UPopcornFXEffect;
	UPopcornFXEffect						*m_Owner; // No refptr. CPopcornFXEffect is always accessed through the owner. It can't be invalid
};
