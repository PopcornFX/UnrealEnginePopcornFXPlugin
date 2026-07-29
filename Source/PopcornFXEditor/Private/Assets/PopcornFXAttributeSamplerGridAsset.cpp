#include "PopcornFXAttributeSamplerGridAsset.h"

UPopcornFXAttributeSamplerGridFactory::UPopcornFXAttributeSamplerGridFactory(FObjectInitializer const &PCIP)
	: Super(PCIP)
{
	SupportedClass = UPopcornFXAttributeSamplerGridAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}