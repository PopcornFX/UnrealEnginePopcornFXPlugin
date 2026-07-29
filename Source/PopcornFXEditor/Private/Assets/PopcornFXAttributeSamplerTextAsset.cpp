#include "PopcornFXAttributeSamplerTextAsset.h"

UPopcornFXAttributeSamplerTextFactory::UPopcornFXAttributeSamplerTextFactory(FObjectInitializer const &PCIP)
	: Super(PCIP)
{
	SupportedClass = UPopcornFXAttributeSamplerTextAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}