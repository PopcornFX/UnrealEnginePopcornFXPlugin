//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "PopcornFXAttributeSampler.h"

#include "PopcornFXAttributeSamplerText.generated.h"

struct	FAttributeSamplerTextData;

/** Can override an Attribute Sampler **Text** by a **FString**. */
UCLASS(EditInlineNew, meta=(BlueprintSpawnableComponent), ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXAttributeSamplerText : public UPopcornFXAttributeSampler
{
	GENERATED_UCLASS_BODY()

public:
	/** The Text to be sampled */
	UPROPERTY(Category="PopcornFX AttributeSampler", BlueprintReadOnly, EditAnywhere)
	FString		Text;

	UFUNCTION(BlueprintCallable, Category="PopcornFX|AttributeSampler")
	void		SetText(FString InText);

	// overrides
	void		BeginDestroy() override;
#if WITH_EDITOR
	void		PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

	// PopcornFX Internal
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) override;

private:
	FAttributeSamplerTextData	*m_Data;
};
